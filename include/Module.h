#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <algorithm>
#include <functional>
#include "Property.h"

using std::vector;

class Module : public PropertyObject {
protected:
  ModuleType* type_;
  vector<Polygon3d<4> > facePolys_;
  
  Property<float> dsDistance_;
  Property<float> sensorThickness_;
  Property<int> numFaces_;
  Property<float> length_;
public:
  enum class Shape { RECTANGULAR, WEDGE };

  int numFaces() const { return numFaces_; }
  float length() const { return length_; }

  DerivedProperty<float, Module> thickness;
  DerivedProperty<float, Module> aperture;


  Module() :
      numFaces(this, "numFaces", Fallback(moduleType_->numFaces), RangeInclusive<int>(1,2)),
      length(this, "length", Fallback(moduleType_->length), std::greater<float>(0)),
      dsDistance(this, "dsDistance", moduleType_->dsDistance),
      sensorThickness(this, "sensorThickness", moduleType_->sensorThickness),
      thickness([&](){ return dsDistance() + sensorThickness(); }, ComputeOnce()),
      phiAperture([&]() { auto phi = std::minmax_element(outerFace().getCorners().begin(), outerFace().getCorners().end(), [](XYZVector& v1, XYZVector& v2) { return v1.Phi() < v2.Phi(); });
                          return *(phi.second) - *(phi.first); }, ComputeOnce())
  {}
  

  virtual Shape shape() const = 0;

  virtual void build() = 0;
  void translate(XYZVector translation);
  void rotatePhi(double angle);
};


class BarrelModule : public Module {
  typedef BarrelModule self_type;
  
  Property<float> width;
public:

  BarrelModule() :
      width(this, "width", Fallback(moduleType_->width), std::greater<float>(0))
  {}


  Shape shape() const { return RECTANGULAR; }

  void build();

};

class RectangularEndcapModule : public Module {
  typedef RectangularEndcapModule self_type;
  Property<float> width;
public:

  RectangularEndcapModule() :
      width(this, "width", Fallback(moduleType_->width), std::greater<float>(0))
  {}


  Shape shape() const { return RECTANGULAR; }

  void build();
};

class WedgeEndcapModule : public Module {
  typedef WedgeEndcapModule self_type;

  Property<float> waferDiameter;
public:
  Property<float> buildAperture, buildRadius;
  DerivedProperty<float> minWidth, maxWidth;
  

  WedgeEndcapModule() :
      waferDiameter(this, "waferDiameter", Fallback(moduleType_->waferDiameter))
      minWidth(NoDefault()), maxWidth(NoDefault()),
      buildAperture(NoDefault()), buildRadius(NoDefault()),
  {}


  Shape shape() const { return WEDGE; }

  void build();

};

#endif
