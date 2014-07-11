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

private:
  static const std::map<Type, const std::string> typeString;
  //static const std::string componentString;
  Type type_;
  PropertyNode<std::string> materialsNode_;
  PropertyNode<std::string> componentsNode_;
  const std::string getTypeString() const;

  void buildComponents();

  class Component : public PropertyObject {
  public:
    ReadonlyProperty<std::string, NoDefault> component;
    PropertyNode<std::string> elementNode_;
    Component() :
      //component ("Component", parsedOnly()),
      elementNode_ ("Element", parsedOnly()) {}
    virtual ~Component() {}
    //std::string name();
    void build();

    class Element : public PropertyObject {
    public:
      ReadonlyProperty<std::string, NoDefault> element;
      ReadonlyProperty<bool, NoDefault> exiting;
      ReadonlyProperty<bool, NoDefault> scale;
      ReadonlyProperty<std::string, NoDefault> quantity;
      ReadonlyProperty<std::string, NoDefault> unit;
      Element() :
        exiting ("Exiting", parsedAndChecked()),
        scale ("Scale", parsedAndChecked()),
        quantity ("Quantity", parsedAndChecked()),
        unit ("Unit", parsedAndChecked()) {}
      virtual ~Element() {}
      //std::string name();
      void build();
    };

    PtrVector<Element> elements;
  };

  PtrVector<Component> components;
};

#endif /* MATERIALOBJECT_H_ */
