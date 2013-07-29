#ifndef ENDCAP_H
#define ENDCAP_H

#include <vector>
#include <string>
#include <memory>

#include <boost/ptr_container/ptr_vector.hpp>

#include "global_funcs.h"
#include "Property.h"
#include "Disk.h"


class Endcap : public PropertyObject, public Buildable, public Identifiable<Endcap> {
  boost::ptr_vector<Disk> disks_;

  enum class MinZType { ABSOLUTE, BARRELGAP };

  Property<double, NoDefault> barrelGap;
  PropertyNode<int> diskNode;
  
  vector<double> findMaxDsDistances();
public:
  ReadonlyProperty<int, NoDefault> numDisks;
  Property<double, NoDefault> barrelMaxZ;
  Property<double, NoDefault> minZ;
  Property<double, NoDefault> maxZ;
  ReadonlyProperty<double, Computable> maxR;

  Endcap() :
      barrelGap("barrelGap", parsedOnly()),
      numDisks("numDisks", parsedAndChecked()),
      minZ("minZ", parsedOnly()),
      maxZ("maxZ", parsedAndChecked()),
      diskNode("Disk", parsedOnly())
  {}

  void setup() {
    maxR.setup([&]() { double max = 0; for (const auto& d : disks_) { max = MAX(max, d.maxR()); } return max; });
    for (auto& d : disks_) d.setup();
  }

  void build();

  void accept(GeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& d : disks_) { d.accept(v); }
  }
  void accept(ConstGeometryVisitor& v) const { 
    v.visit(*this); 
    for (const auto& d : disks_) { d.accept(v); }
  }
};



#endif
