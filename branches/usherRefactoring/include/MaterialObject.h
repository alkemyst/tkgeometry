/**
 * @file MaterialObject.h
 *
 * @date 19/giu/2014
 * @author Stefano Martina
 */

#ifndef MATERIALOBJECT_H_
#define MATERIALOBJECT_H_

#include "Property.h"
#include "Materialway.h"

namespace material {

  class MaterialTab;

  class MaterialObject : public PropertyObject {
  private:
    class Element; //forward declaration for getElementIfService(Element& inputElement)
  public:
    enum Type {MODULE, ROD};

    MaterialObject(Type materialType);
    virtual ~MaterialObject() {};

    virtual void build();

    void routeServicesTo(MaterialObject& outputObject) const;
    void addElementIfService(Element& inputElement);

    //void chargeTrain(Materialway::Train& train) const;

    //TODO: do methods for interrogate/get materials

  private:
    const MaterialTab& materialTab_;
    static const std::map<Type, const std::string> typeString;
    Type materialType_;
    ReadonlyProperty<std::string, NoDefault> type_;
    PropertyNodeUnique<std::string> materialsNode_;

    const std::string getTypeString() const;

    class Element : public PropertyObject {
    public:
      ReadonlyProperty<std::string, NoDefault> componentName; //only the inner component's name
      ReadonlyProperty<long, NoDefault> nStripAcross;
      ReadonlyProperty<long, NoDefault> nSegments;
      ReadonlyProperty<std::string, NoDefault> elementName;
      ReadonlyProperty<bool, NoDefault> service;
      ReadonlyProperty<bool, NoDefault> scale;
      ReadonlyProperty<double, NoDefault> quantity;
      ReadonlyProperty<std::string, NoDefault> unit;
      Element() :
        componentName ("componentName", parsedOnly()),
        nStripAcross("nStripAcross", parsedOnly()),
        nSegments("nSegments", parsedOnly()),
        elementName ("elementName", parsedAndChecked()),
        service ("service", parsedAndChecked()),
        scale ("scale", parsedAndChecked()),
        quantity ("quantity", parsedAndChecked()),
        unit ("unit", parsedAndChecked()) {};
      virtual ~Element() {};
      //void build();
      //void chargeTrain(Materialway::Train& train) const;
    private:
      static const std::map<std::string, Materialway::Train::UnitType> unitTypeMap;
    };

    class Component : public PropertyObject {
    public:
      ReadonlyProperty<std::string, NoDefault> componentName;
      PropertyNodeUnique<std::string> componentsNode_;
      PropertyNodeUnique<std::string> elementsNode_;
      Component() :
        componentName ("componentName", parsedAndChecked()),
        componentsNode_ ("Component", parsedOnly()),
        elementsNode_ ("Element", parsedOnly()) {};
      virtual ~Component() {};
      void build();
      void routeServicesTo(MaterialObject& outputObject);
      //void chargeTrain(Materialway::Train& train) const;

      PtrVector<Component> components;
      PtrVector<Element> elements;
    };

    class Materials : public PropertyObject {
    public:
      PropertyNodeUnique<std::string> componentsNode_;
      //Property<double, Computable> radiationLength, interactionLenght;
      Materials() :
        componentsNode_ ("Component", parsedOnly()) {};
      virtual ~Materials() {};
      void build();
      void setup();
      void routeServicesTo(MaterialObject& outputObject);
      //void chargeTrain(Materialway::Train& train) const;

      PtrVector<Component> components;
    };

    //ATTENTION: Materials objects of the same structure are shared between MaterialObject objects
    //   of the modules/layer/etc.. (for containing memory use).
    //   This is not for service routing objects.
    Materials * materials;

    PtrVector<Element> serviceElements; //used for MaterialObject not from config file (service routing)
  };
} /* namespace material */

#endif /* MATERIALOBJECT_H_ */
