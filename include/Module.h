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
  virtual double thickness() const = 0;

  virtual Module* clone() = 0;
  virtual void build() = 0;
};

/// ===================================================== GEOMETRIC MODULES =====================================================
//
class GeometricModule : public Module {
  Polygon3d<4> basePoly_;
  Property<double, NoDefault> dsDistance_; // a GeometricModule is a purely 2d geometric object represented in 3d space with just a polygon and an additional for thickness value for tracker geometry construction
protected:
  Polygon3d<4>& basePoly() { return basePoly_; }
public:
  GeometricModule() :
      thickness_("thickness", checked())
  {}

  const XYZVector& center() const { return basePoly_.center(); }
  double thickness() const { return thickness_; }

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

  RectangularModule* clone() { return new RectangularModule(*this); }

  void check();
  void build();
}

class WedgeModule : public GeometricModule {
  Polygon3d<4> basePoly_;
protected:
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

  WedgeModule* clone() { return new WedgeModule(*this); }

  void build();
};

// ========================================================================================================================================



// ======================================================= DETECTOR MODULES ===============================================================


class DetectorModule : public Module, public Decorator<Module> {  // implementors of the DetectorModule interface must take care of rotating the module based on which part of the subdetector it will be used in (Barrel, EC)
protected:
  Polygon3d<4>& basePoly() override { return decorated()->basePoly(); }
public:
  DetectorModule(const Module* decorated) : Decorator<Module>(decorated) {}
  const XYZVector& center() const override { return decorated()->center(); }
  double thickness() const override { return decorated()->thickness(); }

  virtual void build() = 0;
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


class TypedModule : public Module, public Decorator<Module> {
protected:
  Polygon3d<4>& basePoly() override { return decorated()->basePoly(); }
public:
  TypedModule(const Module* decorated) : Decorator<Module>(decorated) {}
  const XYZVector& center() const override { return decorated()->center(); }
  Polygon3d<4>& thickness() override { return decorated()->thickness(); }

  void build();
};

class SingleSensorModule : public TypedModule {
  Sensor sensor_;
public:
  SingleSensorModule(const Module* decorated) : TypedModule(decorated) {}

  const Sensor& sensor() const { return sensor_; }

  SingleSensorModule* clone() override { return new SingleSensorModule(*this); }
}; 

class DualSensorModule : public TypedModule {
public:
  typedef array<2, Sensor> Sensors;
private:
  Sensors sensors_;
public:
  DualSensorModule(const Module* decorated) : TypedModule(decorated) {}

  const Sensors& sensors() const { return sensors_; }

  DualSensorModule* clone() override { return new DualSensorModule(*this); }
};



enum class Shape { RECTANGULAR, WEDGE };


class Module : public PropertyObject, public Buildable, public Placeable, public Identifiable<Module> {
protected:
  ModuleType* type_;
  vector<Polygon3d<4>> facePolys_;
  
  Property<string, NoDefault> moduleType;
  Property<int, Computable> numFaces;
  Property<float, Computable> sensorThickness;

  const Polygon3d<4>& innerFace() const { return facePolys_.front(); }
  const Polygon3d<4>& outerFace() const { return facePolys_.back(); }
public:
  ReadonlyProperty<float, Computable> waferDiameter;
  ReadonlyProperty<float, Computable> dsDistance;

  ReadonlyProperty<float, Computable> thickness;
  ReadonlyProperty<float, Computable> aperture;


//  virtual Shape shape() const = 0;

  Module() :
      moduleType("moduleType", unchecked()),
      numFaces("numFaces", unchecked(), [this]() { return type_->numFaces(); }),
      sensorThickness("sensorThickness", unchecked(), [this]() { return type_->sensorThickness(); }),

      waferDiameter("waferDiameter", unchecked(), [this]() { return type_->waferDiameter(); }),
      dsDistance("dsDistance", unchecked(), [this]() { return type_->dsDistance(); }),

      thickness([&]() { return dsDistance() + sensorThickness(); }),
      aperture([&]() { float max = 0, min = 999; 
                       for (auto& vertex : outerFace()) { max = MAX(max, vertex.Phi()); min = MIN(min, vertex.Phi()); } 
                       return max - min; })
  {}

  virtual Module* clone() = 0;
  
  virtual void check() {
    type_ = moduleType.state() ? ModuleTypeRepo::getInstance().get(moduleType()) : ModuleTypeRepo::getInstance().getDefault();
    PropertyObject::check();
  }

  virtual void build() = 0;
  void translate(const XYZVector& translation);
  void rotatePhi(double angle);


  const XYZVector& getCorner(int index) const { return innerFace().getVertex(index); } // LEGACY INTERFACE 
  const XYZVector& getMeanPoint() const { return center(); }
};

class RectangularModule : public virtual Module {
  
  Property<float, NoDefault> aspectRatio;
public:
  Property<float, NoDefault> length;
  Property<float, NoDefault> width;
//  Shape shape() const { return Shape::RECTANGULAR; }

  RectangularModule() :
      aspectRatio("aspectRatio", unchecked()),
      length("length", unchecked()),
      width("width", unchecked())
  {}

  virtual void check(); 

};


class WedgeModule : public virtual Module {
protected:
  float length_, minWidth_, maxWidth_;
  float area_,/*dist_*/;
  bool cropped_;
  float amountCropped_;
public:
  Property<float, NoDefault> buildAperture, buildRadius, cropRadius;
  float length() const { return length_; }
  float minWidth() const { return minWidth_; }
  float maxWidth() const { return maxWidth_; }
//  Shape shape() const { return Shape::WEDGE; }
  
  WedgeModule() {}

};




class BarrelModule : public RectangularModule {
public:
  Property<int, AutoDefault> layer;
  Property<int, AutoDefault> ring;
  Property<int, AutoDefault> rod;
  Property<int, AutoDefault> side;
  BarrelModule* clone() override { return new BarrelModule(*this); }
  void build();
};

class EndcapModule : public virtual Module {
public:
  Property<int, AutoDefault> disk;
  Property<int, AutoDefault> ring;
  Property<int, AutoDefault> blade; // CUIDADO Think of a better name!
  Property<int, AutoDefault> side;
  virtual EndcapModule* clone() = 0;
  void build() = 0;
};

class RectangularEndcapModule : public EndcapModule, public RectangularModule {
public:
  //void check() { RectangularModule::check(); }
  RectangularEndcapModule* clone() override { return new RectangularEndcapModule(*this); }
  void build();
};


class WedgeEndcapModule : public EndcapModule, public WedgeModule {
public:
  //void check() { WedgeModule::check(); }
  WedgeEndcapModule* clone() override { return new WedgeEndcapModule(*this); }
  void build();
};



Module * m = new EndcapM(new RectM(new DualSided(new )));



class PtModule : public M {
public:

}; 



#endif
