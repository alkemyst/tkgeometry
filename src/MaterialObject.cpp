/*
 * materialObject.cpp
 *
 *  Created on: 19/giu/2014
 *      Author: stefano
 */

#include "MaterialObject.h"

const std::map<MaterialObject::Type, const std::string> MaterialObject::typeString = {
    {MODULE, "module"},
    {ROD, "rod"}
};

const std::string MaterialObject::getTypeString() const {
  auto mapIter = typeString.find(type_);
  if (mapIter != typeString.end()) {
    return mapIter->second;
  } else {
    return "";
  }
}

void MaterialObject::build() {

  //TODO: fare store e build su MaterialObject; passare property object solo di Materials module (non importa passare tutto il tree, non serve ereditarietà proprietà); su MaterialObject popolare i materiali; fare stessa cosa di qui detectormodule sul layer.
  //std::cout << "pippero " << materialsNode_.size() << std::endl;

  for (auto& currentMaterialNode : materialsNode_) {
    std::cout << "pimpero " << currentMaterialNode.first << std::endl;
    //const std::string pippo = typeString[type_];
    if (currentMaterialNode.first.compare(getTypeString()) == 0) {
    //if (currentMaterialNode.first == typeString[type_]) {
    //if(false) {
      for (auto& currentComponentNode : currentMaterialNode.second) {
        Component* newComponent = new Component();
        newComponent->store(currentComponentNode.second);
        newComponent->build();

        components.push_back(newComponent);
      }
    }
  }
  //std::cout << "pappero " << materialsNode_() << std::endl;
}

void MaterialObject::Component::build() {

}

void MaterialObject::Component::Element::build() {

}
