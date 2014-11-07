/**
 * @file MaterialObject.cpp
 *
 * @date 19/jun/2014
 * @author Stefano Martina
 */

//#include "Materialway.h"
#include "MaterialObject.h"
#include "ConversionStation.h"
#include "global_constants.h"
#include "MaterialTab.h"
//#include "InactiveElement.h"
#include "MaterialProperties.h"
#include "DetectorModule.h"
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
      materials_ (nullptr) {}

  const std::string MaterialObject::getTypeString() const {
    auto mapIter = typeString.find(materialType_);
    if (mapIter != typeString.end()) {
      return mapIter->second;
    } else {
      return "";
    }
  }

  double MaterialObject::totalGrams(double length, double surface) const {
    double result = 0.0;
    for (const Element* currElement : serviceElements_) {
      result += currElement->totalGrams(length, surface);
    }
    if (materials_ != nullptr) {
      result += materials_->totalGrams(length, surface);
    }
    return result;
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
            newMaterials->store(currentMaterialNode.second);
            newMaterials->build();
            materialsMap_[currentMaterialNode.first] = newMaterials;
          }
          materials_ = materialsMap_[currentMaterialNode.first];

          break;
        }
      }

    }
    cleanup();
  }

  void MaterialObject::copyServicesTo(MaterialObject& outputObject) const {
    for(const Element * currElement : serviceElements_) {
      outputObject.addElementIfService(currElement);
    }
    
    if (materials_ != nullptr) {
      materials_->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::copyServicesTo(ConversionStation& outputObject) const {
    for(const Element * currElement : serviceElements_) {
      outputObject.addElementIfService(currElement);
    }

    if (materials_ != nullptr) {
      materials_->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::copyLocalsTo(MaterialObject& outputObject) const {
    for(const Element * currElement : serviceElements_) {
      outputObject.addElementIfLocal(currElement);
    }

    if (materials_ != nullptr) {
      materials_->copyLocalsTo(outputObject);
    }
  }

  void MaterialObject::addElementIfService(const Element* inputElement) {
    if (inputElement->service() == true) {
    //std::cout << "ADDING " << inputElement->elementName() << std::endl;
      //std::cout << "COMP " << inputElement->componentName() << std::endl;
      //bool test = (inputElement->elementName().compare("SenSi") == 0);
      serviceElements_.push_back(inputElement);
    }
  }

  void MaterialObject::addElementIfLocal(const Element* inputElement) {
    if (inputElement->service() == false) {      
      serviceElements_.push_back(inputElement);
    }
  }

  void MaterialObject::addElement(const Element* inputElement) {
    serviceElements_.push_back(inputElement);
  }

  void MaterialObject::populateMaterialProperties(MaterialProperties& materialProperties) const {
    double quantity;

    for (const Element* currElement : serviceElements_) {
      //currElement.populateMaterialProperties(materialProperties);
      //populate directly because need to skip the control if is a service
      //TODO: check why componentName is not present in no Element

      
      if (currElement->debugInactivate() == false) {
        quantity = currElement->quantityInGrams(materialProperties);

        if(currElement->scale() == true) {
          quantity *= currElement->nSegments(); //nStripsAcross();
        }
        if (currElement->componentName.state()) {
          materialProperties.addLocalMass(currElement->elementName(), currElement->componentName(), quantity);
        } else {
          materialProperties.addLocalMass(currElement->elementName(), quantity);
        }
      }
    }

    if (materials_ != nullptr) {
      materials_->populateMaterialProperties(materialProperties);
    }
  }

  ElementsVector& MaterialObject::getLocalElements() const {
    ElementsVector* elementsList = new ElementsVector;
    if (materials_ != nullptr) {
      materials_->getLocalElements(*elementsList);
    }

    return *elementsList;
  }

  //void MaterialObject::chargeTrain(Materialway::Train& train) const {
  //  materials_->chargeTrain(train);
  //}

  MaterialObject::Materials::Materials() :
    componentsNode_ ("Component", parsedOnly()) {};

  double MaterialObject::Materials::totalGrams(double length, double surface) const {
    double result = 0.0;
    for (auto& currentComponentNode : components_) {
      result += currentComponentNode->totalGrams(length, surface);
    }
    return result;
  }

  void MaterialObject::Materials::build() {
        
    for (auto& currentComponentNode : componentsNode_) {
      Component* newComponent = new Component();
      newComponent->store(propertyTree());
      newComponent->store(currentComponentNode.second);
      newComponent->check();
      newComponent->build();

      components_.push_back(newComponent);
    }
    cleanup();
  }

  void MaterialObject::Materials::copyServicesTo(MaterialObject& outputObject) const {
    for (const Component* currComponent : components_) {
      currComponent->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::Materials::copyServicesTo(ConversionStation& outputObject) const {
    for (const Component* currComponent : components_) {
      currComponent->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::Materials::copyLocalsTo(MaterialObject& outputObject) const {
    for (const Component* currComponent : components_) {
      currComponent->copyLocalsTo(outputObject);
    }
  }

//  void MaterialObject::Materials::chargeTrain(Materialway::Train& train) const {
//    for (const Component& currComp : components_) {
//      currComp.chargeTrain(train);
//    }
//  }

  void MaterialObject::Materials::populateMaterialProperties(MaterialProperties& materialProperties) const {
    for (const Component* currComponent : components_) {
      currComponent->populateMaterialProperties(materialProperties);
    }
  }

  void MaterialObject::Materials::getLocalElements(ElementsVector& elementsList) const {
    for (const Component* currComponent : components_) {
      currComponent->getLocalElements(elementsList);
    }
  }

  MaterialObject::Component::Component() :
    //componentName ("componentName", parsedAndChecked()),
    componentsNode_ ("Component", parsedOnly()),
    elementsNode_ ("Element", parsedOnly()) {};

  double MaterialObject::Component::totalGrams(double length, double surface) const {
    double result = 0.0;
    for (auto& currentComponentNode : components_) {
      result += currentComponentNode->totalGrams(length, surface);
    }
    for  (auto& currentElementNode : elements_) {
      result += currentElementNode->totalGrams(length, surface);
    }
    return result;
  }

  void MaterialObject::Component::build() {
    //std::cout << "COMPONENT " << componentName() << std::endl;

    //sub components
    for (auto& currentComponentNode : componentsNode_) {
      Component* newComponent = new Component();
      newComponent->store(propertyTree());
      newComponent->store(currentComponentNode.second);
      newComponent->check();
      newComponent->build();

      components_.push_back(newComponent);
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

      elements_.push_back(newElement);
    }
    cleanup();
  }

  void MaterialObject::Component::copyServicesTo(MaterialObject& outputObject) const {
    for(const Element* currElement : elements_) {
      outputObject.addElementIfService(currElement);
    }
    for (const Component* currComponent : components_) {
      currComponent->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::Component::copyServicesTo(ConversionStation& outputObject) const {
    for(const Element* currElement : elements_) {
      outputObject.addElementIfService(currElement);
    }
    for (const Component* currComponent : components_) {
      currComponent->copyServicesTo(outputObject);
    }
  }

  void MaterialObject::Component::copyLocalsTo(MaterialObject& outputObject) const {
    for(const Element* currElement : elements_) {
      outputObject.addElementIfLocal(currElement);
    }
    for (const Component* currComponent : components_) {
      currComponent->copyLocalsTo(outputObject);
    }
  }

//  void MaterialObject::Component::chargeTrain(Materialway::Train& train) const {
//    for (const Component& currComp : components_) {
//      currComp.chargeTrain(train);
//    }
//    for (const Element& currElem : elements_) {
//      currElem.chargeTrain(train);
//    }
//  }

  void MaterialObject::Component::populateMaterialProperties(MaterialProperties& materialProperties) const {
    for (const Component* currComponent : components_) {
      currComponent->populateMaterialProperties(materialProperties);
    }
    for (const Element* currElement : elements_) {
      currElement->populateMaterialProperties(materialProperties);
    }
  }

  void MaterialObject::Component::getLocalElements(ElementsVector& elementsList) const {
    for (const Component* currComponent : components_) {
      currComponent->getLocalElements(elementsList);
    }
    for (const Element* currElement : elements_) {
      currElement->getLocalElements(elementsList);
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

  double MaterialObject::Element::quantityInGrams(const DetectorModule& module) const {
    return quantityInGrams(module.length(), module.area());
  }

  double MaterialObject::Element::quantityInGrams(const MaterialProperties& materialProperties) const {
    return quantityInGrams(materialProperties.getLength(), materialProperties.getSurface());
  }

  double MaterialObject::Element::quantityInGrams(double length, double surface) const {
    double returnVal;
    try {
      switch (unitStringMap.at(unit())) {
      case Element::GRAMS:
        returnVal = quantity();
        break;

      case Element::GRAMS_METER:
        returnVal = length * quantity() / 1000.0;
        break;

      case Element::MILLIMETERS:
        std::string elementNameString = elementName();
        double elementDensity = materialTab_.density(elementNameString);
        returnVal = elementDensity * surface * quantity() / 1000.0;
        break;
      }
    } catch (const std::out_of_range& ex) {
      logERROR(msg_no_valid_unit + unit());
    }

    return returnVal;
  }

  double MaterialObject::Element::totalGrams(const DetectorModule& module) const {
    return totalGrams(module.length(), module.area());
  }

  double MaterialObject::Element::totalGrams(const MaterialProperties& materialProperties) const {
    return totalGrams(materialProperties.getLength(), materialProperties.getSurface());
  }
  
  double MaterialObject::Element::totalGrams(double length, double surface) const {
    double quantity = quantityInGrams(length, surface);
    if(scale() == true) {
      quantity *= nSegments();
    }
    return quantity;
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

  void MaterialObject::Element::getLocalElements(ElementsVector& elementsList) const {
    if(service() == false) {
      elementsList.push_back(this);
    }
  }


} /* namespace material */
