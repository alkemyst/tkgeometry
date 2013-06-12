#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <array>

#include <boost/property_tree/info_parser.hpp>

#include "global_funcs.h"
#include "Polygon3d.h"
#include "Property.h"
#include "Sensor.h"
#include "Visitor.h"

using std::vector;
using std::string;
using std::array;

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
  virtual const Polygon3d<4>& basePoly() const = 0;

  virtual const XYZVector& center() const = 0;
  virtual const XYZVector& normal() const = 0;
  virtual double area() const = 0;
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

  const Polygon3d<4>& basePoly() const override { return basePoly_; }
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

  double area() const override { return length()*width(); }

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
  double minWidth() const { return minWidth_; }
  double maxWidth() const { return maxWidth_; }
  double area() const override { return area_; }
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

  const Polygon3d<4>& basePoly() const override { return decorated().basePoly(); }

  const XYZVector& center() const override { return decorated().center(); }
  const XYZVector& normal() const override { return decorated().normal(); }
  double area() const override { return decorated().area(); }
  double aperture() const override { return decorated().aperture(); }
  double dsDistance() const override { return decorated().dsDistance(); }
  double thickness() const override { return decorated().thickness(); }

  virtual double maxZ() const = 0;
  virtual double minZ() const = 0;
  virtual double maxR() const = 0;
  virtual double minR() const = 0;
  virtual double maxPhi() const = 0;
  virtual double minPhi() const = 0;

  void translate(const XYZVector& vector) override { decorated().translate(vector); }
  void rotateX(double angle) override { decorated().rotateX(angle); }
  void rotateY(double angle) override { decorated().rotateY(angle); }
  void rotateZ(double angle) override { decorated().rotateZ(angle); }
};



class BarrelModule : public DetectorModule {
//  double sideDisplacement_[2];
public:
 // enum class SideZ { MINZ = 0, MAXZ = 1 };

  Property<int, AutoDefault> layer;
  Property<int, AutoDefault> ring;
  Property<int, AutoDefault> rod;
  Property<int, AutoDefault> side;

  BarrelModule(Module* decorated) : DetectorModule(decorated) {}

  BarrelModule* clone() override { return new BarrelModule(*this); }
  void build();
  void accept(GenericGeometryVisitor& v) { v.visit(*this); decorated().accept(v); }

  double maxZ() const { return MAX(basePoly().getVertex(0).Z(), basePoly().getVertex(2).Z()); } 
  double minZ() const { return MIN(basePoly().getVertex(0).Z(), basePoly().getVertex(2).Z()); } 
  double maxR() const { return center().Rho(); }
  double minR() const { return center().Rho(); }
  double maxPhi() const { return MAX(basePoly().getVertex(0).Phi(), basePoly().getVertex(2).Phi()); }
  double minPhi() const { return MIN(basePoly().getVertex(0).Phi(), basePoly().getVertex(2).Phi()); }

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

  double minZ() const { return center().Z(); }
  double maxZ() const { return center().Z(); }
  double maxR() const { return MAX(basePoly().getVertex(0).Rho(), basePoly().getVertex(2).Rho()); }
  double minR() const { XYZVector side[2];
                        std::partial_sort_copy(basePoly().begin(), basePoly().end(), std::begin(side), std::end(side), [](const XYZVector& v1, const XYZVector& v2) { return v1.Rho() < v2.Rho(); });
                        return ((side[0]+side[1])/2).Rho(); }
  double maxPhi() const { return std::max_element(basePoly().begin(), basePoly().end(), [](const XYZVector& v1, const XYZVector& v2) { return v1.Phi() < v2.Phi(); })->Phi(); } 
  double minPhi() const { return std::min_element(basePoly().begin(), basePoly().end(), [](const XYZVector& v1, const XYZVector& v2) { return v1.Phi() < v2.Phi(); })->Phi(); } 

