#include "Layer.h"
#include "RodPair.h"


double Layer::calculatePlaceRadius(int numRods,
                                   double bigDelta,
                                   double smallDelta,
                                   double dsDistance,
                                   double moduleWidth,
                                   double overlap) {

  double d = dsDistance/2;

  double f = (moduleWidth/2) - (overlap/2);

  double R = bigDelta + smallDelta + d;
  double S = -bigDelta + smallDelta + d;

  double T = tan(2*M_PI/numRods);

  double a = T;
  double b = R*T + S*T - 2*f;
  double c = R*S*T - R*f - S*f - T*f*f;

  double r = (-b + sqrt(b*b - 4*a*c))/(2*a);

  return r;
}

std::pair<float, int> Layer::calculateOptimalLayerParms(const RodTemplate& rodTemplate) {
                                                              
  // CUIDADO fix placeRadiusHint!!!!!
  double maxDsDistance = (*std::max_element(rodTemplate.begin(), 
                                            rodTemplate.end(), 
                                            [](shared_ptr<BarrelModule> m1, shared_ptr<BarrelModule> m2) { return m1->dsDistance() > m2->dsDistance(); } ))->dsDistance();
  float moduleWidth = (*rodTemplate.rbegin())->width();
  float f = moduleWidth/2 - rodOverlap()/2;
  float gamma = atan(f/placeRadiusHint() + bigDelta() + smallDelta() + maxDsDistance/2) + atan(f/(placeRadiusHint() - bigDelta() + smallDelta() + maxDsDistance/2));
  float tentativeSlices = M_2_PI/(gamma * modsPerSlice());

  float optimalRadius;
  int optimalSlices;

  switch (radiusMode()) {
  case SHRINK:
    optimalSlices = floor(tentativeSlices);
    optimalRadius = calculatePlaceRadius(optimalSlices*modsPerSlice(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, rodOverlap());
    break;
  case ENLARGE:
    optimalSlices = ceil(tentativeSlices);
    optimalRadius = calculatePlaceRadius(optimalSlices*modsPerSlice(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, rodOverlap());
    break;
  case FIXED:
    optimalSlices = ceil(tentativeSlices);
    optimalRadius = placeRadiusHint();
    break;
  case AUTO:
    int slicesLo = floor(tentativeSlices);
    int slicesHi = ceil(tentativeSlices);
    float radiusLo = calculatePlaceRadius(slicesLo*modsPerSlice(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, rodOverlap());
    float radiusHi = calculatePlaceRadius(slicesHi*modsPerSlice(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, rodOverlap());

    if (fabs(radiusHi - placeRadiusHint()) < fabs(radiusLo - placeRadiusHint())) {
      optimalRadius = radiusHi;
      optimalSlices = slicesHi;
    } else {
      optimalRadius = radiusLo;
      optimalSlices = slicesLo;
    }
    break;
  }

  return std::make_pair(optimalRadius, optimalSlices*modsPerSlice());
}


RodTemplate Layer::makeRodTemplate(const PropertyTree& pt) {
  RodTemplate rodTemplate(numModules.state() ? numModules() : 0);
  for (auto& childTree : pt.getChildren("Module")) {
    int ring = childTree.getValue<int>(0);
    if (ring > 0) {
      if (rodTemplate.size() < ring) rodTemplate.resize(ring);
      TypedModule* tm = ModuleTypeRepo::getInstance().decorateModule(new RectangularModule(), moduleType());
      (rodTemplate[ring-1] = std::make_shared<BarrelModule>(tm))->store(childTree);  // decorator chain: new Barrel(new Typed(new Rectangular()))
    }
  }
  if (!numModules.state()) rodTemplate.push_back(NULL);// additional module built with default inherited properties to use in case the maxZ placement strategy requires additional modules than the ones constructed from mod-specific props
  for (auto& m : rodTemplate) {
    if (m == NULL) {
      TypedModule* tm = ModuleTypeRepo::getInstance().decorateModule(new RectangularModule(), moduleType());
      m = std::make_shared<BarrelModule>(tm);  // decorator chain: new Barrel(new Typed(new Rectangular()))
      m->store(propertyTree());
    }
    m->build();
  }
  return rodTemplate;
}

void Layer::build() {
  try { 
    check();

    RodTemplate rodTemplate = makeRodTemplate(propertyTree());

    std::pair<double, int> optimalLayerParms = calculateOptimalLayerParms(rodTemplate);
    placeRadius_ = optimalLayerParms.first; 
    numRods_ = optimalLayerParms.second;
    if (!minBuildRadius.state() || !maxBuildRadius.state()) {
      minBuildRadius(placeRadius_);
      maxBuildRadius(placeRadius_);
    }

    float rodPhiRotation = M_2_PI/numRods_;

    RodPair* first = new RodPair();
    first->myid(1);
    first->minBuildRadius(minBuildRadius()-bigDelta());
    first->maxBuildRadius(maxBuildRadius()+bigDelta());
    first->smallDelta(smallDelta());
    first->store(propertyTree());

    RodPair* second = new RodPair(*first);
    second->myid(2);
    if (!sameParityRods()) second->zPlusParity(second->zPlusParity()*-1);

    first->build(rodTemplate);
    first->translate(XYZVector(placeRadius_+bigDelta(), 0, 0));
    rods_.push_back(first);

    second->build(rodTemplate);
    second->translate(XYZVector(placeRadius_-bigDelta(), 0, 0));
    second->rotatePhi(rodPhiRotation);
    rods_.push_back(second);

    for (int i = 2; i < numRods_; i++) {
      RodPair* rod = new RodPair(i%2 ? *second : *first); // clone rods
      rod->rotatePhi(rodPhiRotation*i);
      rod->myid(i+1);
      rods_.push_back(rod);
    }

  } catch (PathfulException& pe) { 
    pe.pushPath(fullid()); 
    throw; 
  }
}


define_enum_strings(Layer::RadiusMode) = { "shrink", "enlarge", "fixed", "auto" };
