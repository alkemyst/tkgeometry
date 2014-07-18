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

void MaterialObject::build() {
  //std::cout << "Materials " << materialsNode_.size() << std::endl;
  for (auto& currentMaterialNode : materialsNode_) {
    if (currentMaterialNode.first.compare(getTypeString()) == 0) {
      //cleanupTree();
      store(currentMaterialNode.second);
      buildComponents();
    }
  }
}

void MaterialObject::buildComponents() {
  for (auto& currentComponentNode : componentsNode_) {
    Component* newComponent = new Component();
    newComponent->store(propertyTree());
    newComponent->store(currentComponentNode.second);
    newComponent->build();
  }
}

void MaterialObject::Component::build() {
  std::cout << "COMPONENT " << componentName() << std::endl;
  for (auto& currentComponentNode : componentsNode_) {
    Component* newComponent = new Component();
    newComponent->store(propertyTree());
    newComponent->store(currentComponentNode.second);
    newComponent->build();
  }
  for  (auto& currentElementNode : elementNode_) {
    Element* newElement = new Element();
    newElement->store(propertyTree());
    newElement->store(currentElementNode.second);
    newElement->build();

    elements.push_back(newElement);
  }
}

void MaterialObject::Component::Element::build() {
  std::cout << "  ELEMENT " << elementName() << std::endl;
  std::cout << "    DATA "
      << " nSegments " << (nSegments.state() ? std::to_string(nSegments()) : "NOT_SET")
      << " exiting " << service()
      << " scale " << scale()
      << " quantity " << quantity()
      << " unit " << unit()
      << std::endl;
}
