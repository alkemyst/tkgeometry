/**
 * @file Station.cpp
 *
 * @date Jul 21, 2014
 * @author Stefano Martina
 */

#include "ConversionStation.h"
#include "MaterialObject.h"

namespace material {

  const std::map<ConversionStation::Type, const std::string> ConversionStation::typeString = {
      {FLANGE, "flange"},
      {ENDCAP, "endcap"}
  };

  const std::string ConversionStation::getTypeString() const {
    auto mapIter = typeString.find(stationType_);
    if (mapIter != typeString.end()) {
      return mapIter->second;
    } else {
      return "";
    }
  }

  void ConversionStation::build() {
    for (auto& currentStationNode : stationsNode_) {
      store(currentStationNode.second);
      check();
      if (type_().compare(getTypeString()) == 0) {
        conversionsNode_.clear();
        store(currentStationNode.second);
        buildConversions();
        break;
      }
    }
    cleanup();
  }

  void ConversionStation::routeServicesTo(MaterialObject& outputObject) const {

  }

  void ConversionStation::buildConversions() {
    //std::cout << "STATION" << std::endl;

    for (auto& currentConversionNode : conversionsNode_) {
      Conversion* newConversion = new Conversion();
      newConversion->store(propertyTree());
      newConversion->store(currentConversionNode.second);
      newConversion->check();
      newConversion->build();

      conversions.push_back(newConversion);
    }
  }

  void ConversionStation::addElementIfService(const MaterialObject::Element* inputElement) {
      if (inputElement->service() == true) {
        inputElements.push_back(inputElement);
      }
    }

  void ConversionStation::Conversion::build() {
    //std::cout << "  CONVERSION" << std::endl;

    if (inputNode_.size() > 0) {
      Inoutput* newInput = new Inoutput();
      newInput->store(propertyTree());
      newInput->store(inputNode_.begin()->second);
      newInput->check();
      newInput->build();

      inputs.push_back(newInput);
    }

    if (outputNode_.size() > 0) {
      Inoutput* newOutput = new Inoutput();
      newOutput->store(propertyTree());
      newOutput->store(outputNode_.begin()->second);
      newOutput->check();
      newOutput->build();

      outputs.push_back(newOutput);
    }
    cleanup();
  }

  void ConversionStation::Inoutput::build() {
    //std::cout << "    INPUT/OUTPUT" << std::endl;

    for  (auto& currentElementNode : elementsNode_) {
      Element* newElement = new Element();
      newElement->store(propertyTree());
      newElement->store(currentElementNode.second);
      newElement->check();
      //newElement->build();
      newElement->cleanup();

      elements.push_back(newElement);
    }
    cleanup();
  }

  /*
  void ConversionStation::Element::build() {
    std::cout << "      ELEMENT -> "
        << " elementName " << elementName()
        << "; quantity " << quantity()
        << "; unit " << unit()
        << "; service " << (service.state() ? std::to_string(service()) : "NOT_SET" )
        << std::endl;
  }
  */

} /* namespace material */
