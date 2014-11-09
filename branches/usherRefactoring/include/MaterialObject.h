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

class DetectorModule;

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
    class Component;

    typedef std::vector<const Component*> ComponentsVector;
    typedef std::vector<const Element*> ElementsVector;

    enum Type {MODULE, ROD, LAYER, SERVICE};

    MaterialObject(Type materialType);
    virtual ~MaterialObject() {};

    double totalGrams(double length, double surface) const;

    virtual void build();

    virtual void copyServicesTo(MaterialObject& outputObject) const;
    virtual void copyServicesTo(ConversionStation& outputObject) const;
    void copyLocalsTo(MaterialObject& outputObject) const;
    void addElementIfService(const Element* inputElement);
    void addElementIfLocal(const Element* inputElement);
    void addElement(const Element* inputElement);
    void populateMaterialProperties(MaterialProperties& materialProperties) const;

    ElementsVector& getLocalElements() const;

    //TODO: do methods for interrogate/get materials

  private:
    static const std::map<Type, const std::string> typeString;
    Type materialType_;
    ReadonlyProperty<std::string, NoDefault> type_;
    ReadonlyProperty<bool, Default> debugInactivate_;
    PropertyNodeUnique<std::string> materialsNode_;
    PropertyNode<int> sensorNode;

    const std::string getTypeString() const;

  public:
    class Element : public PropertyObject {
    public:
      enum Unit{GRAMS, MILLIMETERS, GRAMS_METER};
      //static const std::map<Unit, const std::string> unitString;
      static const std::map<std::string, Unit> unitStringMap;

      Property<std::string, NoDefault> componentName; //only the inner component's name
      Property<long, NoDefault> numStripsAcross; //the real strips and segments of sensor
      Property<long, NoDefault> numSegments;
      Property<long, NoDefault> nStripsAcross; //the reference strips and segments for scaling
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
      double quantityInGrams(const DetectorModule& module) const;
      double quantityInGrams(const MaterialProperties& materialProperties) const;
      double quantityInGrams(double length, double surface) const;
      double totalGrams(const DetectorModule& module) const;
      double totalGrams(const MaterialProperties& materialProperties) const;
      double totalGrams(double length, double surface) const;
      void populateMaterialProperties(MaterialProperties& materialProperties) const;
      void getLocalElements(ElementsVector& elementsList) const;

    private:
      const MaterialTab& materialTab_;
      static const std::string msg_no_valid_unit;
      //static const std::map<std::string, Materialway::Train::UnitType> unitTypeMap;
    };

    class Component : public PropertyObject {
    public:
      //Property<std::string, NoDefault> componentName;
      PropertyNodeUnique<std::string> componentsNode_;
      PropertyNodeUnique<std::string> elementsNode_;
      Component();
      virtual ~Component() {};
      double totalGrams(double length, double surface) const;
      void build();
      void copyServicesTo(MaterialObject& outputObject) const;
      void copyServicesTo(ConversionStation& outputObject) const;
      void copyLocalsTo(MaterialObject& outputObject) const;
      //void chargeTrain(Materialway::Train& train) const;
      void populateMaterialProperties(MaterialProperties& materialPropertie) const;
      void getLocalElements(ElementsVector& elementsList) const;

      ComponentsVector components_;
      ElementsVector elements_;
    };

    class Materials : public PropertyObject {
    public:
      PropertyNodeUnique<std::string> componentsNode_;
      //Property<double, Computable> radiationLength, interactionLenght;
      Materials();
      virtual ~Materials() {};
      double totalGrams(double length, double surface) const;
      void build();
      void copyServicesTo(MaterialObject& outputObject) const;
      void copyServicesTo(ConversionStation& outputObject) const;
      void copyLocalsTo(MaterialObject& outputObject) const;
      //void chargeTrain(Materialway::Train& train) const;
      void populateMaterialProperties(MaterialProperties& materialProperties) const;
      void getLocalElements(ElementsVector& elementsList) const;

      ComponentsVector components_;
    };

    //ATTENTION: Materials objects of the same structure are shared between MaterialObject objects
    //   of the modules/layer/etc.. (for containing memory use).
    //   This is not for service routing objects.
    Materials * materials_;

    ElementsVector serviceElements_; //used for MaterialObject not from config file (service routing)
    
  };

  typedef std::vector<const MaterialObject::Element*> ElementsVector;

} /* namespace material */

#endif /* MATERIALOBJECT_H_ */
