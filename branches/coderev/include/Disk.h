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
  Property<double, NoDefault> zError;
  Property<double, Default> minRingOverlap;
  Property<int, Default> diskParity;

  void buildTopDown();
  void buildBottomUp();

public:
  Property<double, NoDefault> buildZ;

  Disk() :
    numRings("numRings", checked()),
    innerRadius("innerRadius", checked()),
    outerRadius("outerRadius", checked()),
    bigDelta("bigDelta", checked()),
    zError("zError", checked()),
    minRingOverlap("minRingOverlap", unchecked(), 1.),
    diskParity("diskParity", unchecked(), 1)
  {}

  void build();
  void translateZ(double z);
};

#endif
