#ifndef LAYER_H
#define LAYER_H

#include "Property.h" 

class Layer : public PropertyObject, public Buildable {
  vector<RodPair*> rods_;

  typedef vector<shared_ptr<Module>> RodTemplate;
  RodTemplate makeRodTemplate(const PropertyTree&);

  Property<double> smallDelta, bigDelta;
  Property<bool> sameRods;
  Property<int> zPlusParity;
  Property<bool> mezzanine;
  Property<double> startZ;
  Property<int> numModules;
  Property<double> maxZ;
public:
  enum RadiusMode { SHRINK, ENLARGE, FIXED, AUTO };
  Property<RadiusMode> radiusMode;
  Property<double> placeRadiusHint;

  Property<double> minBuildRadius;
  Property<double> maxBuildRadius;

  Layer() :
      smallDelta(noDefault(), greater(0)),
      bigDelta(noDefault(), greater(0)),
      sameRods(true),
      zPlusParity(1),
      mezzanine(false),
      startZ(noDefault()),
      numModules(noDefault()),
      maxZ(noDefault())
  {
    requiredProperty(smallDelta, "smallDelta");
    requiredProperty(bigDelta, "bigDelta");
    optionalProperty(startZ, "startZ");
    optionalProperty(numModules, "numModules");
    requiredProperty();
  }


  void build();
};


#endif
