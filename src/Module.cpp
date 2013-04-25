#include "Module.h"





void RectangularModule::check() {
  Module::check();

  if (!length.state() && width.state() && aspectRatio.state()) { 
    length(width() * aspectRatio());
  } else if (length.state() && !width.state() && aspectRatio.state()) {
    width(length() / aspectRatio());
  } else if (!length.state() && !width.state() && aspectRatio.state()) {
    length(waferDiameter() * sin(atan(aspectRatio())));
    width(waferDiameter() * cos(atan(aspectRatio())));
  } else {
    throw PathfulException("Module geometry is inconsistently specified", fullid());
  }
}


void RectangularModule::build() {
  check();

  float l = length(), w = width();
  basePoly() << XYZVector( w/2, l/2, 0) 
             << XYZVector(-w/2, l/2, 0)
             << XYZVector(-w/2,-l/2, 0)
             << XYZVector( w/2,-l/2, 0);

}



void WedgeModule::build() {
  check();

  //////// BEGIN COPY-PASTE WITH MINIMAL ADJUSTMENTS ////////
  cropped_ = false;

  double r = waferDiameter()/2.;
  double phi = buildAperture()/2.;// We need the half angle covered by the module
  double d = buildDistance();

  //double gamma1; // alternate
  double gamma2;
  double h1, h2, b1, b2, dfar, l;
  h1 = d * tan(phi);// The short (half)base
  // Distance of short base from the wafer center
  b1 = sqrt(pow(r, 2)-pow(h1, 2)); // main + alternate

  // y coordinate of the wafer center
  l = b1 + d; // main

  // Distance of the far angle form the z axis
  gamma2 = l*cos(phi) + sqrt(pow(r, 2)-pow(l*sin(phi), 2)); // main

  h2 = gamma2 * sin(phi);// The long (half)base
  dfar = gamma2 * cos(phi);// Distance of long base from the z axis

  // The distance of the long base from the wafer center
  //b2 =  sqrt(pow(r,2)-pow(h2,2)); // old
  b2 = dfar - d - b1;

  // NOTE: in principle we don't need to compute b2 to get the
  // module's corner coordinates. Still we use this way of computing
  // b2 to ease the computation of the module's area

  // Add a check: if the module overcomes the max rho
  // it must be cut.
  if (cropDistance.state() && dfar > cropDistance()) {
    amountCropped_ = dfar - cropDistance();
    b1 = 0;
    b2 = cropDistance() - d;
    h2 = h1/d * cropDistance();
    cropped_ = true;
  }

  // Some member variable computing:
  area_     = fabs((b1+b2) * (h2+h1));
  length_   = (b1 + b2);
  //phiWidth_ = 2*phi;
  minWidth_  = 2 * h1;
  maxWidth_  = 2 * h2;
  //dist_     = d;
  //aspectRatio_ = length_/(h1+h2);

  basePoly() << (XYZVector( length_/2., maxWidth_/2., 0))
             << (XYZVector( length_/2.,-maxWidth_/2., 0))
             << (XYZVector(-length_/2.,-minWidth_/2., 0))
             << (XYZVector(-length_/2., minWidth_/2., 0));
}



void BarrelModule::build() {
  check();
  decorated().store(propertyTree());
  decorated().build();
  decorated().rotateY(M_PI/2);
}

void EndcapModule::build() {
  check();
  decorated().store(propertyTree());
  decorated().build();
}


void SingleSensorModule::build() {
  check();
  decorated().store(propertyTree());
  decorated().build();
  auto& children = propertyTree().getChildren("Sensor");
  sensor_.store(!children.empty() ? children.front() : propertyTree());  
  sensor_.poly(basePoly());
  sensor_.build();
}

void DualSensorModule::build() {
  check();
  decorated().store(propertyTree());
  decorated().build();
  auto& childmap = propertyTree().getChildmap<int>("Sensor");
  sensors_[0].store(childmap.count(1) > 0 ? childmap.at(1) : propertytree());
  sensors_[0].poly((Polygon3d<4>(basepoly()).translate(-basePoly().normal()*dsDistance()/2));
  sensors_[0].build();
  sensors_[1].store(childmap.count(2) > 0 ? childmap.at(2) : propertytree());
  sensors_[1].poly((Polygon3d<4>(basepoly()).translate(basePoly().normal()*dsDistance()/2));
  sensors_[1].build();
}


TypedModule* TypedModule::decorate(const Module* m, ModuleType type) {
  if (type == ModuleType::SINGLE_SENSOR) { return new SingleSensorModule(m); }
  else { return new DualSensorModule(m); }
}

define_enum_strings(ModuleShape) = { "rectangular", "wedge" };
define_enum_strings(ModuleType) = { "singlesensor", "dualsensor" };
