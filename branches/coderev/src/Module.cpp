#include "Module.h"



void Module::translate(const XYZVector& translation) {
  std::for_each(facePolys_.begin(), facePolys_.end(), std::bind2nd(std::mem_fun_ref(&Polygon3d<4>::translate), translation));
}

void Module::rotatePhi(double angle) {
 // std::for_each(facePolys_.begin(), facePolys_.end(), bind2nd(mem_fun_ref(&Polygon3d<4>::rotate), Math::RotationZ(angle)));
  std::for_each(facePolys_.begin(), facePolys_.end(), std::bind2nd(std::mem_fun_ref(&Polygon3d<4>::rotateZ), angle));
}
/*
void Module::rotate(const Rotation3D& rotation) {
  std::for_each(facePolys_.begin(), facePolys_.end(), bind2nd(mem_fun_ref(&Polygon3d<4>::rotate), rotation));
}

void Module::transform(const Transform3D& transform) {
  std::for_each(facePolys_.begin(), facePolys_.end(), bind2nd(mem_fun_ref(&Polygon3d<4>::transform), transform));
}
*/
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

void BarrelModule::build() {
  check();

  float l = length(), w = width();
  Polygon3d<4> poly; 
  poly << (XYZVector(0,  w/2,  l/2)) // a BarrelModule is generated lying flat on the YZ plane (like it were on the rod at phi=0)
       << (XYZVector(0, -w/2,  l/2))
       << (XYZVector(0, -w/2, -l/2))
       << (XYZVector(0,  w/2, -l/2));

  if (numFaces() == 1) {
    facePolys_.push_back(poly);
  } else {
    Polygon3d<4> innerPoly(poly), outerPoly(poly);
    innerPoly.translate(XYZVector(-dsDistance()/2, 0, 0));
    outerPoly.translate(XYZVector( dsDistance()/2, 0, 0));
    facePolys_.push_back(innerPoly);
    facePolys_.push_back(outerPoly);
  }
}

void RectangularEndcapModule::build() {
  check();

  float l = length(), w = width();
  Polygon3d<4> poly;
  poly << (XYZVector( l/2,  w/2, 0)) // a RectangularEndcapModule is generated lying flat on the XY plane, with the length along the X axis (like it were on a ring at phi=0)
       << (XYZVector( l/2, -w/2, 0))
       << (XYZVector(-l/2, -w/2, 0))
       << (XYZVector(-l/2,  w/2, 0));

  if (numFaces() == 1) {
    facePolys_.push_back(poly);
  } else {
    Polygon3d<4> innerPoly(poly), outerPoly(poly);
    innerPoly.translate(XYZVector(0, 0, -dsDistance()/2));
    outerPoly.translate(XYZVector(0, 0,  dsDistance()/2));
    facePolys_.push_back(innerPoly);
    facePolys_.push_back(outerPoly);
  }
}

void WedgeEndcapModule::build() {
  check();


  //////// BEGIN COPY-PASTE WITH MINIMAL ADJUSTMENTS ////////
  cropped_ = false;

  double r = waferDiameter()/2.;
  double phi = buildAperture()/2.;// We need the half angle covered by the module
  double d = buildRadius();

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
  if (cropRadius.state() && dfar > cropRadius()) {
    amountCropped_ = dfar - cropRadius();
    b1 = 0;
    b2 = cropRadius() - d;
    h2 = h1/d * cropRadius();
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

  Polygon3d<4> poly;
  poly << (XYZVector( length_/2., maxWidth_/2., 0))
       << (XYZVector( length_/2.,-maxWidth_/2., 0))
       << (XYZVector(-length_/2.,-minWidth_/2., 0))
       << (XYZVector(-length_/2., minWidth_/2., 0));

  if (numFaces() == 1) {
    facePolys_.push_back(poly);
  } else {
    Polygon3d<4> innerPoly(poly), outerPoly(poly);
    innerPoly.translate(XYZVector(0, 0, -dsDistance()/2));
    outerPoly.translate(XYZVector(0, 0,  dsDistance()/2));
    facePolys_.push_back(innerPoly);
    facePolys_.push_back(outerPoly);
  }
}

define_enum_strings(Shape) = { "rectangular", "wedge" };
