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
  Property<MinZType, Default> minZType;
  Property<float, NoDefault> minZ;
  Property<float, NoDefault> maxZ;
public:
  Property<float, NoDefault> barrelMaxZ;

  Endcap() :
      numDisks("numDisks", checked()),
      minZType("minZType", unchecked(), MinZType::BARRELGAP),
      minZ("minZ", checked()),
      maxZ("maxZ", checked())
  {}

  void build();
};



#endif
