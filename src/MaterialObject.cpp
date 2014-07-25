/**
 * @file MaterialObject.cpp
 *
 * @date 19/giu/2014
 * @author Stefano Martina
 */

#include "MaterialObject.h"

namespace material {

  const std::map<MaterialObject::Type, const std::string> MaterialObject::typeString = {
      {MODULE, "module"},
      {ROD, "rod"}
  };

  const std::string MaterialObject::getTypeString() const {
    auto mapIter = typeString.find(materialType_);
    if (mapIter != typeString.end()) {
      return mapIter->second;
    } else {
      return "";
    }
  }

  void MaterialObject::build() {
    static std::map<std::string, Materials*> materialsMap_; //for saving memory

    //std::cout << "Materials " << materialsNode_.size() << std::endl;

    for (auto& currentMaterialNode : materialsNode_) {
      store(currentMaterialNode.second);
      check();
      if (type_().compare(getTypeString()) == 0) {
        if (materialsMap_.count(currentMaterialNode.first) == 0) {
          Materials * newMaterials  = new Materials();
          newMaterials->store(currentMaterialNode.second);
          newMaterials->build();
          materialsMap_[currentMaterialNode.first] = newMaterials;
        }
        materials = materialsMap_[currentMaterialNode.first];

        break;
      }
    }

    cleanup();
  }

  void MaterialObject::Materials::build() {
    for (auto& currentComponentNode : componentsNode_) {
      Component* newComponent = new Component();
      newComponent->store(propertyTree());
      newComponent->store(currentComponentNode.second);
      newComponent->check();
      newComponent->build();

      components.push_back(newComponent);
    }
    cleanup();
  }

  void MaterialObject::Materials::Component::build() {
    //std::cout << "COMPONENT " << componentName() << std::endl;

    //sub components
    for (auto& currentComponentNode : componentsNode_) {
      Component* newComponent = new Component();
      newComponent->store(propertyTree());
      newComponent->store(currentComponentNode.second);
      newComponent->check();
      newComponent->build();

      components.push_back(newComponent);
    }
    //elements
    for  (auto& currentElementNode : elementsNode_) {
      Element* newElement = new Element();
      newElement->store(propertyTree());
      newElement->store(currentElementNode.second);
      newElement->check();
      newElement->cleanup();
      //newElement->build();

      elements.push_back(newElement);
    }
    cleanup();
  }

  /*
  void MaterialObject::Materials::Component::Element::build() {
    std::cout << "  ELEMENT " << elementName() << std::endl;
    std::cout << "    DATA "
        << " nSegments " << (nSegments.state() ? std::to_string(nSegments()) : "NOT_SET")
        << " exiting " << service()
        << " scale " << scale()
        << " quantity " << quantity()
        << " unit " << unit()
        << std::endl;
  }
  */
} /* namespace material */
