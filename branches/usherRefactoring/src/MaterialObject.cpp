/**
 * @file MaterialObject.cpp
 *
 * @date 19/giu/2014
 * @author Stefano Martina
 */

#include "MaterialObject.h"
#include "global_constants.h"
#include "MaterialTab.h"
#include "InactiveElement.h"
#include <messageLogger.h>
#include <stdexcept>





namespace material {
  const std::map<MaterialObject::Type, const std::string> MaterialObject::typeString = {
      {MODULE, "module"},
      {ROD, "rod"}
  };

  MaterialObject::MaterialObject(Type materialType) :
      materialType_ (materialType),
      type_ ("type", parsedAndChecked()),
      materialsNode_ ("Materials", parsedOnly()),
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

  void MaterialObject::populateInactiveElement(InactiveElement& inactiveElement) {
    for (Element& currElement : serviceElements) {
      //currElement.populateInactiveElement(inactiveElement);
      //populate directly because need to skip the control if is a service
      inactiveElement.addLocalMass(currElement.elementName(), currElement.componentName(), currElement.quantityInGrams(inactiveElement));
    }

    if (materials != nullptr) {
      materials->populateInactiveElement(inactiveElement);
    }
  }


  //void MaterialObject::chargeTrain(Materialway::Train& train) const {
  //  materials->chargeTrain(train);
  //}

  MaterialObject::Materials::Materials() :
          componentsNode_ ("Component", parsedOnly()) {};

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

  void MaterialObject::Materials::populateInactiveElement(InactiveElement& inactiveElement) {
    for (Component & currComponent : components) {
      currComponent.populateInactiveElement(inactiveElement);
    }
  }

  MaterialObject::Component::Component() :
          componentName ("componentName", parsedAndChecked()),
          componentsNode_ ("Component", parsedOnly()),
          elementsNode_ ("Element", parsedOnly()) {};

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

  void MaterialObject::Component::populateInactiveElement(InactiveElement& inactiveElement) {
    for (Component & currComponent : components) {
      currComponent.populateInactiveElement(inactiveElement);
    }
    for (Element & currElement : elements) {
      currElement.populateInactiveElement(inactiveElement);
    }
  }

  /*
  const std::map<MaterialObject::Type, const std::string> MaterialObject::Element::unitString = {
      {GRAMS, "g"},
      {MILLIMETERS, "mm"},
      {GRAMS_METER, "gm"}
  };
  */

  MaterialObject::Element::Element() :
          componentName ("componentName", parsedOnly()),
          nStripAcross("nStripAcross", parsedOnly()),
          nSegments("nSegments", parsedOnly()),
          elementName ("elementName", parsedAndChecked()),
          service ("service", parsedAndChecked()),
          scale ("scale", parsedAndChecked()),
          quantity ("quantity", parsedAndChecked()),
          unit ("unit", parsedAndChecked()),
          materialTab_ (MaterialTab::instance()) {};

  const std::string MaterialObject::Element::msg_no_valid_unit = "No valid unit: ";

  const std::map<std::string, MaterialObject::Element::Unit> MaterialObject::Element::unitStringMap = {
      {"g", GRAMS},
      {"mm", MILLIMETERS},
      {"g/m", GRAMS_METER}
  };

  double MaterialObject::Element::quantityInGrams(InactiveElement& inactiveElement) {
    double returnVal;
    try {
      switch (unitStringMap.at(unit())) {
      case Element::GRAMS:
        returnVal = quantity();
        break;

      case Element::GRAMS_METER:
        returnVal = inactiveElement.getLength() * quantity() / 1000.0;
        break;

      case Element::MILLIMETERS:
        returnVal = materialTab_.density(elementName()) * inactiveElement.getSurface() * quantity() / 1000.0;
        break;
      }
    } catch (const std::out_of_range& ex) {
      logERROR(msg_no_valid_unit + unit());
    }

    return returnVal;
  }

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

  void MaterialObject::Element::populateInactiveElement(InactiveElement& inactiveElement) {
    if(service() == false) {
      inactiveElement.addLocalMass(elementName(), componentName(), quantityInGrams(inactiveElement));
    }
  }

} /* namespace material */
