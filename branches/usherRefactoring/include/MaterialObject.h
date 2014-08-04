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
  protected:
    class Element; //forward declaration for getElementIfService(Element& inputElement)
  public:
    enum Type {MODULE, ROD};

    MaterialObject(Type materialType);
    virtual ~MaterialObject() {};

    virtual void build();

    virtual void routeServicesTo(MaterialObject& outputObject) const;
    void addElementIfService(Element& inputElement);
    void populateInactiveElement(InactiveElement& inactiveElement);


    //void chargeTrain(Materialway::Train& train) const;

    //TODO: do methods for interrogate/get materials

  protected:
    static const std::map<Type, const std::string> typeString;
    Type materialType_;
    ReadonlyProperty<std::string, NoDefault> type_;
    PropertyNodeUnique<std::string> materialsNode_;

    const std::string getTypeString() const;

    class Element : public PropertyObject {
    public:
      enum Unit{GRAMS, MILLIMETERS, GRAMS_METER};
      //static const std::map<Unit, const std::string> unitString;
      static const std::map<std::string, Unit> unitStringMap;

      ReadonlyProperty<std::string, NoDefault> componentName; //only the inner component's name
      ReadonlyProperty<long, NoDefault> nStripAcross;
      ReadonlyProperty<long, NoDefault> nSegments;
      ReadonlyProperty<std::string, NoDefault> elementName;
      ReadonlyProperty<bool, NoDefault> service;
      ReadonlyProperty<bool, NoDefault> scale;
      ReadonlyProperty<double, NoDefault> quantity;
      ReadonlyProperty<std::string, NoDefault> unit;

      Element();
      virtual ~Element() {};
      //void build();
      //void chargeTrain(Materialway::Train& train) const;
      double quantityInGrams(InactiveElement& inactiveElement);
      void populateInactiveElement(InactiveElement& inactiveElement);
    private:
      const MaterialTab& materialTab_;
      static const std::string msg_no_valid_unit;
      //static const std::map<std::string, Materialway::Train::UnitType> unitTypeMap;
    };

  private:

    class Component : public PropertyObject {
    public:
      ReadonlyProperty<std::string, NoDefault> componentName;
      PropertyNodeUnique<std::string> componentsNode_;
      PropertyNodeUnique<std::string> elementsNode_;
      Component();
      virtual ~Component() {};
      void build();
      void routeServicesTo(MaterialObject& outputObject);
      //void chargeTrain(Materialway::Train& train) const;
      void populateInactiveElement(InactiveElement& inactiveElement);

      PtrVector<Component> components;
      PtrVector<Element> elements;
    };

    class Materials : public PropertyObject {
    public:
      PropertyNodeUnique<std::string> componentsNode_;
      //Property<double, Computable> radiationLength, interactionLenght;
      Materials();
      virtual ~Materials() {};
      void build();
      void setup();
      void routeServicesTo(MaterialObject& outputObject);
      //void chargeTrain(Materialway::Train& train) const;
      void populateInactiveElement(InactiveElement& inactiveElement);

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
