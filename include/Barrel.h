#ifndef BARREL_H
#define BARREL_H

#include <vector>
#include <string>
#include <memory>

#include <boost/ptr_container/ptr_vector.hpp>

#include "global_funcs.h"
#include "Property.h"
#include "Layer.h"

using std::string;
using std::vector;

class Barrel : public PropertyObject, public Buildable, public Identifiable<Barrel> {
public:
  typedef boost::ptr_vector<Layer> Container;
private:
  Container layers_;

  Property<int, NoDefault> numLayers;
  Property<int, NoDefault> innerRadius;
  Property<int, NoDefault> outerRadius;
  Property<bool, Default> sameRods;

  PropertyNode<int> layerNode;
public:
  ReadonlyProperty<double, Computable> maxZ;
  ReadonlyProperty<double, Computable> maxR;

  Barrel() : 
      numLayers("numLayers", parsedAndChecked()),
      innerRadius("innerRadius", parsedAndChecked()),
      outerRadius("outerRadius", parsedAndChecked()),
      sameRods("sameRods", parsedAndChecked(), false),
      layerNode("Layer", parsedOnly())
  {}

  void setup() {
    maxZ.setup([this]() { double max = 0; for (const auto& l : layers_) { max = MAX(max, l.maxZ()); } return max; });
    maxR.setup([this]() { double max = 0; for (const auto& l : layers_) { max = MAX(max, l.maxR()); } return max; });
    for (auto& l : layers_) l.setup();
  }

  void build(); 

  const Container& layers() const { return layers_; }

  void accept(GeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& l : layers_) { l.accept(v); }
  }
  void accept(ConstGeometryVisitor& v) const { 
    v.visit(*this); 
    for (const auto& l : layers_) { l.accept(v); }
  }
};

#endif
