#ifndef DISK_H
#define DISK_H

#include <vector>
#include <string>
#include <memory>

#include <boost/ptr_container/ptr_vector.hpp>

#include "global_funcs.h"
#include "Property.h"
#include "Ring.h"

class Disk : public PropertyObject, public Buildable, public Identifiable<Disk> {
  boost::ptr_vector<Ring> rings_;

  Property<int, NoDefault> numRings;
  Property<double, NoDefault> innerRadius;
  Property<double, NoDefault> outerRadius;
  Property<double, NoDefault> bigDelta;
  Property<double, Default> minRingOverlap;
  Property<int, Default> diskParity;

  PropertyNode<int> ringNode;

  inline double getDsDistance(const vector<double>& buildDsDistances, int rindex) const;
  void buildTopDown(const vector<double>& buildDsDistances);
  void buildBottomUp(const vector<double>& buildDsDistances);

public:
  Property<double, NoDefault> zError;
  Property<double, NoDefault> buildZ;

  Disk() :
    numRings("numRings", parsedAndChecked()),
    innerRadius("innerRadius", parsedAndChecked()),
    outerRadius("outerRadius", parsedAndChecked()),
    bigDelta("bigDelta", parsedAndChecked()),
    zError("zError", parsedAndChecked()),
    minRingOverlap("minRingOverlap", parsedOnly(), 1.),
    diskParity("diskParity", parsedOnly(), 1),
    ringNode("Ring", parsedOnly())
  {}

  void check() override;
  void build(const vector<double>& buildDsDistances);
  void translateZ(double z);

  void accept(GenericGeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& r : rings_) { r.accept(v); }
  }
};

#endif
