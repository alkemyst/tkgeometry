#include "RodPair.h"

void RodPair::clearComputables() { 
  minAperture.clear(); 
  maxAperture.clear(); 
}

void RodPair::translate(const XYZVector& translation) {
  for (auto& m : zPlusModules_) { m.translate(translation); }
  for (auto& m : zMinusModules_) { m.translate(translation); }
  clearComputables();
}

void RodPair::translateR(double radius) {
  for (auto& m : zPlusModules_) { m.translateR(radius); }
  for (auto& m : zMinusModules_) { m.translateR(radius); }
  clearComputables();
}

void RodPair::rotateZ(double angle) {
  for (auto& m : zPlusModules_) { m.rotateZ(angle); }
  for (auto& m : zMinusModules_) { m.rotateZ(angle); }
  clearComputables();
}

void RodPair::cutAtEta(double eta) { 
  zPlusModules_.erase_if([eta](BarrelModule& m) { return fabs(m.center().Eta()) > eta; }); 
  zMinusModules_.erase_if([eta](BarrelModule& m) { return fabs(m.center().Eta()) > eta; }); 
}


void StraightRodPair::compressToZ(double z) {

  if (zPlusModules_.empty()) return;

  const BarrelModule& maxBarrelModule = *std::max_element(zPlusModules_.begin(), zPlusModules_.end(), [](const BarrelModule& m1, const BarrelModule& m2) { return m1.maxZ() < m2.maxZ(); });
  const BarrelModule& minBarrelModule = *std::min_element(zMinusModules_.begin(), zMinusModules_.end(), [](const BarrelModule& m1, const BarrelModule& m2) { return m1.minZ() < m2.minZ(); });

  double maxZ = fabs(maxBarrelModule.maxZ());
  double minZ = fabs(minBarrelModule.minZ());

  double Deltap =  fabs(z) - maxZ;
  double Deltam = -fabs(z) + minZ;

  logINFO("Rod length cannot exceed " + any2str(z));
  logINFO("  zPlus rod will be compressed by " + any2str(Deltap));
  logINFO("  zMinus rod will be compressed by " + any2str(Deltam));

  double deltam=0;
  double deltap=0;
  XYZVector modShift;
  deltam=Deltam/((minBarrelModule.center()).Z());
  deltap=Deltap/((maxBarrelModule.center()).Z());
  for (auto& m : zPlusModules_) {
    double myMeanZ = m.center().Z();
    m.translateZ(myMeanZ > 0 ? deltap*myMeanZ : deltam*myMeanZ);
  }
  for (auto& m : zMinusModules_) {
    double myMeanZ = m.center().Z();
    m.translateZ(myMeanZ > 0 ? deltap*myMeanZ : deltam*myMeanZ);
  }

}

bool StraightRodPair::solveCollisionsZPlus() {
  std::map<int, double> zGuards;
  zGuards[1] = zGuards[-1] = std::numeric_limits<double>::lowest();
  int parity = zPlusParity();
  int n = 1;
  bool found = false;
  logINFO("Checking for collisions in Z+ rod");
  for (auto& m : zPlusModules_) {
    double minPhysZ = m.center().Z() - MAX(m.physicalLength(), m.length())/2;
    if (zGuards[parity] > minPhysZ) {
      double offset = zGuards[parity] - minPhysZ;
      m.translateZ(offset);
      logINFO("  Module " + any2str(n) + " collides with previous " + (parity > 0 ? "outer" : "inner") + " module. Translated by " + any2str(offset) + " mm");
      found = true;
    }
    zGuards[parity] = m.center().Z() + MAX(m.physicalLength(), m.length())/2;
    parity = -parity;
    n++;
  }
  return found;
}

bool StraightRodPair::solveCollisionsZMinus() {
  std::map<int, double> zGuards;
  zGuards[1] = zGuards[-1] = std::numeric_limits<double>::max();
  int parity = -zPlusParity();
  int n = -1;
  bool found = false;
  logINFO("Checking for collisions in Z- rod");
  for (auto& m : zMinusModules_) {
    double maxPhysZ = m.center().Z() + MAX(m.physicalLength(), m.length())/2;
    if (zGuards[parity] < maxPhysZ) {
      double offset = zGuards[parity] - maxPhysZ;
      m.translateZ(offset);
      logINFO("  Module " + any2str(n) + " collides with previous " + (parity > 0 ? "outer" : "inner") + " module. Translated by " + any2str(offset) + " mm");
    }
    zGuards[parity] = m.center().Z() - MAX(m.physicalLength(), m.length())/2;
    parity = -parity;
    n--;
  }
  return found;
}


