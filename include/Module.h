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
  Property<int, AutoDefault> step; // CUIDADO Think of a better name!
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

#endif
