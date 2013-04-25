#ifndef MODULE_H
#define MODULE_H

#include <vector>
#include <string>
#include <algorithm>
#include <functional>

using std::vector;
using std::string;

#include "Polygon3d.h"
#include "Property.h"
#include "ModuleType.h"


class Module : public PropertyObject, public Buildable, public Placeable, public Identifiable<Module> {
protected:
  virtual Polygon3d<4>& basePoly() = 0;
public:
  virtual const XYZVector& center() const = 0;
  virtual double dsDistance() const = 0;
  virtual double thickness() const = 0;

  virtual Module* clone() = 0;
  virtual void build() = 0;

  virtual void translate(const XYZVector& vector) = 0;
  virtual void rotateZ(double angle) = 0;
protected:
  virtual void rotateX(double angle) = 0;
  virtual void rotateY(double angle) = 0;
};

/// ===================================================== GEOMETRIC MODULES =====================================================
//
class GeometricModule : public Module {
  Polygon3d<4> basePoly_;
  Property<double, NoDefault> dsDistance_; // a GeometricModule is a purely 2d geometric object represented in 3d space with just a polygon and an additional for thickness value for tracker geometry construction
public:
  GeometricModule() :
      dsDistance_("dsDistance", checked())
  {}

  const Polygon3d<4>& basePoly() override { return basePoly_; }
  const XYZVector& center() const override { return basePoly_.center(); }
  double dsDistance() const override { return dsDistance_; }
  double thickness() const override { return dsDistance_ + 0.1; } // for Geometric modules it is assumed they have a 0.1 mm thick generic sensor

  void translate(const XYZVector& vector) override { basePoly_.translate(vector); }
  void rotateZ(double angle) override { basePoly_.rotateZ(angle); }
protected:
  void rotateX(double angle) override { basePoly_.rotateX(angle); }
  void rotateY(double angle) override { basePoly_.rotateY(angle); }
};

class RectangularModule : public GeometricModule {
public:
  Property<float, NoDefault> aspectRatio;
  Property<float, NoDefault> length;
  Property<float, NoDefault> width;

  RectangularModule() :
      aspectRatio("aspectRatio", unchecked()),
      length("length", unchecked()),
      width("width", unchecked())
  {}

  RectangularModule* clone() override { return new RectangularModule(*this); }

  void check() override;
  void build() override;
}

class WedgeModule : public GeometricModule, public Cloneable<WedgeModule> {
  float length_, minWidth_, maxWidth_;
  float area_,/*dist_*/;
  bool cropped_;
  float amountCropped_;
public:
  Property<float, NoDefault> buildAperture, buildDistance, cropDistance;
  float length() const { return length_; }
  float minWidth() const { return minWidth_; }
  float maxWidth() const { return maxWidth_; }
//  Shape shape() const { return Shape::WEDGE; }
  
  WedgeModule() :
      buildAperture("buildAperture", checked()),
      buildDistance("buildDistance", checked()),
      cropDistance("cropDistance",   checked())
  {}

  WedgeModule* clone() override { return new WedgeModule(*this); }

  void build() override;
};

enum class ModuleShape { RECTANGULAR, WEDGE };
// ========================================================================================================================================



// ======================================================= DETECTOR MODULES ===============================================================


class DetectorModule : public Module, public Decorator<Module> {  // implementors of the DetectorModule interface must take care of rotating the module based on which part of the subdetector it will be used in (Barrel, EC)
protected:
  Polygon3d<4>& basePoly() override { return decorated().basePoly(); }
public:
  DetectorModule(const Module* decorated) : Decorator<Module>(decorated) {}
  const XYZVector& center() const override { return decorated().center(); }
  double dsDistance() const override { return decorated().dsDistance(); }
  double thickness() const override { return decorated().thickness(); }


  void translate(const XYZVector& vector) override { decorated().translate(vector); }
  void rotateZ(double angle) override { decorated().rotateZ(angle); }
protected:
  void rotateX(double angle) override { decorated().rotateX(angle); }
  void rotateY(double angle) override { decorated().rotateY(angle); }
};



class BarrelModule : public DetectorModule {
public:
  Property<int, AutoDefault> layer;
  Property<int, AutoDefault> ring;
  Property<int, AutoDefault> rod;
  Property<int, AutoDefault> side;

  BarrelModule(const Module* decorated) : DetectorModule(decorated) {}

  BarrelModule* clone() override { return new BarrelModule(*this); }
  void build();
};

class EndcapModule : public DetectorModule {
public:
  Property<int, AutoDefault> disk;
  Property<int, AutoDefault> ring;
  Property<int, AutoDefault> blade; // CUIDADO Think of a better name!
  Property<int, AutoDefault> side;

  EndcapModule(const Module* decorated) : DecoratorModule(decorated) {}

  EndcapModule* clone() override { return new EndcapModule(*this); }
  void build();
};

// ===================================================================================================================================



// ======================================================== TYPED MODULE =============================================================


enum class ModuleType { SINGLE_SENSOR, DUAL_SENSOR };


class TypedModule : public Module, public Decorator<Module> {
protected:
  Polygon3d<4>& basePoly() override { return decorated().basePoly(); }
public:
  static TypedModule* decorate(const Module* m, ModuleType type); 

  TypedModule(const Module* decorated) : Decorator<Module>(decorated) {}
  const XYZVector& center() const override { return decorated().center(); }
  double dsDistance() const override { return decorated().dsDistance(); }


};

class SingleSensorModule : public TypedModule {
  Sensor sensor_;
public:
  SingleSensorModule(const Module* decorated) : TypedModule(decorated) {}

  double thickness() const { return decorated().dsDistance() + sensor_.sensorThickness(); }

  const Sensor& sensor() const { return sensor_; }

  void build() override;

  SingleSensorModule* clone() override { return new SingleSensorModule(*this); }

  void translate(const XYZVector& vector) override { decorated().translate(vector); sensor_.translate(vector); }
  void rotateZ(double angle) override { decorated().rotateZ(angle); sensor_.rotateZ(angle); }
protected:
  void rotateX(double angle) override { decorated().rotateX(angle); sensor_.rotateX(angle); }
  void rotateY(double angle) override { decorated().rotateY(angle); sensor_.rotateY(angle); }
}; 

class DualSensorModule : public TypedModule {
public:
  typedef array<2, Sensor> Sensors;
private:
  Sensors sensors_;
public:
  DualSensorModule(const Module* decorated) : TypedModule(decorated) {}

  double thickness() const override { return decorated().dsDistance() + sensors_[0].sensorThickness()/2 + sensors_[1].sensorThickness()/2; }

  const Sensors& sensors() const { return sensors_; }

  void build() override;

  DualSensorModule* clone() override { return new DualSensorModule(*this); }

  void translate(const XYZVector& vector) override { decorated().translate(vector); for (auto& s : sensors_) s.translate(vector); }
  void rotateZ(double angle) override { decorated().rotateZ(angle); for (auto& s : sensors_) s.rotateZ(angle); }
protected:
  void rotateX(double angle) override { decorated().rotateX(angle); for (auto& s : sensors_) s.rotateX(angle); }
  void rotateY(double angle) override { decorated().rotateY(angle); for (auto& s : sensors_) s.rotateY(angle); }
};

// ===================================================================================================================================



#endif