double StraightRodPair::computeNextZ(double newDsDistance, double lastDsDistance, double lastZ, BuildDir direction, int parity) {
  double d = smallDelta();
  double dz = zError();
  double ov = minModuleOverlap();
  double maxr = maxBuildRadius();
  double minr = minBuildRadius();

  double newR = (parity > 0 ? maxr + d : minr - d) - newDsDistance/2;
  double lastR = (parity > 0 ? maxr - d : minr + d) + lastDsDistance/2;

  double newZ = lastZ;

  if (direction == BuildDir::RIGHT) {
    double originZ = parity > 0 ? dz : -dz;
    double newZorigin = (newZ - ov) * newR/lastR;
    double newZshifted = (newZ - originZ) * newR/lastR + originZ;
    newZ = MIN(newZorigin, newZshifted);
  } else {
    double originZ = parity > 0 ? -dz : dz;
    double newZorigin = (newZ + ov) * newR/lastR;
    double newZshifted = (newZ - originZ) * newR/lastR + originZ;
    newZ = MAX(newZorigin, newZshifted);
  }

  return newZ;
}



template<typename Iterator> vector<double> StraightRodPair::computeZList(Iterator begin, Iterator end, double startZ, BuildDir direction, int smallParity, bool fixedStartZ) {

  vector<double> zList;

  double targetZ = maxZ.state() ? maxZ() : std::numeric_limits<double>::max(); // unreachable target in case maxZ not set
  int targetMods = buildNumModules.state() ? buildNumModules() : std::numeric_limits<int>::max(); // unreachable target in case numModules not set

  //std::map<int, double> zGuards; // avoids clashes between modules at the same R (inner-inner or outer-outer). the guards provide a minZ modules have to be placed at. implemented as a map with two keys (1, -1) accessed with the parity
  //zGuards[1] = zGuards[-1] = (direction == BuildDir::RIGHT ? std::numeric_limits<double>::min() : std::numeric_limits<double>::max()); // guards initialized to default values that don't get in the way of the algorithm later

  double newZ = startZ; // + lengthOffset/2;

  int parity = smallParity;
  BarrelModule* lastm = begin->get();

  int n = 0;

  auto solveCollisions = [&n](const Module& templ, double newZ, std::map<int, double>& zGuards, BuildDir direction, int parity) {
    double lengthOffset = templ.length();
    double physicalLengthOffset = templ.physicalLength();
    if (direction == BuildDir::RIGHT) {
      if (zGuards[parity] > newZ) std::cout << "I will have to shift module " << n << " right by " << zGuards[parity] - newZ << std::endl;
      newZ = MAX(zGuards[parity], newZ);
      zGuards[parity] = newZ + MAX(physicalLengthOffset, lengthOffset);
    } else {
      if (zGuards[parity] < newZ) std::cout << "I will have to shift module " << n << " left by " << newZ - zGuards[parity] << std::endl;
      newZ = MIN(newZ, zGuards[parity]);
      zGuards[parity] = newZ - MIN(physicalLengthOffset, lengthOffset);
    }
    return newZ;
  };


  if (fixedStartZ) {
    //zGuards[parity] = newZ + (direction == BuildDir::RIGHT ? MAX((*begin)->physicalLength(), (*begin)->length()) : -MIN((*begin)->physicalLength(), (*begin)->length()));
    zList.push_back(newZ);
    newZ += (direction == BuildDir::RIGHT ? (*begin)->length() : (*begin)->length());
    parity = -parity;
    ++begin;
    n = 1;
  }

  for (auto& curm : pair2range(make_pair(begin, end))) {
    if (abs(newZ) >= targetZ || n++ >= targetMods) break;
    newZ = computeNextZ(curm->dsDistance(), lastm->dsDistance(), newZ, direction, parity);
    //newZ = solveCollisions(*curm, newZ, zGuards, direction, parity);
    zList.push_back(newZ);
    newZ += (direction == BuildDir::RIGHT ? curm->length() : -curm->length());
    lastm = curm.get();
    parity = -parity;
  } 

  double lengthOffset = lastm->length();
  double physicalLengthOffset = lastm->physicalLength();

  for (; abs(newZ) < targetZ && n < targetMods; n++) {  // in case the rodtemplate vector is finished but we haven't hit the targetZ or targetMods yet, we keep on using the last module for dsDistance and length
    newZ = computeNextZ(lastm->dsDistance(), lastm->dsDistance(), newZ, direction, parity);
    //newZ = solveCollisions(*lastm, newZ, zGuards, direction, parity);
    zList.push_back(newZ);
    newZ += (direction == BuildDir::RIGHT ? lastm->length() : -lastm->length());
    parity = -parity;
  }

  return zList;

}


