#ifndef BARREL_H
#define BARREL_H

#include <vector>
#include <string>
#include <memory>

#include <boost/ptr_container/ptr_vector.hpp>

#include "global_funcs.h"
#include "Property.h"
#include "Layer.h"
#include "SupportStructure.h"
#include "Visitable.h"

using std::string;
using std::vector;
using material::SupportStructure;

class Barrel : public PropertyObject, public Buildable, public Identifiable<string>, Clonable<Barrel>, public Visitable {
 public:
  typedef PtrVector<Layer> Container;
  typedef PtrVector<SupportStructure> SupportStructures;
 private:
  Container layers_;
  SupportStructures supportStructures_;

  Property<int, NoDefault> innerRadius;
  Property<int, NoDefault> outerRadius;
  Property<bool, Default> sameRods;
  Property<double, Default> barrelRotation;
  Property<double, Default> supportMarginOuter;
  Property<double, Default> supportMarginInner;
  
  PropertyNode<int> layerNode;
  PropertyNodeUnique<std::string> supportNode;
 public:
  Property<int, NoDefault> numLayers;
  ReadonlyProperty<double, Computable> maxZ, minZ;
  ReadonlyProperty<double, Computable> maxR, minR;
  ReadonlyProperty<bool, Default> skipServices;
  
  Barrel() : 
      numLayers("numLayers", parsedAndChecked()),
      innerRadius("innerRadius", parsedAndChecked()),
      outerRadius("outerRadius", parsedAndChecked()),
      sameRods("sameRods", parsedAndChecked(), false),
      barrelRotation("barrelRotation", parsedOnly(), 0.),
      supportMarginOuter("supportMarginOuter", parsedOnly(), 2.),
      supportMarginInner("supportMarginInner", parsedOnly(), 2.),
      skipServices("skipServices", parsedOnly(), false), // broken, do not use
      layerNode("Layer", parsedOnly()),
      supportNode("Support", parsedOnly())
      {}
  
  void setup() {
    maxZ.setup([this]() { double max = 0; for (const auto& l : layers_) { max = MAX(max, l.maxZ()); } return max; });
    minZ.setup([this]() { double min = 99999; for (const auto& l : layers_) { min = MIN(min, l.minZ()); } return min; });
    maxR.setup([this]() { double max = 0; for (const auto& l : layers_) { max = MAX(max, l.maxR()); } return max; });
    minR.setup([this]() { double min = 99999; for (const auto& l : layers_) { min = MIN(min, l.minR()); } return min; });
  }

  void build(); 
  void cutAtEta(double eta);

  const Container& layers() const { return layers_; }

  void accept(GeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& l : layers_) { l.accept(v); }
  }
  void accept(ConstGeometryVisitor& v) const { 
    v.visit(*this); 
    for (const auto& l : layers_) { l.accept(v); }
  }

  SupportStructures& supportStructures() {return supportStructures_;}
};

#endif
