#include "Module.h"

double ModuleHelpers::polygonAperture(const Polygon3d<4>& poly) { 
  auto minmax = std::minmax_element(poly.begin(), poly.end(), [](const XYZVector& v1, const XYZVector& v2) { return v1.Phi() < v2.Phi(); }); 
  return minmax.second->Phi() - minmax.first->Phi(); 
}



void RectangularModule::check() {
  Module::check();

  if (length.state() && width.state()) {
    aspectRatio(length()/width());
  } else if (!length.state() && width.state() && aspectRatio.state()) { 
    length(width() * aspectRatio());
  } else if (length.state() && !width.state() && aspectRatio.state()) {
    width(length() / aspectRatio());
  } else if (!length.state() && !width.state() && aspectRatio.state()) {
    length(waferDiameter() * sin(atan(aspectRatio())));
    width(waferDiameter() * cos(atan(aspectRatio())));
  } else {
    throw PathfulException("Module geometry is inconsistently specified");
  }
}


void RectangularModule::build() {
  try { 
    check();

    float l = length(), w = width();
    basePoly_ << XYZVector( l/2, w/2, 0) 
              << XYZVector(-l/2, w/2, 0)
              << XYZVector(-l/2,-w/2, 0)
              << XYZVector( l/2,-w/2, 0);
    builtok(true);
  }
  catch (PathfulException& pe) { pe.pushPath(*this, myid()); throw; }
}



void WedgeModule::build() {
  try {
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
    if (buildCropDistance.state() && dfar > buildCropDistance()) {
      amountCropped_ = dfar - buildCropDistance();
      b1 = 0;
      b2 = buildCropDistance() - d;
      h2 = h1/d * buildCropDistance();
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

    basePoly_ << (XYZVector( length_/2., maxWidth_/2., 0))
              << (XYZVector( length_/2.,-maxWidth_/2., 0))
              << (XYZVector(-length_/2.,-minWidth_/2., 0))
              << (XYZVector(-length_/2., minWidth_/2., 0));
  }
  catch (PathfulException& pe) { pe.pushPath(*this, myid()); throw; }
  cleanup();
  builtok(true);
}



void BarrelModule::build() {
  try {
    check();
    if (!decorated().builtok()) {
      decorated().store(propertyTree());
      decorated().build();
    }
    decorated().rotateY(M_PI/2);
//    sideDisplacement_[SideZ::MINZ] = decorated().center().Z() - minZ();
//    sideDisplacement_[SideZ::MAXZ] = maxZ() - decorated().center().Z();
  }
  catch (PathfulException& pe) { pe.pushPath(*this, myid()); throw; }
  cleanup();
  builtok(true);
}

void EndcapModule::build() {
  try {
    check();
    if (!decorated().builtok()) {
      decorated().store(propertyTree());
      decorated().build();
    }
  }
  catch (PathfulException& pe) { pe.pushPath(*this, myid()); throw; }
  cleanup();
  builtok(true);
}


void SingleSensorModule::build() {
  try {
    check();
    if (!decorated().builtok()) {
      decorated().store(propertyTree());
      decorated().build();
    }
    sensor_.store(propertyTree());
    if (sensorNode.count(1) > 0) sensor_.store(sensorNode.at(1));
    if (!sensorNode.empty()) sensor_.store(sensorNode.begin()->second);  
    sensor_.poly() = basePoly();
    sensor_.build();
  }
  catch (PathfulException& pe) { pe.pushPath(*this, myid()); throw; }
  cleanup();
  builtok(true);
}

void DualSensorModule::build() {
  try {
    check();
    if (!decorated().builtok()) {
      decorated().store(propertyTree());
      decorated().build();
    }
    sensors_[0].store(propertyTree());
    if (sensorNode.count(1) > 0) sensors_[0].store(sensorNode.at(1));
    (sensors_[0].poly() = basePoly()).translate(-normal()*dsDistance()/2);
    sensors_[0].build();
    sensors_[1].store(propertyTree());
    if (sensorNode.count(2) > 0) sensors_[1].store(sensorNode.at(2));
    (sensors_[1].poly() = basePoly()).translate(normal()*dsDistance()/2);
    sensors_[1].build();
  }
  catch (PathfulException& pe) { pe.pushPath(*this, myid()); throw; }
  cleanup();
  builtok(true);
}

/*
TypedModule* TypedModule::decorate(ModuleType type, Module* m) {
  if (type == ModuleType::SINGLE_SENSOR) { return new SingleSensorModule(m); }
  else { return new DualSensorModule(m); }
}
*/
Module* TypedModule::decorate(const string& type, Module* m) {
  using namespace boost::property_tree;
  if (type.empty()) return m;
  ptree pt;
  Module* tmod = m;
  try {
    info_parser::read_info(join<string>({Paths::stdconfig, Paths::moduleTypes, type}, Paths::sep), pt);
    int ns = pt.get<int>("numSensors", 0);
    if (ns == 1) tmod = new SingleSensorModule(m);
    else if (ns == 2) tmod = new DualSensorModule(m);
    else throw InvalidPropertyValue("numSensors", any2str(ns));
  } 
  catch(info_parser::info_parser_error& e) { throw InvalidPropertyValue("moduleType", type); }
  catch(ptree_bad_path& e) { throw CheckedPropertyMissing("numSensors"); }

  tmod->store(pt);

  return tmod;
}

define_enum_strings(ModuleShape) = { "rectangular", "wedge" };
//define_enum_strings(ModuleType) = { "singlesensor", "dualsensor" };
