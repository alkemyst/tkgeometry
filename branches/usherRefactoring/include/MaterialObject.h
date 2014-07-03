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
public:
  MaterialObject();
  virtual ~MaterialObject();

  void buildMaterials();

private:
  struct Materials {
    std::string destination;
    bool scale;
    double quantity;
    std::string unit;
  };
};

#endif /* MATERIALOBJECT_H_ */
