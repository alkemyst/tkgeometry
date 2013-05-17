#include "Layer.h"
#include "RodPair.h"

#include <iostream>

void Layer::check() {
  PropertyObject::check();

  if (numModules.state() && maxZ.state()) throw PathfulException("Only one between numModules and maxZ can be specified");
  else if (!numModules.state() && !maxZ.state()) throw PathfulException("At least one between numModules and maxZ must be specified");

}


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
                                            [](const unique_ptr<RectangularModule>& m1, const unique_ptr<RectangularModule>& m2) { return m1->dsDistance() > m2->dsDistance(); } ))->dsDistance();
  float moduleWidth = (*rodTemplate.rbegin())->width();
  float f = moduleWidth/2 - rodOverlapPhi()/2;
  float gamma = atan(f/(placeRadiusHint() + bigDelta() + smallDelta() + maxDsDistance/2)) + atan(f/(placeRadiusHint() - bigDelta() + smallDelta() + maxDsDistance/2));
  float tentativeModsPerSegment = 2*M_PI/(gamma * phiSegments());

  float optimalRadius;
  int optimalModsPerSegment;

  switch (radiusMode()) {
  case SHRINK:
    optimalModsPerSegment = floor(tentativeModsPerSegment);
    optimalRadius = calculatePlaceRadius(optimalModsPerSegment*phiSegments(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, rodOverlapPhi());
    break;
  case ENLARGE:
    optimalModsPerSegment = ceil(tentativeModsPerSegment);
    optimalRadius = calculatePlaceRadius(optimalModsPerSegment*phiSegments(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, rodOverlapPhi());
    break;
  case FIXED:
    optimalModsPerSegment = ceil(tentativeModsPerSegment);
    optimalRadius = placeRadiusHint();
    break;
  case AUTO: {
    int modsPerSegLo = floor(tentativeModsPerSegment);
    int modsPerSegHi = ceil(tentativeModsPerSegment);
    float radiusLo = calculatePlaceRadius(modsPerSegLo*phiSegments(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, rodOverlapPhi());
    float radiusHi = calculatePlaceRadius(modsPerSegHi*phiSegments(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, rodOverlapPhi());

    if (fabs(radiusHi - placeRadiusHint()) < fabs(radiusLo - placeRadiusHint())) {
      optimalRadius = radiusHi;
      optimalModsPerSegment = modsPerSegHi;
    } else {
      optimalRadius = radiusLo;
      optimalModsPerSegment = modsPerSegLo;
    }
    break;
             }
  default:
    throw PathfulException("Invalid value for enum radiusMode");
  }

  return std::make_pair(optimalRadius, optimalModsPerSegment*phiSegments());
}



RodTemplate Layer::makeRodTemplate() {
  RodTemplate rodTemplate(numModules.state() ? numModules() : (!ringNode.empty() ? ringNode.rbegin()->first + 1 : 1)); // + 1 to make room for a default constructed module to use when building rods in case the rodTemplate vector doesn't have enough elements
  for (int i = 0; i < rodTemplate.size(); i++) {
    rodTemplate[i] = std::move(unique_ptr<RectangularModule>(new RectangularModule()));
    rodTemplate[i]->store(propertyTree());
    if (ringNode.count(i+1) > 0) rodTemplate[i]->store(ringNode.at(i+1));
    rodTemplate[i]->build();
  }
  return rodTemplate;
}


void Layer::build() {
  try { 
    std::cout << ">>> Building " << fullid() << " <<<" << std::endl;
    check();

    RodTemplate rodTemplate = makeRodTemplate();

    std::pair<double, int> optimalLayerParms = calculateOptimalLayerParms(rodTemplate);
    placeRadius_ = optimalLayerParms.first; 
    numRods_ = optimalLayerParms.second;
    if (!minBuildRadius.state() || !maxBuildRadius.state()) {
      minBuildRadius(placeRadius_);
      maxBuildRadius(placeRadius_);
    }

    float rodPhiRotation = 2*M_PI/numRods_;

    RodPair* first = new RodPair();
    first->myid(1);
    first->minBuildRadius(minBuildRadius()-bigDelta());
    first->maxBuildRadius(maxBuildRadius()+bigDelta());
    if (numModules.state()) first->numModules(numModules());
    else if (maxZ.state()) first->maxZ(maxZ());
    first->smallDelta(smallDelta());
    first->ringNode = ringNode; // we need to pass on the contents of the ringNode to allow the RodPair to build the module decorators
    first->store(propertyTree());
    first->build(rodTemplate);

    std::cout << ">>> Copying rod " << first->fullid() << " <<<" << std::endl;
    RodPair* second = new RodPair(*first);
    second->myid(2);
    if (!sameParityRods()) second->zPlusParity(first->zPlusParity()*-1);

    first->translateR(placeRadius_ + bigDelta());
    //first->translate(XYZVector(placeRadius_+bigDelta(), 0, 0));
    rods_.push_back(first);

    second->translateR(placeRadius_ - bigDelta());
    //second->translate(XYZVector(placeRadius_-bigDelta(), 0, 0));
    second->rotateZ(rodPhiRotation);
    rods_.push_back(second);

    for (int i = 2; i < numRods_; i++) {
      RodPair* rod = new RodPair(i%2 ? *second : *first); // clone rods
      rod->rotateZ(rodPhiRotation*i);
      rod->myid(i+1);
      rods_.push_back(rod);
    }

  } catch (PathfulException& pe) { 
    pe.pushPath(fullid()); 
    throw; 
  }

  cleanup();
  builtok(true);
}


define_enum_strings(Layer::RadiusMode) = { "shrink", "enlarge", "fixed", "auto" };
