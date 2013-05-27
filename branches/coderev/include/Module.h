#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <array>

#include <boost/property_tree/info_parser.hpp>

using std::vector;
using std::string;
using std::array;

#include "Polygon3d.h"
#include "Property.h"
#include "Sensor.h"
#include "Visitor.h"


namespace ModuleHelpers {
  double polygonAperture(const Polygon3d<4>& poly); 
}

namespace Paths {
  const string stdconfig = "stdcfg";
  const string moduleTypes = "moduleTypes";
  const string sep = "/";
}


class Module : public PropertyObject, public Buildable, public Placeable, public Identifiable<Module> {
public:
  virtual const Polygon3d<4>& basePoly() = 0;

  virtual const XYZVector& center() const = 0;
  virtual const XYZVector& normal() const = 0;
  virtual double aperture() const = 0;
  virtual double dsDistance() const = 0;
  virtual double thickness() const = 0;

  virtual Module* clone() = 0;
  virtual void build() = 0;

  virtual void translate(const XYZVector& vector) = 0;
  virtual void rotateX(double angle) = 0;
  virtual void rotateY(double angle) = 0;
  virtual void rotateZ(double angle) = 0;

  virtual void accept(GenericGeometryVisitor& v) = 0;
};

/// ===================================================== GEOMETRIC MODULES =====================================================
//
class GeometricModule : public Module {
  Property<double, Default> dsDistance_; // a GeometricModule is a purely 2d geometric object represented in 3d space with just a polygon and an additional for thickness value for tracker geometry construction
  Property<double, UncachedComputable> aperture_;
protected:
  Polygon3d<4> basePoly_;
public:
  GeometricModule() :
      dsDistance_("dsDistance", parsedAndChecked(), 0.),
      aperture_([this](){ return ModuleHelpers::polygonAperture(basePoly_); })
  {}

  const Polygon3d<4>& basePoly() override { return basePoly_; }
  const XYZVector& center() const override { return basePoly_.getCenter(); }
  const XYZVector& normal() const override { return basePoly_.getNormal(); }
  double aperture() const { return aperture_(); }
  double dsDistance() const override { return dsDistance_(); }
  void dsDistance(double dist) { dsDistance_(dist); }
  double thickness() const override { return dsDistance_() + 0.1; } // for Geometric modules it is assumed they have a 0.1 mm thick generic sensor

  void translate(const XYZVector& vector) override { basePoly_.translate(vector); }
  void rotateX(double angle) override { basePoly_.rotateX(angle); }
  void rotateY(double angle) override { basePoly_.rotateY(angle); }
  void rotateZ(double angle) override { basePoly_.rotateZ(angle); }
};

class RectangularModule : public GeometricModule {
public:
  Property<float, NoDefault> length;
  Property<float, NoDefault> width;
  Property<float, NoDefault> aspectRatio;
  Property<float, Default> waferDiameter;

  RectangularModule() :
      length("length", parsedOnly()), // not checked because a custom checker is defined for RectangularModules
      width("width", parsedOnly()),  // same here
      aspectRatio("aspectRatio", parsedOnly()), // same here
      waferDiameter("waferDiameter", parsedAndChecked(), 131.)
  {}

  RectangularModule* clone() override { return new RectangularModule(*this); }

  void check() override;
  void build() override;

  void accept(GenericGeometryVisitor& v) { v.visit(*this); }
};

class WedgeModule : public GeometricModule {
  double length_, minWidth_, maxWidth_;
  double area_,/*dist_*/;
  bool cropped_;
  double amountCropped_;
public:
  Property<float, NoDefault> buildAperture, buildDistance, buildCropDistance;
  Property<float, Default> waferDiameter;
  double length() const { return length_; }
  float minWidth() const { return minWidth_; }
  float maxWidth() const { return maxWidth_; }
//  Shape shape() const { return Shape::WEDGE; }
  
  WedgeModule() :
      buildAperture("buildAperture", parsedAndChecked()),
      buildDistance("buildDistance", parsedAndChecked()),
      buildCropDistance("buildCropDistance", parsedAndChecked()),
      waferDiameter("waferDiameter", parsedAndChecked(), 131.)
  {}

  WedgeModule* clone() override { return new WedgeModule(*this); }

  void build() override;

  void accept(GenericGeometryVisitor& v) { v.visit(*this); }
};

enum class ModuleShape { RECTANGULAR, WEDGE };
// ========================================================================================================================================



// ======================================================= DETECTOR MODULES ===============================================================


class DetectorModule : public Module, public Decorator<Module> {  // implementors of the DetectorModule interface must take care of rotating the module based on which part of the subdetector it will be used in (Barrel, EC)
public:
  DetectorModule(Module* decorated) : Decorator<Module>(decorated) {}

  const Polygon3d<4>& basePoly() override { return decorated().basePoly(); }

  const XYZVector& center() const override { return decorated().center(); }
  const XYZVector& normal() const override { return decorated().normal(); }
  double aperture() const override { return decorated().aperture(); }
  double dsDistance() const override { return decorated().dsDistance(); }
  double thickness() const override { return decorated().thickness(); }

  void translate(const XYZVector& vector) override { decorated().translate(vector); }
  void rotateX(double angle) override { decorated().rotateX(angle); }
  void rotateY(double angle) override { decorated().rotateY(angle); }
  void rotateZ(double angle) override { decorated().rotateZ(angle); }
};



