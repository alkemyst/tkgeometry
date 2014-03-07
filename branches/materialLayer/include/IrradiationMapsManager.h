/**
 * @file IrradiationMapsManager.h
 * @author Stefano Martina
 * @date 19/feb/2014
 */

#ifndef IRRADIATIONMAPSMANAGER_H_
#define IRRADIATIONMAPSMANAGER_H_

#include <utility>
#include <string>
#include <set>
#include"IrradiationMap.h"

/**
 * @class IrradiationMapsManager
 * @brief The administrator of the irradiation maps.
 * @details Mantains a set of maps sorted by resolution, when
 * is asked for the irradiation of a point returns the value of the
 * better map that contains this point in his region
 */
class IrradiationMapsManager {
public:
  IrradiationMapsManager();
  ~IrradiationMapsManager();
  void addIrradiationMap(const IrradiationMap& newIrradiationMap);
  void addIrradiationMap(std::string newIrradiationMapFile);
  double calculateIrradiationPower(std::pair<double,double> coordinates) const;

private:

  /**
   * The set that contains all the maps ordered by resolution
   */
  std::set<IrradiationMap> irradiationMaps;
};

#endif /* IRRADIATIONMAPSMANAGER_H_ */
