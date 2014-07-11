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

  //TODO: fare store e build su MaterialObject; passare property object solo di Materials module (non importa passare tutto il tree, non serve ereditarietà proprietà); su MaterialObject popolare i materiali; fare stessa cosa di qui detectormodule sul layer.
  for (auto& currentMaterialNode : materialsNode_) {
    if (currentMaterialNode.first.compare(getTypeString()) == 0) {
      /*
      std::cout << "PIPPO " << type_ << " " << currentMaterialNode.second.size() << std::endl;
      for (auto& currentComponentNode : currentMaterialNode.second) {
        std::cout << "PLUTO " << currentComponentNode.first << std::endl;
        if (currentComponentNode.first.compare(componentString)) {
          Component* newComponent = new Component();
          newComponent->store(currentComponentNode.second);
          newComponent->build();

          components.push_back(newComponent);
        } else {
          std::cout << "MAH " << currentComponentNode.second.size() << std::endl;
        }
      }
      std::cout << std::endl;
      */

      std::cout << "MAT " << materialsNode_.size() << std::endl;
      std::cout << "COM " << componentsNode_.size() << std::endl;
      cleanupTree();
      store(currentMaterialNode.second);
      buildComponents();
    }
  }
}

void MaterialObject::buildComponents() {
  /*
  std::cout << "PIPPO " << componentsNode_.size() << std::endl;
  std::cout << "PAPPO " << propertyTree().size() << std::endl;
  for(auto& currPt : propertyTree()) {
    std::cout << "PAPPA " << currPt.first << std::endl;
  }
  */

  //TODO: ATTENTION COMPONENTS ARE COLLAPSED, NO GOOD !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
  for (auto& currentComponentNode : componentsNode_) {
    std::cout << "COMPONENT " << currentComponentNode.first << std::endl;
    Component* newComponent = new Component();
    newComponent->store(currentComponentNode.second);
    newComponent->build();
  }
}

void MaterialObject::Component::build() {
  //set property 'component' from data of the property tree
  component.fromPtree(propertyTree());

  for  (auto& currentElementNode : elementNode_) {
    std::cout << "  ELEMENT " << currentElementNode.first << std::endl;
    Element* newElement = new Element();
    newElement->store(currentElementNode.second);
    newElement->build();

    elements.push_back(newElement);
  }
}

void MaterialObject::Component::Element::build() {
  element.fromPtree(propertyTree());
  std::cout << "    DATA "
      << " exiting " << exiting()
      << " scale " << scale()
      << " quantity " << quantity()
      << " unit " << unit()
      << std::endl;
}
