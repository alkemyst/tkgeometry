#include "RodPair.h"


template<typename Iterator> vector<double> RodPair::maxZStrategy(Iterator begin, Iterator end, double startZ, int direction, int smallParity, bool looseStartZ) {

  vector<double> zList;
  double newZ = startZ;
  int parity = smallParity;
  double maxz = maxZ();
  double d = smallDelta();
  double dz = zError();
  double ov = minModuleOverlap();
  double maxr = maxBuildRadius();
  double minr = minBuildRadius();

  auto mit = begin;
  if (!looseStartZ) {
    zList.push_back(startZ);
    newZ = startZ + (direction > 0 ? (*mit)->length() : -(*mit)->length());
    ++mit;
    parity = -parity;
  }

  while (abs(newZ) < maxz) { // maxZ is always > 0 even for Neg Z rods
    double newR = (parity > 0 ? maxr + d : minr - d) - (*mit)->dsDistance()/2; 
    double lastR = (parity > 0 ? maxr - d : minr + d) + (mit != begin ? (*(mit-1))->dsDistance() : (*begin)->dsDistance())/2;// (if looseStart) the previous module (opposite rod) has the same dsDistance
    if (direction > 0) {
      double originZ = parity > 0 ? dz : -dz;
      double newZorigin = (newZ - ov)*newR/lastR;
      double newZshifted = (newZ - originZ)*newR/lastR + originZ;
      newZ = newZorigin < newZshifted ? newZorigin : newZshifted;
      zList.push_back(newZ);
      newZ += (*mit)->length();
    } else {
      double originZ = parity > 0 ? -dz : dz;
      double newZorigin = (newZ + ov)*newR/lastR;
      double newZshifted = (newZ - originZ)*newR/lastR + originZ;
      newZ = newZorigin > newZshifted ? newZorigin : newZshifted;
      zList.push_back(newZ);
      newZ -= (*mit)->length();
    }
    parity = -parity;

    if (mit < end-1) ++mit;
  }
  //return listZ[listZ.size()-1] + (direction > 0 ? modLengthZ : -modLengthZ);
  //
  return zList;
}

template<typename Iterator> vector<double> RodPair::numModulesStrategy(Iterator begin, Iterator end, double startZ, int direction, int smallParity, bool looseStartZ) {
  vector<double> zList;


  double newZ = startZ;
  int parity = smallParity;
  double d = smallDelta();
  double dz = zError();
  double ov = minModuleOverlap();
  double maxr = maxBuildRadius();
  double minr = minBuildRadius();

  auto mit = begin;
  if (!looseStartZ) {
    zList.push_back(startZ);
    newZ = startZ + (direction > 0 ? (*mit)->length() : -(*mit)->length());
    ++mit;
    parity = -parity;
  }

  for (int i = looseStartZ ? 0 : 1; i<numModules(); ++i, ++mit) {
    double newR = (parity > 0 ? maxr + d : minr - d) - (*mit)->dsDistance()/2; 
    double lastR = (parity > 0 ? maxr - d : minr + d) + (mit != begin ? (*(mit-1))->dsDistance() : (*begin)->dsDistance())/2;// (if looseStart) the previous module (opposite rod) has the same dsDistance
    if (direction > 0) {
      double originZ = parity > 0 ? dz : -dz;
      double newZorigin = (newZ - ov)*newR/lastR;
      double newZshifted = (newZ - originZ)*newR/lastR + originZ;
      newZ = newZorigin < newZshifted ? newZorigin : newZshifted;
      zList.push_back(newZ);
      newZ += (*mit)->length();
    } else {
      double originZ = parity > 0 ? -dz : dz;
      double newZorigin = (newZ + ov)*newR/lastR;
      double newZshifted = (newZ - originZ)*newR/lastR + originZ;
      newZ = newZorigin > newZshifted ? newZorigin : newZshifted;
      zList.push_back(newZ);
      newZ -= (*mit)->length();
    }
    parity = -parity;
  }

  return zList;
 // return newZ;
  
}


template<typename Iterator> vector<double> RodPair::computeZList(Iterator begin, Iterator end, double startZ, int direction, int smallParity, bool looseStartZ) {
  return maxZ.state() ? 
         maxZStrategy(begin, end, startZ, direction, smallParity, looseStartZ) : 
         numModulesStrategy(begin, end, startZ, direction, smallParity, looseStartZ);
}