template<typename Iterator> pair<vector<double>, vector<double>> StraightRodPair::computeZListPair(Iterator begin, Iterator end, double startZ, int recursionCounter) {

  bool fixedStartZ = true;
  vector<double> zPlusList = computeZList(begin, end, startZ, BuildDir::RIGHT, zPlusParity(), fixedStartZ);
  vector<double> zMinusList = computeZList(begin, end, startZ, BuildDir::LEFT, -zPlusParity(), !fixedStartZ);

  double zUnbalance = (zPlusList.back()+(*(end-1))->length()/2) + (zMinusList.back()-(*(end-1))->length()/2); // balancing uneven pos/neg strings

  if (++recursionCounter == 100) { // this stops infinite recursion if the balancing doesn't converge
    std::ostringstream tempSS;
    tempSS << "Balanced module placement in rod pair at avg build radius " << (maxBuildRadius()+minBuildRadius())/2. << " didn't converge!! Layer is skewed";
    tempSS << "Unbalance is " << zUnbalance << " mm";
    logWARNING(tempSS);

    return std::make_pair(zPlusList, zMinusList);
  }  

  if (fabs(zUnbalance) > 0.1) { // 0.1 mm unbalance is tolerated
    return computeZListPair(begin, end,
                            startZ-zUnbalance/2, // countering the unbalance by displacing the startZ (by half the inverse unbalance, to improve convergence)
                            recursionCounter);
  } else {
    std::ostringstream tempSS;
    tempSS << "Balanced module placement in rod pair at avg build radius " << (maxBuildRadius()+minBuildRadius())/2. << " converged after " << recursionCounter << " step(s).\n" 
           << "   Residual Z unbalance is " << zUnbalance << ".\n"
           << "   Positive string has " << zPlusList.size() << " modules, negative string has " << zMinusList.size() << " modules.\n"
           << "   Z+ rod starts at " << zPlusList.front() << ", Z- rod starts at " << zMinusList.front() << ".";
    logINFO(tempSS);
    return std::make_pair(zPlusList, zMinusList);
  }
}

void StraightRodPair::buildModules(Container& modules, const RodTemplate& rodTemplate, const vector<double>& posList, BuildDir direction, int parity, int side) {
  for (int i=0; i<(int)posList.size(); i++, parity = -parity) {
    BarrelModule* mod = new BarrelModule(i < rodTemplate.size() ? *rodTemplate[i].get() : *rodTemplate.rbegin()->get());
    mod->setup();
    mod->myid(i+1);
    mod->side(side);
    //mod->store(propertyTree());
    //if (ringNode.count(i+1) > 0) mod->store(ringNode.at(i+1)); 
    //mod->build();
    mod->translateR(parity > 0 ? smallDelta() : -smallDelta());
    mod->translateZ(posList[i] + (direction == BuildDir::RIGHT ? mod->length()/2 : -mod->length()/2));
   // mod->translate(XYZVector(parity > 0 ? smallDelta() : -smallDelta(), 0, posList[i])); // CUIDADO: we are now translating the center instead of an edge as before
    modules.push_back(mod);
  }
}

