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

  Property<int, NoDefault> numDisks;
  Property<double, NoDefault> barrelGap;
  Property<double, NoDefault> minZ;
  Property<double, NoDefault> maxZ;
  PropertyNode<int> diskNode;
  
  vector<double> findMaxDsDistances();
public:
  Property<double, NoDefault> barrelMaxZ;

  ReadonlyProperty<double, Computable> maxR;

  Endcap() :
      numDisks("numDisks", parsedAndChecked()),
      barrelGap("barrelGap", parsedOnly()),
      minZ("minZ", parsedOnly()),
      maxZ("maxZ", parsedAndChecked()),
      maxR([&]() { double max = 0; for (const auto& d : disks_) { max = MAX(max, d.maxR()); } return max; }),
      diskNode("Disk", parsedOnly())
  {}

  void build();

  void accept(GenericGeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& d : disks_) { d.accept(v); }
  }
};



#endif
