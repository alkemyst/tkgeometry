#ifndef PLOT_DRAWER_H
#define PLOT_DRAWER_H

#include <utility>
#include <map>
#include <set>
#include <iostream>

#include <TPolyLine.h>
#include <TPaletteAxis.h>
#include <TH2D.h>
#include <TText.h>
#include <TLatex.h>
#include <TLine.h>
#include <TCanvas.h>

#include <module.hh>
#include <layer.hh>





// ========================================================================================
// Here be STATISTICS
// To be extended at will!
// ========================================================================================


class NoStat {
  double value_;
public:
  NoStat() : value_(0) {}
  void fill(double value) { value_ = value; }
  double get() const { return value_; }
};

class Average {
  int counts_;
  double total_;
public:
  Average() : counts_(0), total_(0) {}
  void fill(double value) {
    total_ += value;
    counts_+= 1;
  }
  double get() const { return total_/counts_; }
};

class Max {
  double curr_;
public:
  Max() : curr_(0) {}
  void fill(double value) { curr_ = (value > curr_) ? value : curr_; }
  double get() const { return curr_; }
};

class Min {
  double curr_;
public:
  Min() : curr_(0) {}
  void fill(double value) { curr_ = (value < curr_) ? value : curr_; }
  double get() const { return curr_; }
};

class Sum {
  double value_;
public:
  Sum() : value_(0) {}
  void fill(double value) { value_ += value; }
  double get() const { return value_; }
};



// ==============================================================================================
// Here be VALUEGETTERS
// To be extended at will!
// ==============================================================================================


/*template<double (Module::*ModuleMethod)() const>
  struct Method {
  double operator()(const Module& m) const { return (m.*ModuleMethod)(); }
  };*/

template<class RetType, RetType (Module::*ModuleMethod)() const>
struct Method {
  double operator()(const Module& m) const { return (double)(m.*ModuleMethod)(); }
};

struct Property {
  const char* name;
  Property(const char* nameString) : name(nameString) {}
  double operator()(const Module& m) const { return m.getProperty(name); }
};

struct TotalIrradiatedPower {
  double operator()(const Module& m) { 
    double chipPower = m.getModuleType()->getPower(m.getNChannels());
    return m.getIrradiatedPowerConsumption()+chipPower;
  }
};


struct Type {
  double operator()(const Module& m) { return m.getColor(); }
};


struct CoordZ {
  double operator()(const Module& m) { return m.getMeanPoint().Z(); }
};




// =============================================================================================
// Here be DRAWSTYLES 
// To be extended at will!
// =============================================================================================

class DrawerPalette {
  double minValue_, maxValue_;
  TPaletteAxis* framePalette_;
public:
  DrawerPalette() : framePalette_(NULL) {}
  void setMinMaxValues(double minValue, double maxValue) {
    minValue_ = minValue * 1.001 - maxValue * 0.001;
    maxValue_ = maxValue * 1.001 - minValue * 0.001;
  }
  double getMinValue() const { return minValue_; }
  double getMaxValue() const { return maxValue_; }

  void setFramePalette(TPaletteAxis* framePalette) { 
    framePalette_ = framePalette; 
  }

  int getColor(double value) const {
    return framePalette_ ? framePalette_->GetValueColor(value) : value;
  }
};




struct FillStyle {
  template <class StatType> void operator()(TPolyLine& line, StatType& bin, const DrawerPalette& palette) const {
    line.SetFillColor(palette.getColor(bin.get()));
    line.DrawPolyLine(line.GetN(),line.GetX(),line.GetY(),"f");
  }
};

class ContourStyle {
  const int lineWidth_;
public:
  ContourStyle(int lineWidth = 2)  : lineWidth_(lineWidth) {}
  template <class StatType> void operator()(TPolyLine& line, StatType& bin, const DrawerPalette& palette) const {
    line.SetLineColor(palette.getColor(bin.get()));
    line.SetLineWidth(lineWidth_);
    line.DrawPolyLine(line.GetN(),line.GetX(),line.GetY());
  }
};


