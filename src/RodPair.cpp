std::vector<double> RodPair::maxZStrategy(const vector<double>& dsDistances, double startZ, int direction, int smallParity, bool looseStartZ) {

  std::vector<double> zList;
  double newZ = startZ;
  int parity = smallParity;
  double maxz = parent_.maxZ();
  double D = parent_.bigDelta();
  double d = parent_.smallDelta();
  double dz = parent_.originDeltaZ();
  double ov = parent_.baseOverlapZ();
  double maxr = parent_.maxPlaceRadius();
  double minr = parent_.minPlaceRadius();
  double mlen = parent_.moduleLength();

  size_t i = 0;
  auto dsBegin = dsDistances.begin(), dsEnd = dsDistances.end();
  auto dsIt = dsBegin;
  if (!looseStartZ) {
    zList.push_back(startZ);
    newZ = startZ + (direction > 0 ? mlen : -mlen);
    dsIt++;
    parity = -parity;
  }

  while (abs(newZ) < maxz) { // maxZ is always > 0 even for Neg Z rods
    double newR = (parity > 0 ? maxr + D + d : minr - D - d) - (*dsIt)/2; 
    double lastR = (parity > 0 ? maxr + D - d : minr - D + d) + (dsIt > dsBegin ? *(dsIt-1) : *dsBegin)/2;// (if looseStart) the previous module (opposite rod) has the same dsDistance
    if (direction > 0) {
      double originZ = parity > 0 ? dz : -dz;
      double newZorigin = (newZ - ov)*newR/lastR;
      double newZshifted = (newZ - originZ)*newR/lastR + originZ;
      newZ = newZorigin < newZshifted ? newZorigin : newZshifted;
      zList.push_back(newZ);
      newZ += mlen;
    } else {
      double originZ = parity > 0 ? -dz : dz;
      double newZorigin = (newZ + ov)*newR/lastR;
      double newZshifted = (newZ - originZ)*newR/lastR + originZ;
      newZ = newZorigin > newZshifted ? newZorigin : newZshifted;
      zList.push_back(newZ);
      newZ -= mlen;
    }
    parity = -parity;

    if (dsIt < dsEnd-1) dsIt++;
  }
  //return listZ[listZ.size()-1] + (direction > 0 ? modLengthZ : -modLengthZ);
  //
  return zList;
}

std::vector<double> RodPair::numModulesStrategy(const vector<double>& dsDistances, double startZ, int direction, int smallParity, bool looseStartZ) {
  std::vector<double> zList;

  double newZ = startZ;
  int parity = smallParity;
  double maxz = parent_.maxZ();
  double D = parent_.bigDelta();
  double d = parent_.smallDelta();
  double dz = parent_.originDeltaZ();
  double ov = parent_.baseOverlapZ();
  double maxr = parent_.maxPlaceRadius();
  double minr = parent_.minPlaceRadius();
  double mlen = parent_.moduleLength();

  int i = 0;
  if (!looseStartZ) {
    listZ.push_back(startZ);
    newZ = startZ + (direction > 0 ? mlen : -mlen);
    i++;
    parity = -parity;
  }

  for (; i<numModules(); i++) {
    double newR = (parity > 0 ? maxr + D + d : minr - D - d) - dsDistances[i]/2; 
    double lastR = (parity > 0 ? maxr + D - d : minr - D + d) + dsDistances[i > 0 ? i-1 : 0]/2;// (if looseStart) the previous module (opposite rod) has the same dsDistance
    if (direction > 0) {
      double originZ = parity > 0 ? dz : -dz;
      double newZorigin = (newZ - ov)*newR/lastR;
      double newZshifted = (newZ - originZ)*newR/lastR + originZ;
      newZ = newZorigin < newZshifted ? newZorigin : newZshifted;
      listZ.push_back(newZ);
      newZ += mlen;
    } else {
      double originZ = parity > 0 ? -dz : dz;
      double newZorigin = (newZ + ov)*newR/lastR;
      double newZshifted = (newZ - originZ)*newR/lastR + originZ;
      newZ = newZorigin > newZshifted ? newZorigin : newZshifted;
      listZ.push_back(newZ);
      newZ -= mlen;
    }
    parity = -parity;
  }

  return zList;
 // return newZ;
  
}


std::vector<double> RodPair::computeZList(const vector<double>& dsDistances, double startZ, int direction, int smallParity, bool looseStartZ) {
  return parent_.maxZ.isSet() ? 
         maxZStrategy(dsDistances, startZ, direction, smallParity, looseStartZ) : 
         numModulesStrategy(dsDistances, startZ, direction, smallParity, looseStartZ);
}


