/**
 * @file InactiveModuleTube.cpp
 * @author Stefano Martina
 * @date 13/mar/2014
 */

#include "InactiveModuleTube.h"

namespace insur {

  InactiveModuleTube::InactiveModuleTube() : InactiveElement() {
    isVertical_ = false;
    feederModuleIndex_ = -1;
  }

  InactiveModuleTube::InactiveModuleTube(InactiveElement& previous) : InactiveElement(previous) {
    isVertical_ = false;
    feederModuleIndex_ = -1;
  }

  InactiveModuleTube::~InactiveModuleTube() {}

  void InactiveModuleTube::setModuleFeederIndex(int module) {
    feederModuleIndex_ = module;
  }

  int InactiveModuleTube::getModuleFeederIndex() {
    return feederModuleIndex_;
  }

} /* namespace insur */