void StraightRodPair::buildFull(const RodTemplate& rodTemplate) {
  double startZ = startZMode() == StartZMode::MODULECENTER ? -(*rodTemplate.begin())->length()/2. : 0.;
  auto zListPair = computeZListPair(rodTemplate.begin(), rodTemplate.end(), startZ, 0);

    // actual module creation
    // CUIDADO log rod balancing effort
  buildModules(zPlusModules_, rodTemplate, zListPair.first, BuildDir::RIGHT, zPlusParity(), 1);
  double currMaxZ = zPlusModules_.size() > 1 ? MAX(zPlusModules_.rbegin()->maxZ(), (zPlusModules_.rbegin()+1)->maxZ()) : (!zPlusModules_.empty() ? zPlusModules_.rbegin()->maxZ() : 0.); 
  // CUIDADO this only checks the positive side... the negative side might actually have a higher fabs(maxZ) if the barrel is long enough and there's an inversion
  buildModules(zMinusModules_, rodTemplate, zListPair.second, BuildDir::LEFT, -zPlusParity(), -1);

  if (maxZ.state() && currMaxZ > maxZ()) compressToZ(maxZ());
  
  if (solveCollisionsZPlus() | solveCollisionsZMinus()) logWARNING("Some modules have been translated to avoid collisions. Check info tab");
  currMaxZ = zPlusModules_.size() > 1 ? MAX(zPlusModules_.rbegin()->maxZ(), (zPlusModules_.rbegin()+1)->maxZ()) : (!zPlusModules_.empty() ? zPlusModules_.rbegin()->maxZ() : 0.); 
  maxZ(currMaxZ);
}

void StraightRodPair::buildMezzanine(const RodTemplate& rodTemplate) {
  // compute Z list (only once since the second mezzanine has just inverted signs for z) 
  vector<double> zList = computeZList(rodTemplate.rbegin(), rodTemplate.rend(), startZ(), BuildDir::LEFT, zPlusParity(), false);
  vector<double> zListNeg;
  std::transform(zList.begin(), zList.end(), std::back_inserter(zListNeg), std::negate<double>());

  buildModules(zPlusModules_, rodTemplate, zList, BuildDir::LEFT, zPlusParity(), 1); // CUIDADO mezzanine layer rings in reverse order????
  maxZ(startZ());

  buildModules(zMinusModules_, rodTemplate, zListNeg, BuildDir::RIGHT, zPlusParity(), -1);
}


void StraightRodPair::build(const RodTemplate& rodTemplate) {
  try {
    std::cout << ">>> Building " << fullid(*this) << " <<<" << std::endl;
    check();
    if (!mezzanine()) buildFull(rodTemplate);
    else buildMezzanine(rodTemplate);
  } catch (PathfulException& pe) { pe.pushPath(fullid(*this)); throw; }

  cleanup();
  builtok(true);
}

void TiltedRodPair::buildModules(Container& modules, const RodTemplate& rodTemplate, const vector<TiltedModuleSpecs>& tmspecs, BuildDir direction) {
  auto it = rodTemplate.begin();
  int side = (direction == BuildDir::RIGHT ? 1 : -1);
  int i = (direction == BuildDir::LEFT && tmspecs[0].z == 0.); // this skips the first module if we're going left (i.e. neg rod) and z=0 because it means the pos rod has already got a module there
  for (; i < tmspecs.size(); i++, ++it) {
    BarrelModule* mod = new BarrelModule(**it);
    mod->setup();
    mod->myid(i+1);
    mod->side(side);
    mod->tilt(side * tmspecs[i].gamma);
    mod->translateR(tmspecs[i].r);
    mod->translateZ(side * tmspecs[i].z);
    modules.push_back(mod);
  }
}


void TiltedRodPair::build(const RodTemplate& rodTemplate, const std::vector<TiltedModuleSpecs>& tmspecs) {
  try {
    std::cout << ">>> Building " << fullid(*this) << " <<<" << std::endl;
    check();
    buildModules(zPlusModules_, rodTemplate, tmspecs, BuildDir::RIGHT);
    buildModules(zMinusModules_, rodTemplate, tmspecs, BuildDir::LEFT);

  } catch (PathfulException& pe) { pe.pushPath(fullid(*this)); throw; }
  cleanup();
  builtok(true);
}


define_enum_strings(RodPair::StartZMode) = { "modulecenter", "moduleedge" };