template<typename Iterator> pair<vector<double>, vector<double>> RodPair::computeZListPair(Iterator begin, Iterator end, double startZ, int recursionCounter) {

  bool looseStartZ = false;
  vector<double> zPlusList = computeZList(begin, end, startZ, RIGHTWARD_BUILD, zPlusParity(), looseStartZ);
  vector<double> zMinusList = computeZList(begin, end, startZ, LEFTWARD_BUILD, -zPlusParity(), !looseStartZ);

  double zUnbalance = (zPlusList.back()+(*(end-1))->length()) + (zMinusList.back()-(*(end-1))->length()); // balancing uneven pos/neg strings
  if (abs(zUnbalance) > 0.1 && ++recursionCounter < 100) { // 0.1 mm unbalance is tolerated
    return computeZListPair(begin, end,
                            startZ-zUnbalance/2, // countering the unbalance by displacing the startZ (by half the inverse unbalance, to improve convergence)
                            recursionCounter);
  } else {
    // CUIDADO HANDLE RECURSION COUNTER HITTING 100 ERROR
    return std::make_pair(zPlusList, zMinusList);
  }
}

void RodPair::buildFull(const RodTemplate& rodTemplate) {

  bool looseStartZ = false;
  vector<double> dsDistances;
  auto zListPair = computeZListPair(rodTemplate.begin(), rodTemplate.end(), 0., 0);

    // actual module creation
    // CUIDADO log rod balancing effort

  for (int i=0, parity = zPlusParity(); i<(int)zListPair.first.size(); i++, parity = -parity) {
    BarrelModule* mod = new BarrelModule(*rodTemplate[i]);
    mod->myid(i+1);
    mod->ring(i+1);
    mod->side(1);
    mod->store(propertyTree());
    mod->build();
    mod->translate(XYZVector(0, parity > 0 ? smallDelta() : -smallDelta(), zListPair.first[i])); // CUIDADO: we are now translating the center instead of an edge as before
    modules_.push_back(mod);
  }

  for (int i=0, parity = -zPlusParity(); i<(int)zListPair.second.size(); i++, parity = -parity) {
    BarrelModule* mod = new BarrelModule(*rodTemplate[i]);
    mod->myid(i+1);
    mod->ring(i+1);
    mod->side(1);
    mod->store(propertyTree());
    mod->translate(XYZVector(0, parity > 0 ? smallDelta() : -smallDelta() , zListPair.second[i]));
    modules_.push_back(mod);
  }

}

void RodPair::buildMezzanine(const RodTemplate& rodTemplate) {
  // compute Z list (only once since the second mezzanine has just inverted signs for z) 
  vector<double> zList = computeZList(rodTemplate.rbegin(), rodTemplate.rend(), startZ(), LEFTWARD_BUILD, zPlusParity(), false);

  for (int i=0, parity = zPlusParity(); i<(int)zList.size(); i++, parity = -parity) {
    BarrelModule* mod = new BarrelModule(*rodTemplate[i]);
    mod->myid(i+1);
    mod->ring(i+1); // CUIDADO looks like mezzanine layers have ring numbers in reverse order!!!
    mod->side(1);
    mod->store(propertyTree());
    mod->build();
    mod->translate(XYZVector(0, parity > 0 ? smallDelta() : -smallDelta(), zList[i]));
    modules_.push_back(mod);        
  }

  for (int i=0, parity = -zPlusParity(); i<(int)zList.size(); i++, parity = -parity) {
    BarrelModule* mod = new BarrelModule(*rodTemplate[i]);
    mod->myid(i+1);
    mod->ring(i+1);
    mod->side(-1);
    mod->store(propertyTree());
    mod->build();
    mod->translate(XYZVector(0, parity > 0 ? smallDelta() : -smallDelta(), -zList[i]));
    modules_.push_back(mod);
  }
}


void RodPair::build(const RodTemplate& rodTemplate) {
  try {
    check();
    if (!mezzanine()) buildFull(rodTemplate);
    else buildMezzanine(rodTemplate);
    built_ = true;
  } catch (PathfulException& pe) { pe.pushPath(fullid()); throw; }
}

void RodPair::translate(const XYZVector& translation) {
  for (auto& m : modules_) { m.translate(translation); }
}

void RodPair::rotatePhi(double angle) {
  for (auto& m : modules_) { m.rotatePhi(angle); }
}