  void translateZ(double z) { decorated().translate(XYZVector(0, 0, z)); }
  //void translateR(double radius) { decorated().translate(radiusVector_*radius); } // CUIDADO it would need a translateR for symmetry with BarrelModules
};

// ===================================================================================================================================



// ======================================================== TYPED MODULE =============================================================


enum class ModuleType { SINGLE_SENSOR, DUAL_SENSOR };


class TypedModule : public Module, public Decorator<Module> {
  typedef vector<Sensor> Sensors;
protected:
  Sensors sensors_;
public:
  ReadonlyProperty<int, AutoDefault> numSparsifiedHeaderBits, numSparsifiedPayloadBits;

  static Module* decorate(const string& type, Module* m); 

  TypedModule(Module* decorated) : 
      Decorator<Module>(decorated),
      numSparsifiedHeaderBits("numSparsifiedHeaderBits", parsedOnly()),
      numSparsifiedPayloadBits("numSparsifiedPayloadBits", parsedOnly())
  {}

  const Polygon3d<4>& basePoly() const override { return decorated().basePoly(); }

  const XYZVector& center() const override { return decorated().center(); }
  const XYZVector& normal() const override { return decorated().normal(); }
  double area() const override { return decorated().area(); }
  double dsDistance() const override { return decorated().dsDistance(); }

  const Sensors& sensors() const { return sensors_; }

  void translate(const XYZVector& vector) override { decorated().translate(vector); for (auto& s : sensors_) s.poly().translate(vector); }
  void rotateX(double angle) override { decorated().rotateX(angle); for (auto& s : sensors_) s.poly().rotateX(angle); }
  void rotateY(double angle) override { decorated().rotateY(angle); for (auto& s : sensors_) s.poly().rotateY(angle); }
  void rotateZ(double angle) override { decorated().rotateZ(angle); for (auto& s : sensors_) s.poly().rotateZ(angle); }
};

class SingleSensorModule : public TypedModule {
  PropertyNode<int> sensorNode;
public:
  SingleSensorModule(Module* decorated) : TypedModule(decorated), 
    sensorNode("Sensor", parsedOnly()) {}

  double aperture() const { return decorated().aperture(); }
  double thickness() const { return decorated().dsDistance() + sensors_.front().sensorThickness(); }

  void build() override;

  SingleSensorModule* clone() override { return new SingleSensorModule(*this); }

  void accept(GenericGeometryVisitor& v) { v.visit(*this); decorated().accept(v); }
}; 

class DualSensorModule : public TypedModule {
  Property<double, UncachedComputable> aperture_;
  PropertyNode<int> sensorNode;
public:
  DualSensorModule(Module* decorated) :
      TypedModule(decorated),
      aperture_([this](){ return ModuleHelpers::polygonAperture(sensors_[0].poly()); }),
      sensorNode("Sensor", parsedOnly())
  {}

  double aperture() const { return aperture_(); }
  double thickness() const override { return decorated().dsDistance() + sensors_[0].sensorThickness()/2 + sensors_[1].sensorThickness()/2; }

  virtual void build() override;

};


class PtModule : public DualSensorModule {
public:
  ReadonlyProperty<int, Autodefault> numTriggerDataHeaderBits, numTriggerDataPayloadBits;

  PtModule(Module* decorated) : 
      DualSensorModule(decorated),
 
  {}
  PtModule* clone() override { return new PtModule(*this); }

  virtual void accept(GenericGeometryVisitor& v) { v.visit(*this); decorated().accept(v); }
};

class StereoModule : public DualSensorModule {
public:
  ReadonlyProperty<double, AutoDefault> stereoRotation;

  StereoModule(Module* decorated) : DualSensorModule(decorated) {}
  StereoModule* clone() override { return new StereoModule(*this); }

  virtual void accept(GenericGeometryVisitor& v) { v.visit(*this); decorated().accept(v); }
};

// ===================================================================================================================================



#endif