// ===============================================================================================
// Here be VALIDATORS 
// Extend at will!
// ===============================================================================================



template<const int SubdetType>
struct CheckType {
  bool operator()(const Module& m) const { return m.getSubdetectorType() & SubdetType; }
};

template<const int Section>
struct CheckSection {
  bool operator()(const Module& m) const { return m.getSection() & Section; } 
};

template<const int PhiIndex>
struct CheckPhiIndex {
  bool operator()(const Module& m) const { return m.getPhiIndex() == PhiIndex; }
};

// ===============================================================================================
// Here be COORDINATE CLASSES and LINEGETTER
// Careful what you do
// ===============================================================================================



struct Rounder {
  static const int mmFraction = 10;
  int round(double x) { return floor(x*mmFraction+0.5)/mmFraction; }
};



struct XY : public std::pair<int, int>, private Rounder {
  const bool valid;
  XY(const Module& m) : std::pair<int, int>(round(m.getMeanPoint().X()), round(m.getMeanPoint().Y())), valid(m.getMeanPoint().Z() >= 0) {}
  XY(const XYZVector& v) : std::pair<int, int>(round(v.X()), round(v.Y())), valid(v.Z() >= 0) {}
  // bool operator<(const XY& other) const { return (x() < other.x()) || (x() == other.x() && y() < other.y()); }
  int x() const { return this->first; }
  int y() const { return this->second; }
};


struct YZ : public std::pair<int, int>, private Rounder {
  const bool valid;
  YZ(const Module& m) : std::pair<int,int>(round(m.getMeanPoint().Z()), round(m.getMeanPoint().Rho())), valid(m.getMeanPoint().Z() >= 0) {}
  YZ(const XYZVector& v) : std::pair<int, int>(round(v.Z()), round(v.Rho())), valid(v.Z() >= 0) {}
  //  bool operator<(const YZ& other) const { return (y() < other.y()) || (y() == other.y() && z() < other.z()); }
  int y() const { return this->second; }
  int z() const { return this->first; }
};


struct YZFull : public YZ {
  const bool valid;
  YZFull(const Module& m) : YZ(m), valid(true) {}
  YZFull(const XYZVector& v) : YZ(v), valid(true) {}
};


template<class CoordType> class LineGetter {
  typedef typename CoordType::first_type CoordTypeX;  // C++ syntax is bewildering sometimes (to say the least)
  typedef typename CoordType::first_type CoordTypeY;
  CoordTypeX maxx_, minx_;
  CoordTypeY maxy_, miny_;
public:
  LineGetter() : maxx_(std::numeric_limits<double>::min()), minx_(std::numeric_limits<double>::max()), maxy_(std::numeric_limits<double>::min()), miny_(std::numeric_limits<double>::max()) {}
  CoordTypeX maxx() const { return maxx_; }
  CoordTypeX minx() const { return minx_; }
  CoordTypeY maxy() const { return maxy_; }
  CoordTypeY miny() const { return miny_; }
  TPolyLine* operator()(const Module& m) {
    std::set<CoordType> xy; // duplicate detection
    double x[] = {0., 0., 0., 0., 0.}, y[] = {0., 0., 0., 0., 0.};
    int j=0;
    for (int i=0; i<4; i++) {
      CoordType c(m.getCorner(i));
      if (xy.insert(c).second == true) {
        x[j] = c.first;
        y[j++] = c.second;
      } 
      maxx_ = MAX(c.first, maxx_);
      minx_ = MIN(c.first, minx_);
      maxy_ = MAX(c.second, maxy_);
      miny_ = MIN(c.second, miny_);
    }
    if (j==4) { // close the poly line in case it's made of 4 distinct points, to get the closing line drawn
      x[j] = x[0]; 
      y[j++] = y[0];
    }
    return new TPolyLine(j, x, y);
  }
};





// ===============================================================================================
// Here be FRAMEGETTERS & FRAMESTYLERS
// Careful what you do
// ===============================================================================================


