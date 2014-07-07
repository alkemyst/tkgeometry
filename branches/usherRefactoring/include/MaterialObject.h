/**
 * @file MaterialObject.h
 *
 * @date 19/giu/2014
 * @author Stefano Martina
 */

#ifndef MATERIALOBJECT_H_
#define MATERIALOBJECT_H_

#include "Property.h"

class MaterialObject : public PropertyObject{
private:
  PropertyNode<std::string> materialsNode;

  struct Materials {
    std::string destination;
    bool scale;
    double quantity;
    std::string unit;
  };

  Materials materials;

public:
  MaterialObject() :
    materialsNode ("Materials", parsedOnly()) {}
  virtual ~MaterialObject();

  virtual void build();
};

#endif /* MATERIALOBJECT_H_ */
