#include "Ring.h"


inline void Ring::check() {
  PropertyObject::check();
  if (moduleShape() == ModuleShape::WEDGE && buildDirection() == TOPDOWN) throw PathfulException("Wedge-module rings cannot be built top down");
}

inline double Ring::solvex(double y) {
  return(1/8.*(3*y+2-sqrt(9*pow(y, 2)-4*y+4)));
}

inline double Ring::compute_l(double x, double y, double d) {
  double result = d;
  result /= (1-x)-sqrt((1-x)*(y-x));
  return result;
}

inline double Ring::compute_d(double x, double y, double l) {
  double result = l;
  result *= (1-x)-sqrt((1-x)*(y-x));
  return result;
}


void Ring::cutAtEta(double eta) { modules_.erase_if([eta](EndcapModule& m) { return fabs(m.center().Eta()) > eta; }); }

double Ring::computeTentativePhiAperture(double moduleWaferDiameter, double minRadius) {
  double r = moduleWaferDiameter/2;

  double l = (minRadius-r);
  double y = pow(r/l, 2);
  double x = solvex(y);


  double tempd;

  int i = 0;
  for (; i < MAX_WEDGE_CALC_LOOPS; i++) {
    l = compute_l(x, y, minRadius);
    y = pow(r/l, 2);
    x = solvex(y);

    tempd = compute_d(x, y, l);

    if (fabs(minRadius - tempd)<1e-15) break;
  }

  if (i >= MAX_WEDGE_CALC_LOOPS) {
    //logWarning("Maximum number of iterations hit while computing wedge geometry");
  }

  double alpha = asin(sqrt(x)) * 2;

  return alpha;
}

std::pair<double, int> Ring::computeOptimalRingParametersWedge(double moduleWaferDiameter, double minRadius) {
  double delta = moduleOverlapPhi()/minRadius;// SM: The needed overlap becomes an angle delta by
  //     checking the unsafest point (r=r_min)

  double tentativeAlpha = computeTentativePhiAperture(moduleWaferDiameter, minRadius) - delta;
  float tentativeNumMods = 2*M_PI / tentativeAlpha; 
  int optimalNumMods = (!requireOddModsPerSlice()? round(tentativeNumMods/phiSegments()) : roundToOdd(tentativeNumMods/phiSegments()) + additionalModules()) * phiSegments();
  float optimalAlpha = 2*M_PI/optimalNumMods + delta; // CUIDADO check this!

  return std::make_pair(optimalAlpha, optimalNumMods);
}

std::pair<double, int> Ring::computeOptimalRingParametersRectangle(double moduleWidth, double maxRadius) {
  double delta = moduleOverlapPhi()/maxRadius;
  double optimalAlpha = 2*asin(moduleWidth/2. / maxRadius) - delta;
  double tentativeNumMods = 2*M_PI/optimalAlpha;
  int modsPerSlice = ceil(tentativeNumMods/phiSegments());
  if ((modsPerSlice % 2) == 0 && requireOddModsPerSlice()) modsPerSlice++;
  int optimalNumMods = modsPerSlice * phiSegments();

  return std::make_pair(optimalAlpha, optimalNumMods);
}


void Ring::buildModules(EndcapModule* templ, int numMods, double smallDelta) {
  double alignmentRotation = alignEdges() ? 0.5 : 0.; // CUIDADO ask stefano what this is about
  for (int i = 0, parity = smallParity(); i < numMods; i++, parity *= -1) {
    EndcapModule* mod = templ->clone();
    mod->setup();
    //mod->blade(i+1);
    mod->myid(i+1);
    mod->rotateZ(2*M_PI*(i+alignmentRotation)/numMods); // CUIDADO had a rotation offset of PI/2
    mod->translateZ(parity*smallDelta); 

    //XYZVector shift = XYZVector(0, 0, diskZ + nearDirection*ringParity*smallDelta + nearDirection*bigDelta);
    //myModule->translate(shift);
    modules_.push_back(mod);  
  }
}


void Ring::buildBottomUp() {
  buildStartRadius(buildStartRadius()+ringGap());
  int numMods;
  double modLength;

  EndcapModule* emod = nullptr;
  if (moduleShape() == ModuleShape::WEDGE) {
    WedgeModule* wmod = new WedgeModule();
    wmod->store(propertyTree());

    auto optimalRingParms = computeOptimalRingParametersWedge(wmod->waferDiameter(), buildStartRadius());
    double alpha = optimalRingParms.first;
    numMods = optimalRingParms.second;

    wmod->buildAperture(alpha);
    wmod->buildDistance(buildStartRadius());
    wmod->buildCropDistance(buildCropRadius());

    wmod->build();

    modLength = wmod->length();

    emod = new EndcapModule(wmod);

  } else {

    RectangularModule* rmod = new RectangularModule();
    rmod->store(propertyTree());
    rmod->build();

    auto optimalRingParms = computeOptimalRingParametersRectangle(rmod->width(), buildStartRadius() + rmod->length());
    numMods = optimalRingParms.second;

    modLength = rmod->length();

    emod = new EndcapModule(rmod); 

  }

  emod->setup();
  emod->store(propertyTree());
  emod->build();
  emod->translate(XYZVector(buildStartRadius() + modLength/2, 0, 0)); // CUIDADO it should use translateR

  minRadius_ = buildStartRadius();
  maxRadius_ = buildStartRadius() + modLength;

  buildModules(emod, numMods, smallDelta());

  delete emod;
}


void Ring::buildTopDown() {
  buildStartRadius(buildStartRadius()-ringGap());

  RectangularModule* rmod = new RectangularModule();
  rmod->setup();
  rmod->store(propertyTree());

  auto optimalRingParms = computeOptimalRingParametersRectangle(rmod->width(), buildStartRadius());
  int numMods = optimalRingParms.second;

  EndcapModule* emod = new EndcapModule(rmod); // decorator chain: new Endcap(new Wedge()))
  emod->setup();
  emod->store(propertyTree());
  emod->build();
  emod->translate(XYZVector(buildStartRadius() - rmod->length()/2, 0, 0)); // CUIDADO it should use translateR

  minRadius_ = buildStartRadius() - rmod->length();
  maxRadius_ = buildStartRadius();

  buildModules(emod, numMods, smallDelta());

  delete emod;
}


void Ring::build() {
  try {
    std::cout << ">>> Building " << fullid(*this) << " <<<" << std::endl;
    check();
    if (buildDirection() == BOTTOMUP) buildBottomUp();
    else buildTopDown();
  } catch (PathfulException& pe) { pe.pushPath(fullid(*this)); throw; }

  cleanup();
  builtok(true);
}

void Ring::translateZ(double z) {
  for (auto& m : modules_) {
    m.translateZ(z);
  }
}

void Ring::mirrorZ() {
  for (auto& m : modules_) {
    m.mirrorZ();
  }
}

define_enum_strings(Ring::BuildDirection) = { "topdown", "bottomup" };
