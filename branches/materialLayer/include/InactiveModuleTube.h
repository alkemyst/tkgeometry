/**
 * @file InactiveModuleTube.h
 * @author Stefano Martina
 * @date 13/mar/2014
 */

#ifndef INACTIVEMODULETUBE_H_
#define INACTIVEMODULETUBE_H_

#include "InactiveElement.h"

namespace insur {

  /**
   * @class InactiveModuleTube
   * @brief This class extends InactiveElement providing the capability of managing
   * the index of a feeder module (apart from the existing index of the feeder layer
   */
  class InactiveModuleTube : public InactiveElement {
  public:
    InactiveModuleTube();
    InactiveModuleTube(InactiveElement& previous);
    virtual ~InactiveModuleTube();
    void setModuleFeederIndex(int moduleIndex);
    int getModuleFeederIndex();

  protected:
    int feederModuleIndex_;
  };

} /* namespace insur */

#endif /* INACTIVEMODULETUBE_H_ */