std::pair<std::vector<double>, std::vector<double>> RodPair::computeZListPair(const vector<double>& dsDistances, double startZ, int recursionCounter) {
  if (++recursionCounter == 100) { // this stops infinite recursion if the balancing doesn't converge
    // CUIDADO handle error (exception)
    return;
  }  

  bool looseStartZ = false;
  std::vector<double> zPlusList = computeZList(dsDistances, startZ, RIGHTWARD_BUILD, parent_.zPlusParity(), looseStartZ);
  std::vector<double> zMinusList = computeZList(dsDistances, startZ, LEFTWARD_BUILD, -parent_.zMinusParity(), !looseStartZ);

  double zUnbalance = (*zPlusList.rbegin()+moduleLength) + (*zMinusList.rbegin()-moduleLength); // balancing uneven pos/neg strings
  if (abs(zUnbalance) > 0.1) { // 0.1 mm unbalance is tolerated
    return buildRecursive(dsDistances,
                          startZ-zUnbalance/2, // countering the unbalance by displacing the startZ (by half the inverse unbalance, to improve convergence)
                          recursionCounter);
  } else {
    return std::make_pair(zPlusList, zMinusList);
  }
};

void RodPair::buildFull(const vector<shared_ptr<Module>>& rodTemplate) {

  bool looseStartZ = false;
  vector<double> dsDistances;
  std::transform(rodTemplate.begin(), rodTemplate.end(), std::back_inserter(dsDistances.begin()), [](Module* m) { return m->dsDistance(); } );
  auto zListPair = computeZListRecursive(dsDistances, 0., 0);

    // actual module creation
    // CUIDADO log rod balancing effort

  for (int i=0, parity = parent_.zPlusParity(); i<(int)zListPair.first.size(); i++, parity = -parity) {
    Module* mod = new Module(rodTemplate[i]);
    mod->ring(i+1);
    mod->zSide(1);
    mod->translate(XYZVector(0, parity > 0 ? parent_.smallDelta() : -parent_.smallDelta(), zListPair.first[i])); // CUIDADO: we are now translating the center instead of an edge as before
    modules_.push_back(mod);
  }

  for (int i=0, parity = -parent_.zPlusParity(); i<(int)zListPair.second.size(); i++, parity = -parity) {
    Module* mod = new Module(rodTemplate[i]);
    mod->ring(i+1);
    mod->zSide(1);
    mod->translate(XYZVector(0, parity > 0 ? parent_.smallDelta() : -parent_.smallDelta() , zListPair.second[i]));
    modules_.push_back(mod);
  }

}

void RodPair::buildMezzanine(const vector<shared_ptr<Module>>& rodTemplate) {
  // compute Z list (only once since the second mezzanine has just inverted signs for z) 
  vector<double> dsDistances;
  std::transform(rodTemplate.rbegin(), rodTemplate.rend(), std::back_inserter(dsDistances.begin()), [](Module* m) { return m->dsDistance(); } );
  vector<double> listZ = computeZList(dsDistances, parent_.startZ(), LEFTWARD_BUILD, parent_.zPlusParity(), false);

  for (int i=0, parity = parent_.zPlusParity(); i<(int)listZ.size(); i++, parity = -parity) {
    Module* mod = new Module(rodTemplate[i]);
    mod->ring(i+1); // CUIDADO looks like mezzanine layers have ring numbers in reverse order!!!
    mod->zSide(1);
    mod->translate(XYZVector(0, parity > 0 ? parent_.smallDelta() : -parent_.smallDelta(), zList[i]));
    modules_.push_back(mod);        
  }

  for (int i=0, parity = -parent_.zPlusParity(); i<(int)listZ.size(); i++, parity = -parity) {
    Module* mod = new Module();
    mod->ring(i+1);
    mod->zSide(-1);
    mod->build(pt, previouslyUnmatched);
    mod->translate(XYZVector(0, parity > 0 ? parent_.smallDelta() : -parent_.smallDelta(), -zList[i]));
    modules_.push_back(mod);
  }
}


void RodPair::build(vector<shared_ptr<Module>>& rodTemplate) {
  if (!parent_.mezzanine()) buildFull(rodTemplate);
  else buildMezzanine(rodTemplate);
}

void RodPair::translate(const XYZVector& translation) {
  std::transform(modules_.begin(), modules_.end(), std::bind2nd(std::mem_fun<&Module::translate>(), translation));
}

void RodPair::rotatePhi(float phi) {
  std::transform(modules_.begin(), modules_.end(), std::bind2nd(std::mem_fun<&Module::rotatePhi>(), phi));
}



