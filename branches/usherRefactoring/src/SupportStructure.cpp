/**
 * @file SupportStructure.cpp
 *
 * @date 25/Nov/2014
 * @author Stefano Martina
 */

#include "SupportStructure.h"
#include "MaterialTab.h"
#include "messageLogger.h"
#include "MaterialProperties.h"
#include "InactiveElement.h"
#include "InactiveTube.h"
#include "InactiveRing.h"

using insur::InactiveTube;
using insur::InactiveRing;

namespace material {
  //=============== begin class SupportStructure
  const std::map<std::string, SupportStructure::Type> SupportStructure::typeStringMap = {
    {"custom", CUSTOM},
    {"auto", AUTO}
  };
  const std::map<std::string, SupportStructure::Direction> SupportStructure::directionStringMap = {
    {"horizontal", HORIZONTAL},
    {"vertical", VERTICAL}
  };


  SupportStructure::SupportStructure() :
    componentsNode("Component", parsedOnly()),
    type("type", parsedAndChecked()),
    autoPosition("autoPosition", parsedOnly()),
    customZMin("customZMin", parsedOnly()),
    customRMin("customRMin", parsedOnly()),
    customLength("customLength", parsedOnly()),
    customDir("customDir", parsedOnly())
  {}
  
  void SupportStructure::build() {
    for (auto& currentComponentNode : componentsNode) {
      Component* newComponent = new Component();
      newComponent->store(propertyTree());
      newComponent->store(currentComponentNode.second);
      newComponent->check();
      newComponent->build();

      components_.push_back(newComponent);
    }

    try {
      supportType_ = typeStringMap.at(type());
    }  catch (const std::out_of_range& ex) {
      logERROR("Unrecognized value " + type() + ".");
      return;
    }

    if (supportType_ == CUSTOM) {
      if (customZMin.state() && customRMin.state() && customLength.state() && customDir.state()) {
        try {
          direction_ = directionStringMap.at(customDir());
        }  catch (const std::out_of_range& ex) {
          logERROR("Unrecognized value " + customDir() + ".");
          return;
        }

        if (direction_ == HORIZONTAL) {
          InactiveTube* tube = new InactiveTube;
          tube->setZLength(customLength());
          tube->setZOffset(customZMin());
          tube->setInnerRadius(customRMin());
          tube->setRWidth(inactiveElementWidth);
          tube->setFinal(true);
          tube->setCategory(insur::MaterialProperties::u_sup);
          inactiveElement_ = tube;
        } else {
          InactiveRing* ring = new InactiveRing;
          ring->setZLength(inactiveElementWidth);
          ring->setZOffset(customZMin());
          ring->setInnerRadius(customRMin());
          ring->setRWidth(customLength());
          ring->setFinal(true);
          ring->setCategory(insur::MaterialProperties::u_sup);
          inactiveElement_ = ring;
        }
      } else {
        logERROR("Property customZMin, customRMin, customLength, or customDir not set.");
        return;
      }
    } else if (supportType_ == AUTO) { //The AUTO supports are only for barrels, so is vertical
      direction_ = VERTICAL;
      //TODO: FINISH!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    }

    populateMaterialProperties(*inactiveElement_);
        
    cleanup();
  }

  InactiveElement* SupportStructure::inactiveElement() {
    return inactiveElement_;
  }
    
  void SupportStructure::populateMaterialProperties(MaterialProperties& materialProperties) const {
    for (const Component* currComponent : components_) {
      currComponent->populateMaterialProperties(materialProperties);
    }
  }

  //=============== end class SupportStructure

  //=============== begin class SupportStructure::Component
  SupportStructure::Component::Component() :
    componentsNode("Component", parsedOnly()),
    elementsNode("Element", parsedOnly()) {}

  void SupportStructure::Component::build() {
    //sub components
    for (auto& currentComponentNode : componentsNode) {
      Component* newComponent = new Component();
      newComponent->store(propertyTree());
      newComponent->store(currentComponentNode.second);
      newComponent->check();
      newComponent->build();

      components_.push_back(newComponent);
    }
    //elements
    for (auto& currentElementNode : elementsNode) {
      Element* newElement = new Element();
      newElement->store(propertyTree());
      newElement->store(currentElementNode.second);
      newElement->check();
      newElement->cleanup();

      elements_.push_back(newElement);
    }
    cleanup();
  }
  void SupportStructure::Component::populateMaterialProperties(MaterialProperties& materialProperties) const {
    for (const Component* currComponent : components_) {
      currComponent->populateMaterialProperties(materialProperties);
    }
    for (const Element* currElement : elements_) {
      currElement->populateMaterialProperties(materialProperties);
    }
  }

  //=============== end class SupportStructure::Component
  
  //=============== begin class SupportStructure::Element
  SupportStructure::Element::Element() :
    componentName ("componentName", parsedOnly()),
    elementName ("elementName", parsedAndChecked()),
    quantity ("quantity", parsedAndChecked()),
    unit ("unit", parsedAndChecked()),
    debugInactivate ("debugInactivate", parsedOnly(), false),
    materialTab_ (MaterialTab::instance()) {}
    
  const std::string SupportStructure::Element::msg_no_valid_unit = "No valid unit: ";

  const std::map<std::string, SupportStructure::Element::Unit> SupportStructure::Element::unitStringMap = {
      {"g", GRAMS},
      {"mm", MILLIMETERS},
      {"g/m", GRAMS_METER}
  };

  double SupportStructure::Element::quantityInGrams(double length, double surface) const {
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
        returnVal = elementDensity * surface * quantity();
        break;
      }
    } catch (const std::out_of_range& ex) {
      logERROR(msg_no_valid_unit + unit());
    }

    return returnVal;
  }

  void SupportStructure::Element::populateMaterialProperties(MaterialProperties& materialProperties) const {
    double quantity;
    
    if(debugInactivate() == false) {
      quantity = quantityInGrams(materialProperties.getLength(), materialProperties.getSurface());
      materialProperties.addLocalMass(elementName(), componentName(), quantity);
    }
  }
  
  //=============== end class SupportStructure::Element
}

