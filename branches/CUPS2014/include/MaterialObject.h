/**
 * @file MaterialObject.h
 *
 * @date 19/Jun/2014
 * @author Stefano Martina
 */

#ifndef MATERIALOBJECT_H_
#define MATERIALOBJECT_H_

#include "Property.h"
//#include "Materialway.h"

namespace insur {
  class MaterialProperties;
}

using insur::MaterialProperties;

namespace material {

  class MaterialTab;
  class ConversionStation;

  class MaterialObject : public PropertyObject {
  public:
    class Element; //forward declaration for getElementIfService(Element& inputElement)
  public:
    enum Type {MODULE, ROD, LAYER, SERVICE};

    MaterialObject(Type materialType);
    virtual ~MaterialObject() {};

    virtual void build();

    virtual void copyServicesTo(MaterialObject& outputObject) const;
    virtual void copyServicesTo(ConversionStation& outputObject) const;
    void copyLocalsTo(MaterialObject& outputObject) const;
    void addElementIfService(const Element* inputElement);
    void addElementIfLocal(const Element* inputElement);
    void addElement(const Element* inputElement);
    void populateMaterialProperties(MaterialProperties& materialProperties) const;


    //void chargeTrain(Materialway::Train& train) const;

    //TODO: do methods for interrogate/get materials

  private:
    static const std::map<Type, const std::string> typeString;
    Type materialType_;
    ReadonlyProperty<std::string, NoDefault> type_;
    ReadonlyProperty<bool, Default> debugInactivate_;
    PropertyNodeUnique<std::string> materialsNode_;

    const std::string getTypeString() const;

  public:
    class Element : public PropertyObject {
    public:
      enum Unit{GRAMS, MILLIMETERS, GRAMS_METER};
      //static const std::map<Unit, const std::string> unitString;
      static const std::map<std::string, Unit> unitStringMap;

      Property<std::string, NoDefault> componentName; //only the inner component's name
      Property<long, NoDefault> nStripsAcross;
      Property<long, NoDefault> nSegments;
      Property<std::string, NoDefault> elementName;
      Property<bool, Default> service;
      Property<bool, Default> scale;
      Property<double, NoDefault> quantity;
      Property<std::string, NoDefault> unit;
      Property<bool, Default> debugInactivate;
      Property<std::string, NoDefault> destination;

      Element();
      Element(const Element& original, double multiplier = 1.0);
      //Element(const Element& originElement);

      virtual ~Element() {};
      void build();
      //void chargeTrain(Materialway::Train& train) const;
      double quantityInGrams(MaterialProperties& materialProperties) const;
      void populateMaterialProperties(MaterialProperties& materialProperties) const;
    private:
      const MaterialTab& materialTab_;
      static const std::string msg_no_valid_unit;
      //static const std::map<std::string, Materialway::Train::UnitType> unitTypeMap;
    };

  private:

    class Component : public PropertyObject {
    public:
      //Property<std::string, NoDefault> componentName;
      PropertyNodeUnique<std::string> componentsNode_;
      PropertyNodeUnique<std::string> elementsNode_;
      Component();
      virtual ~Component() {};
      void build();
      void copyServicesTo(MaterialObject& outputObject) const;
      void copyServicesTo(ConversionStation& outputObject) const;
      void copyLocalsTo(MaterialObject& outputObject) const;
      //void chargeTrain(Materialway::Train& train) const;
      void populateMaterialProperties(MaterialProperties& materialPropertie) const;

      std::vector<const Component*> components;
      std::vector<const Element*> elements;
    };

    class Materials : public PropertyObject {
    public:
      PropertyNodeUnique<std::string> componentsNode_;
      //Property<double, Computable> radiationLength, interactionLenght;
      Materials();
      virtual ~Materials() {};
      void build();
      void setup();
      void copyServicesTo(MaterialObject& outputObject) const;
      void copyServicesTo(ConversionStation& outputObject) const;
      void copyLocalsTo(MaterialObject& outputObject) const;
      //void chargeTrain(Materialway::Train& train) const;
      void populateMaterialProperties(MaterialProperties& materialProperties) const;

      std::vector<const Component*> components;
    };

    //ATTENTION: Materials objects of the same structure are shared between MaterialObject objects
    //   of the modules/layer/etc.. (for containing memory use).
    //   This is not for service routing objects.
    Materials * materials;

    std::vector<const Element*> serviceElements; //used for MaterialObject not from config file (service routing)
  };
} /* namespace material */

#endif /* MATERIALOBJECT_H_ */
