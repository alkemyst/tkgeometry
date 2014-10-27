/**
 * @file MaterialObject.cpp
 *
 * @date 19/giu/2014
 * @author Stefano Martina
 */

//#include "Materialway.h"
#include "MaterialObject.h"
#include "ConversionStation.h"
#include "global_constants.h"
#include "MaterialTab.h"
//#include "InactiveElement.h"
#include "MaterialProperties.h"
#include <messageLogger.h>
#include <stdexcept>





namespace material {
  const std::map<MaterialObject::Type, const std::string> MaterialObject::typeString = {
      {MODULE, "module"},
      {ROD, "rod"},
      {LAYER, "layer"}
  };

  MaterialObject::MaterialObject(Type materialType) :
      materialType_ (materialType),
      type_ ("type", parsedAndChecked()),
      debugInactivate_ ("debugInactivate", parsedOnly(), false),
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
    if (!debugInactivate_()) {
 
      static std::map<std::string, Materials*> materialsMap_; //for saving memory

      //std::cout << "Materials " << materialsNode_.size() << std::endl;

      for (auto& currentMaterialNode : materialsNode_) {
        store(currentMaterialNode.second);
        check();
        if (type_().compare(getTypeString()) == 0) {
          if (materialsMap_.count(currentMaterialNode.first) == 0) {
            Materials * newMaterials  = new Materials();
            newMaterials->store(propertyTree());
            newMaterials->store(currentMaterialNode.second);
            newMaterials->build();
            materialsMap_[currentMaterialNode.first] = newMaterials;
          }
          materials = materialsMap_[currentMaterialNode.first];

          break;
        }
      }

    }
    cleanup();
  }

  void MaterialObject::copyServicesTo(MaterialObject& outputObject) const {
    for(const Element * currElement : serviceElements) {
      outputObject.addElementIfService(currElement);
    }
    
    if (materials != nullptr) {
      materials->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::copyServicesTo(ConversionStation& outputObject) const {
    for(const Element * currElement : serviceElements) {
      outputObject.addElementIfService(currElement);
    }

    if (materials != nullptr) {
      materials->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::copyLocalsTo(MaterialObject& outputObject) const {
    for(const Element * currElement : serviceElements) {
      outputObject.addElementIfLocal(currElement);
    }

    if (materials != nullptr) {
      materials->copyLocalsTo(outputObject);
    }
  }

  void MaterialObject::addElementIfService(const Element* inputElement) {
    if (inputElement->service() == true) {
      //std::cout << "ADDING " << inputElement->elementName() << std::endl;
      //std::cout << "COMP " << inputElement->componentName() << std::endl;
      //bool test = (inputElement->elementName().compare("SenSi") == 0);
      serviceElements.push_back(inputElement);
    }
  }

  void MaterialObject::addElementIfLocal(const Element* inputElement) {
    if (inputElement->service() == false) {
      serviceElements.push_back(inputElement);
    }
  }

  void MaterialObject::addElement(const Element* inputElement) {
    serviceElements.push_back(inputElement);
  }

  void MaterialObject::populateMaterialProperties(MaterialProperties& materialProperties) const {
    double quantity;

    for (const Element* currElement : serviceElements) {
      //currElement.populateMaterialProperties(materialProperties);
      //populate directly because need to skip the control if is a service
      //TODO: check why componentName is not present in no Element
      if (currElement->debugInactivate() == false) {
        quantity = currElement->quantityInGrams(materialProperties);
        if(currElement->scale() == true) {
          quantity *= currElement->nSegments(); //nStripsAcross();
        }
        if (currElement->componentName.state()) {
          materialProperties.addLocalMass(currElement->elementName(), currElement->componentName(), currElement->quantityInGrams(materialProperties));
        } else {
          materialProperties.addLocalMass(currElement->elementName(), currElement->quantityInGrams(materialProperties));
        }
      }
    }

    if (materials != nullptr) {
      materials->populateMaterialProperties(materialProperties);
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

  void MaterialObject::Materials::copyServicesTo(MaterialObject& outputObject) const {
    for (const Component* currComponent : components) {
      currComponent->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::Materials::copyServicesTo(ConversionStation& outputObject) const {
    for (const Component* currComponent : components) {
      currComponent->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::Materials::copyLocalsTo(MaterialObject& outputObject) const {
    for (const Component* currComponent : components) {
      currComponent->copyLocalsTo(outputObject);
    }
  }

//  void MaterialObject::Materials::chargeTrain(Materialway::Train& train) const {
//    for (const Component& currComp : components) {
//      currComp.chargeTrain(train);
//    }
//  }

  void MaterialObject::Materials::populateMaterialProperties(MaterialProperties& materialProperties) const {
    for (const Component* currComponent : components) {
      currComponent->populateMaterialProperties(materialProperties);
    }
  }

  MaterialObject::Component::Component() :
    //componentName ("componentName", parsedAndChecked()),
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
      newElement->build();
      //bool test1 = newElement->componentName.state();
      //bool test2 = newElement->nSegments.state();

      elements.push_back(newElement);
    }
    cleanup();
  }

  void MaterialObject::Component::copyServicesTo(MaterialObject& outputObject) const {
    for(const Element* currElement : elements) {
      outputObject.addElementIfService(currElement);
    }
    for (const Component* currComponent : components) {
      currComponent->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::Component::copyServicesTo(ConversionStation& outputObject) const {
    for(const Element* currElement : elements) {
      outputObject.addElementIfService(currElement);
    }
    for (const Component* currComponent : components) {
      currComponent->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::Component::copyLocalsTo(MaterialObject& outputObject) const {
    for(const Element* currElement : elements) {
      outputObject.addElementIfLocal(currElement);
    }
    for (const Component* currComponent : components) {
      currComponent->copyLocalsTo(outputObject);
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

  void MaterialObject::Component::populateMaterialProperties(MaterialProperties& materialProperties) const {
    for (const Component* currComponent : components) {
      currComponent->populateMaterialProperties(materialProperties);
    }
    for (const Element* currElement : elements) {
      currElement->populateMaterialProperties(materialProperties);
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
    destination ("destination", parsedOnly()),
    componentName ("componentName", parsedOnly()),
    nStripsAcross("nStripsAcross", parsedOnly()),
    nSegments("nSegments", parsedOnly()),
    elementName ("elementName", parsedAndChecked()),
    service ("service", parsedOnly(), false),
    scale ("scale", parsedOnly(), false),
    quantity ("quantity", parsedAndChecked()),
    unit ("unit", parsedAndChecked()),
    debugInactivate ("debugInactivate", parsedOnly(), false),
    materialTab_ (MaterialTab::instance()) {};

  MaterialObject::Element::Element(const Element& original, double multiplier) : Element() {
    if(original.destination.state())
      destination(original.destination());
    if(original.componentName.state())
      componentName(original.componentName());
    if(original.nStripsAcross.state())
      nStripsAcross(original.nStripsAcross());
    if(original.nSegments.state())
      nSegments(original.nSegments());
    elementName(original.elementName());
    service(original.service());
    scale(original.scale());
    quantity(original.quantity() * multiplier);
    unit(original.unit());
    debugInactivate(original.debugInactivate());
  }

  const std::string MaterialObject::Element::msg_no_valid_unit = "No valid unit: ";

  const std::map<std::string, MaterialObject::Element::Unit> MaterialObject::Element::unitStringMap = {
      {"g", GRAMS},
      {"mm", MILLIMETERS},
      {"g/m", GRAMS_METER}
  };

  double MaterialObject::Element::quantityInGrams(MaterialProperties& materialProperties) const {
    double returnVal;
    try {
      switch (unitStringMap.at(unit())) {
      case Element::GRAMS:
        returnVal = quantity();
        break;

      case Element::GRAMS_METER:
        returnVal = materialProperties.getLength() * quantity() / 1000.0;
        break;

      case Element::MILLIMETERS:
        std::string elementNameString = elementName();
        double elementDensity = materialTab_.density(elementNameString);
        double elementSurface =  materialProperties.getSurface();
        returnVal = elementDensity * elementSurface * quantity() / 1000.0;
        break;
      }
    } catch (const std::out_of_range& ex) {
      logERROR(msg_no_valid_unit + unit());
    }

    return returnVal;
  }

  void MaterialObject::Element::build() {
    /*
    std::cout << "  ELEMENT " << elementName() << std::endl;
    std::cout << "    DATA "
        << " componentName " << (componentName.state() ? componentName() : "NOT_SET")
        << " nSegments " << (nSegments.state() ? std::to_string(nSegments()) : "NOT_SET")
        << " exiting " << service()
        << " scale " << scale()
        << " quantity " << quantity()
        << " unit " << unit()
        << " station " << (destination.state() ? destination() : "NOT_SET")
        << std::endl;
    */
  }

//  void MaterialObject::Element::chargeTrain(Materialway::Train& train) const {
//    if (service()) {
//      train.addWagon(elementName(), )
//  }

  void MaterialObject::Element::populateMaterialProperties(MaterialProperties& materialProperties) const {
    double quantity;

    if(debugInactivate() == false) {
      if(service() == false) {
        quantity = quantityInGrams(materialProperties);
        if(scale() == true) {
          quantity *= nSegments(); //nStripsAcross();
        }
        materialProperties.addLocalMass(elementName(), componentName(), quantity);
      }
    }
  }

} /* namespace material */
