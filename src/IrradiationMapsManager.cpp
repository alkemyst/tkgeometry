/**
 * @file IrradiationMapsManager.cpp
 * @author Stefano Martina
 * @date 19/feb/2014
 */

#include "IrradiationMapsManager.h"

IrradiationMapsManager::IrradiationMapsManager() {
}

IrradiationMapsManager::~IrradiationMapsManager() {
  irradiationMaps.clear();
}

/**
 * Add the passed map to internal set
 * @param IrradiationMap is the new map
 */
void IrradiationMapsManager::addIrradiationMap(const IrradiationMap& newIrradiationMap) {
  irradiationMaps.insert(newIrradiationMap);
}

/**
 * Create a new map with passed file and ad it to internal set
 * @param newIrradiationMapFile is the path of the file for the new map
 */
void IrradiationMapsManager::addIrradiationMap(std::string newIrradiationMapFile) {
  IrradiationMap newIrradiationMap (newIrradiationMapFile);
  addIrradiationMap(newIrradiationMap);
}

/**
 * Get the irradiation of the point in the passed coordinates using the best
 * avaiable map that contains that point
 * @param coordinates represent the point, is a pair (z,r) with coordinates z in Z, and r in Rho
 * @return The value of irradiation in the point
 */
double IrradiationMapsManager::calculateIrradiationPower(std::pair<double,double> coordinates) const{
  double irradiation = 0;
  bool mapFound = false;

  //Iterate through the ordered map set untill find a proper map
  for(std::set<IrradiationMap>::const_iterator iter = irradiationMaps.cbegin(); iter != irradiationMaps.cend(); ++ iter) {
    if (iter->isInRegion(coordinates)) {
      irradiation = iter->calculateIrradiation(coordinates);
      mapFound = true;
      break;
    }
  }
  if(!mapFound) {
    logERROR("Error while calculating irradiation, a proper irradiation map is not found");
  }

  return irradiation;
}
