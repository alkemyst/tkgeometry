#ifndef RODPAIR_H
#define RODPAIR_H

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <algorithm>

#include <boost/ptr_container/ptr_vector.hpp>

#include "global_funcs.h"
#include "Property.h"
#include "Module.h"
#include "messageLogger.h"

using std::string;
using std::vector;
using std::pair;
using std::unique_ptr;

typedef vector<unique_ptr<BarrelModule>> RodTemplate;

class RodPair : public PropertyObject, public Buildable, public Identifiable<int> {
public:
  typedef boost::ptr_vector<BarrelModule> Container;
protected:
  Container zPlusModules_, zMinusModules_;

  enum class BuildDir { RIGHT = 1, LEFT = -1 };
  enum class StartZMode { MODULECENTER, MODULEEDGE };

  Property<StartZMode, Default> startZMode;

private:
  void clearComputables();
public:
  Property<double, NoDefault> maxZ;
  Property<double, Computable> minZ, maxR, minR;
  ReadonlyProperty<double, Computable> minAperture;
  ReadonlyProperty<double, Computable> maxAperture;

  RodPair() :
      startZMode("startZMode", parsedAndChecked(), StartZMode::MODULECENTER) 
  {}

  void setup() {
    minAperture.setup([this]() { double min = 999; for (auto& m : zPlusModules_) { min = MIN(min, m.phiAperture()); } return min; }); // CUIDADO not checking the zMinus modules, check if this could cause problems down the road
    maxAperture.setup([this]() { double max = 0; for (auto& m : zPlusModules_) { max = MAX(max, m.phiAperture()); } return max; });
    minZ.setup([&]() { double min = 99999; for (const auto& m : zMinusModules_) { min = MIN(min, m.minZ()); } return min; }); // we want the minZ so we don't bother with scanning the zPlus vector
    minR.setup([&]() { double min = 99999; for (const auto& m : zPlusModules_) { min = MIN(min, m.minR()); } return min; }); // min and maxR can be found by just scanning the zPlus vector, since the rod pair is symmetrical in R
    maxR.setup([&]() { double max = 0; for (const auto& m : zPlusModules_) { max = MAX(max, m.maxR()); } return max; });
    for (auto& m : zPlusModules_) m.setup();
    for (auto& m : zMinusModules_) m.setup();
  }

  int numModules() const { return zPlusModules_.size() + zMinusModules_.size(); }
  int numModulesSide(int side) const { return side >= 0 ? zPlusModules_.size() : zMinusModules_.size(); }

  void translate(const XYZVector& translation);
  void translateR(double radius);
  void rotateZ(double angle);

  void cutAtEta(double eta);

  const std::pair<const Container&,const Container&> modules() const { return std::pair<const Container&,const Container&>(zPlusModules_,zMinusModules_); }
  
  void accept(GeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& m : zPlusModules_) { m.accept(v); }
    for (auto& m : zMinusModules_) { m.accept(v); }
  }
  void accept(ConstGeometryVisitor& v) const { 
    v.visit(*this); 
    for (const auto& m : zPlusModules_) { m.accept(v); }
    for (const auto& m : zMinusModules_) { m.accept(v); }
  }

};

class StraightRodPair : public RodPair {

  // Templated because they need to work both with forward and reverse iterators (mezzanines are built right to left and the rodTemplate vector is iterated backwards)
  double computeNextZ(double newDsDistance, double lastDsDistance, double lastZ, BuildDir direction, int parity);
  template<typename Iterator> vector<double> computeZList(Iterator begin, Iterator end, double startZ, BuildDir direction, int smallParity, bool fixedStartZ);
  template<typename Iterator> pair<vector<double>, vector<double>> computeZListPair(Iterator begin, Iterator end, double startZ, int recursionCounter);
  void buildModules(Container& modules, const RodTemplate& rodTemplate, const vector<double>& posList, BuildDir direction, int parity, int side);
  void buildFull(const RodTemplate& rodTemplate); 
  void buildMezzanine(const RodTemplate& rodTemplate); 

public:
  Property<double, NoDefault> smallDelta;
  Property<double, NoDefault> minBuildRadius;
  Property<double, NoDefault> maxBuildRadius;

  Property<double, Default> minModuleOverlap;
  Property<double, NoDefault> zError;
  Property<int, NoDefault> zPlusParity;
  Property<int, NoDefault> buildNumModules;
  Property<bool, Default> mezzanine;
  Property<double, NoDefault> startZ;
  Property<bool, Default> compressed;
  Property<bool, Default> allowCompressionCuts;

  PropertyNode<int> ringNode;

  
  StraightRodPair() :
              minModuleOverlap    ("minModuleOverlap"    , parsedAndChecked() , 1.),
              zError              ("zError"              , parsedAndChecked()),
              zPlusParity         ("smallParity"         , parsedAndChecked()),
              mezzanine           ("mezzanine"           , parsedOnly(), false),
              startZ              ("startZ"              , parsedOnly()),
              compressed          ("compressed"          , parsedOnly(), true),
              allowCompressionCuts("allowCompressionCuts", parsedOnly(), true),
              ringNode            ("Ring"                , parsedOnly())
  {}


  
  void build(const RodTemplate& rodTemplate);

  std::set<int> solveCollisionsZPlus();
  std::set<int> solveCollisionsZMinus();
  void compressToZ(double z);

};


struct TiltedModuleSpecs {
  double r, z, gamma;
  bool valid() const {
    return r > 0.0 && fabs(gamma) <= 2*M_PI;
  }
};

class TiltedRodPair : public RodPair {
  void buildModules(Container& modules, const RodTemplate& rodTemplate, const vector<TiltedModuleSpecs>& tmspecs, BuildDir direction);
public:
  void build(const RodTemplate& rodTemplate, const std::vector<TiltedModuleSpecs>& tmspecs);

}; 



#endif