class BarrelModule : public DetectorModule {
//  double sideDisplacement_[2];
public:
  enum class SideZ { MINZ = 0, MAXZ = 1 };

  Property<int, AutoDefault> layer;
  Property<int, AutoDefault> ring;
  Property<int, AutoDefault> rod;
  Property<int, AutoDefault> side;

  Property<double, UncachedComputable> minZ, maxZ; // CUIDADO add caching

  BarrelModule(Module* decorated) : DetectorModule(decorated), 
    minZ([this]() { double min = std::numeric_limits<double>::max(); for (const auto& v : basePoly()) { min = MIN(min, v.Z()); } return min; }),
    maxZ([this]() { double max = std::numeric_limits<double>::min(); for (const auto& v : basePoly()) { max = MAX(max, v.Z()); } return max; })
  {}

  BarrelModule* clone() override { return new BarrelModule(*this); }
  void build();

  void accept(GenericGeometryVisitor& v) { v.visit(*this); decorated().accept(v); }

  void translateZ(double z) { decorated().translate(XYZVector(0, 0, z)); }
//  void translateZ(SideZ side, double z) { decorated().translate(XYZVector(0, 0, z + sideDisplacement_[SideZ])); }
  void translateR(double radius) { decorated().translate(decorated().normal()*radius); }
};

class EndcapModule : public DetectorModule {
public:
  Property<int, AutoDefault> disk;
  Property<int, AutoDefault> ring;
  Property<int, AutoDefault> blade; // CUIDADO Think of a better name!
  Property<int, AutoDefault> side;

  EndcapModule(Module* decorated) : DetectorModule(decorated) {}

  EndcapModule* clone() override { return new EndcapModule(*this); }
  void build();

  void accept(GenericGeometryVisitor& v) { v.visit(*this); decorated().accept(v); }

  void translateZ(double z) { decorated().translate(XYZVector(0, 0, z)); }
  //void translateR(double radius) { decorated().translate(radiusVector_*radius); } // CUIDADO it would need a translateR for symmetry with BarrelModules
};

// ===================================================================================================================================



// ======================================================== TYPED MODULE =============================================================


enum class ModuleType { SINGLE_SENSOR, DUAL_SENSOR };


class TypedModule : public Module, public Decorator<Module> {
public:
  static Module* decorate(const string& type, Module* m); 

  TypedModule(Module* decorated) : Decorator<Module>(decorated) {}

  const Polygon3d<4>& basePoly() override { return decorated().basePoly(); }

  const XYZVector& center() const override { return decorated().center(); }
  const XYZVector& normal() const override { return decorated().normal(); }
  double dsDistance() const override { return decorated().dsDistance(); }

};

class SingleSensorModule : public TypedModule {
  Sensor sensor_;
  PropertyNode<int> sensorNode;
public:
  SingleSensorModule(Module* decorated) : TypedModule(decorated), 
    sensorNode("Sensor", parsedOnly()) {}

  double aperture() const { return decorated().aperture(); }
  double thickness() const { return decorated().dsDistance() + sensor_.sensorThickness(); }

  const Sensor& sensor() const { return sensor_; }

  void build() override;

  SingleSensorModule* clone() override { return new SingleSensorModule(*this); }

  void translate(const XYZVector& vector) override { decorated().translate(vector); sensor_.poly().translate(vector); }
  void rotateX(double angle) override { decorated().rotateX(angle); sensor_.poly().rotateX(angle); }
  void rotateY(double angle) override { decorated().rotateY(angle); sensor_.poly().rotateY(angle); }
  void rotateZ(double angle) override { decorated().rotateZ(angle); sensor_.poly().rotateZ(angle); }

  void accept(GenericGeometryVisitor& v) { v.visit(*this); decorated().accept(v); }
}; 

class DualSensorModule : public TypedModule {
public:
  typedef array<Sensor, 2> Sensors;
private:
  Property<double, UncachedComputable> aperture_;
  PropertyNode<int> sensorNode;
  Sensors sensors_;
public:
  DualSensorModule(Module* decorated) :
      TypedModule(decorated),
      aperture_([this](){ return ModuleHelpers::polygonAperture(sensors_[0].poly()); }),
      sensorNode("Sensor", parsedOnly())
  {}

  double aperture() const { return aperture_(); }
  double thickness() const override { return decorated().dsDistance() + sensors_[0].sensorThickness()/2 + sensors_[1].sensorThickness()/2; }

  const Sensors& sensors() const { return sensors_; }

  virtual void build() override;

  virtual DualSensorModule* clone() override { return new DualSensorModule(*this); }

  void translate(const XYZVector& vector) override { decorated().translate(vector); for (auto& s : sensors_) s.poly().translate(vector); }
  void rotateX(double angle) override { decorated().rotateX(angle); for (auto& s : sensors_) s.poly().rotateX(angle); }
  void rotateY(double angle) override { decorated().rotateY(angle); for (auto& s : sensors_) s.poly().rotateY(angle); }
  void rotateZ(double angle) override { decorated().rotateZ(angle); for (auto& s : sensors_) s.poly().rotateZ(angle); }

  void accept(GenericGeometryVisitor& v) { v.visit(*this); decorated().accept(v); }
};


class PtModule : public DualSensorModule {
public:
  PtModule* clone() override { return new PtModule(*this); }
};

class StereoModule : public DualSensorModule {
public:
  ReadonlyProperty<double, AutoDefault> stereoRotation;

  StereoModule* clone() override { return new StereoModule(*this); }
};

// ===================================================================================================================================



#endif
