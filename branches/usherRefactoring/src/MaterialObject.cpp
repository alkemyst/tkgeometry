/**
 * @file MaterialObject.cpp
 *
 * @date 19/giu/2014
 * @author Stefano Martina
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

//const std::string MaterialObject::componentString = "Component";

void MaterialObject::build() {
  //std::cout << "Materials " << materialsNode_.size() << std::endl;
  for (auto& currentMaterialNode : materialsNode_) {
    if (currentMaterialNode.first.compare(getTypeString()) == 0) {
      /*
      for (auto& currentComponentNode : currentMaterialNode.second) {
        if (currentComponentNode.first.compare(componentString)) {
          Component* newComponent = new Component();
          newComponent->store(currentComponentNode.second);
          newComponent->build();

          components.push_back(newComponent);
        } else {
        }
      }
      */

      cleanupTree();
      store(currentMaterialNode.second);
      buildComponents();
    }
  }
}

void MaterialObject::buildComponents() {
  //TODO: ATTENTION COMPONENTS ARE COLLAPSED if have the same name (correct it or name Components univocally in Materials property)
  for (auto& currentComponentNode : componentsNode_) {
    //std::cout << "COMPONENT " << currentComponentNode.first << std::endl;
    Component* newComponent = new Component();
    newComponent->store(currentComponentNode.second);
    newComponent->build();
  }
}

void MaterialObject::Component::build() {
  //set property 'component' from data of the property tree
  component.fromPtree(propertyTree());

  for  (auto& currentElementNode : elementNode_) {
    //std::cout << "  ELEMENT " << currentElementNode.first << std::endl;
    Element* newElement = new Element();
    newElement->store(currentElementNode.second);
    newElement->build();

    elements.push_back(newElement);
  }
}

void MaterialObject::Component::Element::build() {
  element.fromPtree(propertyTree());
  /*
  std::cout << "    DATA "
      << " exiting " << exiting()
      << " scale " << scale()
      << " quantity " << quantity()
      << " unit " << unit()
      << std::endl;
   */
}
