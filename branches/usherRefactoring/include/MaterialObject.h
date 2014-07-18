/**
 * @file MaterialObject.h
 *
 * @date 19/giu/2014
 * @author Stefano Martina
 */

#ifndef MATERIALOBJECT_H_
#define MATERIALOBJECT_H_

#include "Property.h"

class MaterialObject : public PropertyObject {
public:
  enum Type {MODULE, ROD};

  MaterialObject(Type type) :
    type_ (type),
    materialsNode_ ("Materials", parsedOnly()),
    componentsNode_ ("Component", parsedOnly()) {}
  virtual ~MaterialObject() {};

  virtual void build();

  //TODO: do methods for interrogate/get materials

private:
  static const std::map<Type, const std::string> typeString;
  Type type_;
  PropertyNodeUnique<std::string> materialsNode_;
  PropertyNodeUnique<std::string> componentsNode_;
  const std::string getTypeString() const;

  void buildComponents();

  class Component : public PropertyObject {
  public:
    ReadonlyProperty<std::string, NoDefault> componentName;
    PropertyNodeUnique<std::string> componentsNode_;
    PropertyNodeUnique<std::string> elementNode_;
    Component() :
      componentName ("componentName", parsedAndChecked()),
      componentsNode_ ("Component", parsedOnly()),
      elementNode_ ("Element", parsedOnly()) {}
    virtual ~Component() {}
    void build();

    class Element : public PropertyObject {
    public:
      ReadonlyProperty<std::string, NoDefault> componentName; //only the inner component's name
      ReadonlyProperty<long, NoDefault> nStripAcross;
      ReadonlyProperty<long, NoDefault> nSegments;
      ReadonlyProperty<std::string, NoDefault> elementName;
      ReadonlyProperty<bool, NoDefault> service;
      ReadonlyProperty<bool, NoDefault> scale;
      ReadonlyProperty<std::string, NoDefault> quantity;
      ReadonlyProperty<std::string, NoDefault> unit;
      Element() :
        componentName ("componentName", parsedOnly()),
        nStripAcross("nStripAcross", parsedOnly()),
        nSegments("nSegments", parsedOnly()),
        elementName ("elementName", parsedAndChecked()),
        service ("service", parsedAndChecked()),
        scale ("scale", parsedAndChecked()),
        quantity ("quantity", parsedAndChecked()),
        unit ("unit", parsedAndChecked()) {}
      virtual ~Element() {}
      void build();
    };

    PtrVector<Component> components;
    PtrVector<Element> elements;
  };

  PtrVector<Component> components;
};

#endif /* MATERIALOBJECT_H_ */
