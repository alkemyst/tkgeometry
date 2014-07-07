/*
 * materialObject.cpp
 *
 *  Created on: 19/giu/2014
 *      Author: stefano
 */

#include "MaterialObject.h"

MaterialObject::~MaterialObject() {
  // TODO Auto-generated destructor stub
}

void MaterialObject::build() {

  //TODO: fare store e build su MaterialObject; passare property object solo di Materials module (non importa passare tutto il tree, non serve ereditarietà proprietà); su MaterialObject popolare i materiali; fare stessa cosa di qui detectormodule sul layer.
  std::cout << "pippero " << materialsNode.size() << std::endl;
}

