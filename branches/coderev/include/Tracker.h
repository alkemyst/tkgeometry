#ifndef TRACKER_H
#define TRACKER_H

#include "Property.h"

class Tracker : public PropertyObject, public Buildable {
  Property<int> numMinBiasEvents;
  Property<int> rError;
  Property<int> zError;
  Property<float> etaCut;
  Property<int> ptCost;
  Property<int> stripCost;
  Property<float> efficiency;
public:
  Tracker() :
      numMinBiasEvents(noDefault(), greaterThan(0)),
      etaCut(noDefault(), greaterThan(0)),
      ptCost(noDefault(), greaterThan(0)),
      stripCost(noDefault(), greaterThan(0)),
      efficiency(noDefault(), rangeInclusive(0.,1.))
  {
    requiredProperty(numBiasEvents, "numBiasEvents");
    requiredProperty(etaCut, "etaCut");
    requiredProperty(ptCost, "ptCost");
    requiredProperty(stripCost, "stripCost");
    requiredProperty(efficiency, "efficiency");
  }


  void build() {
    check();

    double barrelMaxZ = 0;
    
    for (auto& childtree : propertyTree().getChildren("Barrel")) {
      Barrel* b = new Barrel();
      b->store(childtree);
      b->build();
      barrelMaxZ = MAX(b->maxZ(), barrelMaxZ);
      barrels_.push_back(b);
    }

    for (auto& childtree : propertyTree().getChildren("Endcap")) {
      Endcap* e = new Endcap();
      e->barrelMaxZ(barrelMaxZ);
      e->store(childtree);
      e->build();
      endcaps_.push_back(e);
    }

    built_ = true;
  }

};


#endif
