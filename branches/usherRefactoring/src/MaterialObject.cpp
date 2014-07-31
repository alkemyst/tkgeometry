/**
 * @file MaterialObject.cpp
 *
 * @date 19/giu/2014
 * @author Stefano Martina
 */

#include "MaterialObject.h"
#include "global_constants.h"
#include "MaterialTab.h"




namespace material {

  const std::map<MaterialObject::Type, const std::string> MaterialObject::typeString = {
      {MODULE, "module"},
      {ROD, "rod"}
  };

  MaterialObject::MaterialObject(Type materialType) :
      materialType_ (materialType),
      type_ ("type", parsedAndChecked()),
      materialsNode_ ("Materials", parsedOnly()),
      materialTab_ (MaterialTab::instance()),
      materials (nullptr) {}

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
      //MaterialTab::instance(); //CANCELLAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
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

  void MaterialObject::routeServicesTo(MaterialObject& outputObject) const {
    for(Element & currElement : serviceElements) {
      outputObject.addElementIfService(currElement);
    }
    if (materials != nullptr) {
      materials->routeServicesTo(outputObject);
    }
  }

  void MaterialObject::addElementIfService(Element& inputElement) {
    if (inputElement.service() == true) {
      serviceElements.push_back(&inputElement);
    }
  }


  //void MaterialObject::chargeTrain(Materialway::Train& train) const {
  //  materials->chargeTrain(train);
  //}

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

  void MaterialObject::Materials::setup() {

  }

  void MaterialObject::Materials::routeServicesTo(MaterialObject& outputObject) {
    for (Component& currComponent : components) {
      currComponent.routeServicesTo(outputObject);
    }
  }

//  void MaterialObject::Materials::chargeTrain(Materialway::Train& train) const {
//    for (const Component& currComp : components) {
//      currComp.chargeTrain(train);
//    }
//  }

  void MaterialObject::Component::build() {
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

  void MaterialObject::Component::routeServicesTo(MaterialObject& outputObject) {
    for(Element & currElement : elements) {
      outputObject.addElementIfService(currElement);
    }
    for (Component& currComponent : components) {
      currComponent.routeServicesTo(outputObject);
    }
  }

//  void MaterialObject::Component::chargeTrain(Materialway::Train& train) const {
//    for (const Component& currComp : components) {
//      currComp.chargeTrain(train);
//    }
//    for (const Element& currElem : elements) {
//      currElem.chargeTrain(train);
//    }
//  }

  const std::map<std::string, Materialway::Train::UnitType> MaterialObject::Element::unitTypeMap = {
      {"g", Materialway::Train::GRAMS},
      {"mm", Materialway::Train::MILLIMITERS},
      {"g/m", Materialway::Train::GRAMS_METERS}
  };

  /*
  void MaterialObject::Element::build() {
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

//  void MaterialObject::Element::chargeTrain(Materialway::Train& train) const {
//    if (service()) {
//      train.addWagon(elementName(), )
//  }

} /* namespace material */