class IdMaker {
  static int id;
public:
  int nextId() const { return id++; }
  std::string nextString() const { 
    std::stringstream sid(""); 
    sid << nextId();
    return sid.str(); 
  }
};



template<class CoordType> class FrameGetter : private IdMaker {
public:
  TH2D* operator()(double viewportX, double viewportY) const;
};



template<class CoordType> class SummaryFrameStyle {
  void drawEtaTicks(double maxL, double maxR, double tickDistance, double tickLength, double textDistance,
                    Style_t labelFont, Float_t labelSize, double etaStep, double etaMax, double etaLongLine) const;
public:
  void operator()(TH2D& frame, TCanvas& canvas, DrawerPalette&) const;

};


template<class CoordType>
struct HistogramFrameStyle {
  void operator()(TH2D& frame, TCanvas& canvas, DrawerPalette& palette) const {
    frame.Fill(1,1,0);
    frame.SetMaximum(palette.getMaxValue());
    frame.SetMinimum(palette.getMinValue());

    frame.Draw("colz");
    canvas.Update();
    palette.setFramePalette((TPaletteAxis*)frame.GetListOfFunctions()->FindObject("palette"));
    frame.Draw("AXIS colz");
  }
};


// ===============================================================================================
// Here be PLOTDRAWER MAIN CLASS
// Do not touch please
// ===============================================================================================






template<class CoordType, class ValueGetterType, class StatType = NoStat >
class PlotDrawer {
  typedef typename CoordType::first_type CoordTypeX;
  typedef typename CoordType::second_type CoordTypeY;
  CoordTypeX viewportMaxX_;
  CoordTypeY viewportMaxY_;
  ValueGetterType getValue;
  FrameGetter<CoordType> getFrame;
  LineGetter<CoordType> getLine;

  DrawerPalette palette_;

  std::map<CoordType, StatType*> bins_;
  std::map<CoordType, TPolyLine*> lines_;

public: 
  PlotDrawer(CoordTypeX viewportMaxX = 0, CoordTypeY viewportMaxY = 0, const ValueGetterType& valueGetter = ValueGetterType()) : viewportMaxX_(viewportMaxX), viewportMaxY_(viewportMaxY), getValue(valueGetter) {}

  ~PlotDrawer();

  template<template<class> class FrameStyleType> void drawFrame(TCanvas& canvas, const FrameStyleType<CoordType>& frameStyle = FrameStyleType<CoordType>());
  template<class DrawStyleType> void drawModules(TCanvas& canvas, const DrawStyleType& drawStyle = DrawStyleType());

  void add(const Module& m);

  void addModulesType(const std::vector<Layer*>& layers, int moduleTypes = Module::Barrel | Module::Endcap); // moduleTypes takes the type of modules to be added as an OR list. Check the possible values in module.hh
  template<class InputIterator> void addModulesType(InputIterator begin, InputIterator end, int moduleTypes = Module::Barrel | Module::Endcap);
  template<class ModuleValidator> void addModules(const std::vector<Layer*>& layers, const ModuleValidator& isValid = ModuleValidator());
  template<class InputIterator, class ModuleValidator> void addModulesType(InputIterator begin, InputIterator end, const ModuleValidator& isValid);

};


template<class CoordType, class ValueGetterType, class StatType> PlotDrawer<CoordType, ValueGetterType, StatType>::~PlotDrawer() {
  for (typename std::map<CoordType, StatType*>::iterator it = bins_.begin(); it != bins_.end(); ++it) {
    delete it->second;
    delete lines_[it->first];
  }
  bins_.clear();
  lines_.clear();
}

