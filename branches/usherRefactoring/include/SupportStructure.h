/**
 * @file SupportStructure.h
 *
 * @date 25/Nov/2014
 * @author Stefano Martina
 */

#ifndef SUPPORTSTRUCTURE_H_
#define SUPPORTSTRUCTURE_H_

#include <vector>
#include <string>
#include "Property.h"

namespace insur {
  class MaterialProperties;
  class InactiveElement;
}
using insur::MaterialProperties;
using insur::InactiveElement;

namespace material {
  class MaterialTab;

  class SupportStructure : public PropertyObject {
  private:
    class Component;
    class Element;

  public:
    enum Type {CUSTOM, AUTO};
    enum Direction {HORIZONTAL, VERTICAL};

    static const std::map<std::string, Type> typeStringMap;
    static const std::map<std::string, Direction> directionStringMap;

    typedef std::vector<const Component*> ComponentsVector;
    typedef std::vector<const Element*> ElementsVector;

    Property<std::string, NoDefault> type;
    Property<double, NoDefault> autoPosition;
    Property<double, NoDefault> customZMin;
    Property<double, NoDefault> customRMin;
    Property<double, NoDefault> customLength;
    Property<std::string, NoDefault> customDir;
    
    SupportStructure();
    virtual ~SupportStructure() {};
    void build();

    InactiveElement* inactiveElement();
    
  private:
    const double inactiveElementWidth = 1.;
    PropertyNodeUnique<std::string> componentsNode;

    ComponentsVector components_;
    InactiveElement* inactiveElement_;
    Type supportType_;
    Direction direction_;

    void populateMaterialProperties(MaterialProperties& materialPropertie) const;



    class Component : public PropertyObject {
    public:
      PropertyNodeUnique<std::string> componentsNode;
      PropertyNodeUnique<std::string> elementsNode;
      Component();
      virtual ~Component() {};

      void build();
      void populateMaterialProperties(MaterialProperties& materialPropertie) const;

      ComponentsVector components_;
      ElementsVector elements_;
    };
    
    class Element : public PropertyObject {
    public:
      enum Unit{GRAMS, MILLIMETERS, GRAMS_METER};
      static const std::map<std::string, Unit> unitStringMap;
      Property<std::string, NoDefault> componentName; //only the inner component's name
      Property<std::string, NoDefault> elementName;
      Property<double, NoDefault> quantity;
      Property<std::string, NoDefault> unit;
      Property<bool, Default> debugInactivate;

      Element();
      virtual ~Element() {};

      double quantityInGrams(double length, double surface) const;
      void populateMaterialProperties(MaterialProperties& materialPropertie) const;
    private:
      const MaterialTab& materialTab_;
      static const std::string msg_no_valid_unit;
    };
  };
}

#endif // SUPPORTSTRUCTURE_H_
