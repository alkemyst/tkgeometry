#ifndef ENDCAP_H
#define ENDCAP_H

#include "Property.h"

class Endcap : public PropertyObject, public Buildable {
  vector<shared_ptr<Disk>> disks_;

  enum class MinZType { ABSOLUTE, BARRELGAP };

  Property<int> numDisks;
  Property<MinZType> minZType;
  Property<float> minZ;
  Property<float> maxZ;
public:
  Property<float> barrelMaxZ;

  Endcap() :
      numDisks(noDefault(), greater(0)),
      minZType(BARRELGAP),
      minZ(noDefault(), greater(0)),
      maxZ(noDefault(), greater(0)),
      barrelMaxZ(noDefault(), greater(0))
  {
    registerProperty(numDisks, "numDisks"); 
    registerProperty(minZType, "minZType"); 
    registerProperty(minZ, "minZ"); 
    registerProperty(maxZ, "maxZ"); 
  }

  void build() {
    check();

    double adjMinZ = (minZType() == ABSOLUTE) ? minZ() : minZ() + barrelMaxZ();
    
    auto childmap = propertyTree().getChildMap<int>("Disk");
    for (int i = 1; i <= numDisks(); i++) {
      Disk* disk = new Disk();

      if      (i == 1)          disk->placeZ(adjMinZ);
      else if (i == numDisks()) disk->placeZ(maxZ());
      else                      disk->placeZ(adjMinZ + (maxZ() - adjMinZ)/(numDisks() - 1) * (i-1));

      disk->buildZ((adjMinZ()+maxZ())/2);

      disk->store(childmap.count(i) > 0 ? childmap[i] : propertyTree());
      disk->build();
      disks_.push_back(disk);
    }

    built(true);
  }
};


template<> class EnumReflex<MinZType>::data = { "absolute", "barrelgap" };