template<class CoordType, class ValueGetterType, class StatType>
template<template<class> class FrameStyleType>
void PlotDrawer<CoordType, ValueGetterType, StatType>::drawFrame(TCanvas& canvas, const FrameStyleType<CoordType>& frameStyle) {
  double minValue = std::numeric_limits<double>::max();
  double maxValue = 0;
  for (typename std::map<CoordType, StatType*>::const_iterator it = bins_.begin(); it != bins_.end(); ++it) {
    double value = it->second->get();
    minValue = value < minValue ? value : minValue;
    maxValue = value > maxValue ? value : maxValue; 
  }
  palette_.setMinMaxValues(minValue, maxValue);
  canvas.SetFillColor(kWhite);
  canvas.cd();
  viewportMaxX_ = viewportMaxX_ == 0 ? getLine.maxx()*1.1 : viewportMaxX_;  // in case the viewport coord is 0, auto-viewport mode is used and getLine is queried for the farthest X or Y it has registered
  viewportMaxY_ = viewportMaxY_ == 0 ? getLine.maxy()*1.1 : viewportMaxY_;
  TH2D* frame = getFrame(viewportMaxX_, viewportMaxY_);
  frameStyle(*frame, canvas, palette_);
}



template<class CoordType, class ValueGetterType, class StatType>
template<class DrawStyleType>
void PlotDrawer<CoordType, ValueGetterType, StatType>::drawModules(TCanvas& canvas, const DrawStyleType& drawStyle) {
  canvas.cd();
  for (typename std::map<CoordType, StatType*>::const_iterator it = bins_.begin(); it != bins_.end(); ++it) {
    StatType* bin = it->second;
    TPolyLine* line = lines_.at(it->first);
    drawStyle(*line, *bin, palette_);
  }
}

template<class CoordType, class ValueGetterType, class StatType>
void PlotDrawer<CoordType, ValueGetterType, StatType>::add(const Module& m) {
  CoordType c(m);
  if (!c.valid) return;  // for XY and YZ plots negative Z modules are not needed and are therefore filtered out, YZFull plots on the other hand retain all the modules (CUIDADO I don't actually like the way this works)
  if (bins_[c] == NULL) {
    bins_[c] = new StatType();
    lines_[c] = getLine(m);
  }
  double value = getValue(m);
  bins_[c]->fill(value);
}

template<class CoordType, class ValueGetterType, class StatType>
void PlotDrawer<CoordType, ValueGetterType, StatType>::addModulesType(const std::vector<Layer*>& layers, int moduleTypes) {
  for (std::vector<Layer*>::const_iterator it = layers.begin(); it != layers.end(); ++it) {
    std::vector<Module*>* layerModules = (*it)->getModuleVector();
    for (std::vector<Module*>::const_iterator modIt=layerModules->begin(); modIt!=layerModules->end(); ++modIt) {
      //if ((*modIt)->getMeanPoint().Z()<0) continue;
      int subDet = (*modIt)->getSubdetectorType();
      if (subDet & moduleTypes) add(**modIt);
    }
  }
}

template<class CoordType, class ValueGetterType, class StatType>
template<class InputIterator>
void PlotDrawer<CoordType, ValueGetterType, StatType>::addModulesType(InputIterator begin, InputIterator end, int moduleTypes) {
  for (InputIterator it = begin; it != end; ++it) {
      int subDet = (*it)->getSubdetectorType();
      if (subDet & moduleTypes) add(**it);
  }
}

template<class CoordType, class ValueGetterType, class StatType>
template<class ModuleValidator>
void PlotDrawer<CoordType, ValueGetterType, StatType>::addModules(const std::vector<Layer*>& layers, const ModuleValidator& isValid) {
  for (std::vector<Layer*>::const_iterator it = layers.begin(); it != layers.end(); ++it) {
    std::vector<Module*>* layerModules = (*it)->getModuleVector();
    for (std::vector<Module*>::const_iterator modIt=layerModules->begin(); modIt!=layerModules->end(); ++modIt) {
      if (/*(*modIt)->getMeanPoint().Z()>0 &&*/ isValid(**modIt)) add(**modIt);
    }
  }
}

template<class CoordType, class ValueGetterType, class StatType>
template<class InputIterator, class ModuleValidator>
void PlotDrawer<CoordType, ValueGetterType, StatType>::addModulesType(InputIterator begin, InputIterator end, const ModuleValidator& isValid) {
  for (InputIterator it = begin; it != end; ++it) {
      if (isValid(**it)) add(**it);
  }
}
#endif
