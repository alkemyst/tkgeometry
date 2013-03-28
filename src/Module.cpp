#include "Module.h"



void Module::translate(const Translation3D& translation) {
  std::for_each(facePolys_.begin(), facePolys_.end(), bind2nd(mem_fun_ref(&Polygon3d<4>::translate), translation));
}

void Module::rotatePhi(double angle) {
  std::for_each(facePolys_.begin(), facePolys_.end(), bind2nd(mem_fun_ref(&Polygon3d<4>::rotate), Math::RotationZ(angle)));
}

void Module::rotate(const Rotation3D& rotation) {
  std::for_each(facePolys_.begin(), facePolys_.end(), bind2nd(mem_fun_ref(&Polygon3d<4>::rotate), rotation));
}

void Module::transform(const Transform3D& transform) {
  std::for_each(facePolys_.begin(), facePolys_.end(), bind2nd(mem_fun_ref(&Polygon3d<4>::transform), transform));
}


void BarrelModule::build() {

  type_ = ModuleTypeRepo::getInstance().get(moduleType());

  float l = length(), w = width();
  Polygon3d<4> poly(XYZVector(0,  w/2,  l/2) // a BarrelModule is generated lying flat on the YZ plane (like it were on the rod at phi=0)
                   (XYZVector(0, -w/2,  l/2))
                   (XYZVector(0, -w/2, -l/2))
                   (XYZVector(0,  w/2, -l/2)));

  if (numSides() == 1) {
    facePolys_.push_back(poly);
  } else {
    Polygon3d<4> innerPoly(poly), outerPoly(poly);
    innerPoly.translate(XYZVector(-dsDistance()/2, 0, 0);
    outerPoly.translate(XYZVector( dsDistance()/2, 0, 0));
    facePolys_.push_back(innerPoly);
    facePolys_.push_back(outerPoly);
  }
}

void RectangularEndcapModule::build() {

  type_ = ModuleTypeRepo::getInstance().get(moduleType());

  float l = length(), w = width();
  Polygon3d<4> poly(XYZVector( l/2,  w/2, 0) // a RectangularEndcapModule is generated lying flat on the XY plane, with the length along the X axis (like it were on a ring at phi=0)
                   (XYZVector( l/2, -w/2, 0))
                   (XYZVector(-l/2, -w/2, 0))
                   (XYZVector(-l/2,  w/2, 0)));

  if (numSides() == 1) {
    facePolys_.push_back(poly);
  } else {
    Polygon3d<4> innerPoly(poly), outerPoly(poly);
    innerPoly.translate(XYZVector(0, 0, -dsDistance()/2);
    outerPoly.translate(XYZVector(0, 0,  dsDistance()/2));
    facePolys_.push_back(innerPoly);
    facePolys_.push_back(outerPoly);
  }
}

void WedgeEndcapModule::build() {

  type_ = ModuleTypeRepo::getInstance().get(moduleType());


}
