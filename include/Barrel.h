#ifndef BARREL_H
#define BARREL_H

#include "Property.h"

class Barrel : public PropertyObject, public Buildable {
  vector<shared_ptr<Layer>> layers_;

  Property<int> numLayers;
  Property<int> innerRadius;
  Property<int> outerRadius;
public:
  ReadOnlyProperty<double> maxZ;

  Barrel() :
      numLayers(noDefault(), greaterThan(0)),
      innerRadius(noDefault(), greaterThan(0)),
      outerRadius(noDefault(), greaterThan(0)),
      maxZ([&]() { double m = 0; for (auto& l : layers_) { m = MAX(m, l->maxZ()); } return m; }, CacheIf(built())) 
  {
    requiredProperty(numLayers, "numLayers");
    requiredProperty(innerRadius, "innerRadius");
    requiredProperty(outerRadius, "outerRadius");
  }

  void build() {
    check();

    auto childmap = propertyTree().getChildmap<int>("Layer");
    for (int i = 1; i <= numLayers(); i++) {
      Layer* layer = new Layer();

      if      (i == 1)           { layer.radiusMode(Layer::FIXED); layer.placeRadiusHint(innerRadius()); } 
      else if (i == numLayers()) { layer.radiusMode(Layer::FIXED); layer.placeRadiusHint(outerRadius()); } 
      else                       { layer.placeRadiusHint(innerRadius() + (outerRadius()-innerRadius())/(numLayers()-1)*(i-1)); }

      if (!sameRods()) { layer.buildRadius(make_pair(innerRadius(),outerRadius())); }

      layer->store(childmap.count(i) > 0 ? childmap[i] : propertyTree());
      layer->build();
      layers_.push_back(layer);
    }

    built(true);
  }
};

#endif
