#ifndef LAYER_H
#define LAYER_H

#include <vector>
#include <string>
#include <memory>

#include <boost/ptr_container/ptr_vector.hpp>

#include "global_funcs.h"
#include "Property.h"
#include "Module.h"
#include "RodPair.h"

using std::string;
using std::vector;
using std::pair;
using std::shared_ptr;

typedef vector<shared_ptr<BarrelModule>> RodTemplate;


class Layer : public PropertyObject, public Buildable, public Identifiable<Layer> {
//  friend class RodPair;
public:
  typedef boost::ptr_vector<RodPair> Container;
private:
  Container rods_;

  double calculatePlaceRadius(int numRods, double bigDelta, double smallDelta, double dsDistance, double moduleWidth, double overlap);
  pair<float, int> calculateOptimalLayerParms(const RodTemplate&);
  RodTemplate makeRodTemplate(const PropertyTree&);

  Property<double, NoDefault> smallDelta, bigDelta;
  Property<double, Default> rodOverlap;
  Property<int, Default> modsPerSlice;

  double placeRadius_;
  int numRods_;
public:
  ReadonlyProperty<int, NoDefault> numModules;
  ReadonlyProperty<double, NoDefault> maxZ;
  enum RadiusMode { SHRINK, ENLARGE, FIXED, AUTO };
  Property<RadiusMode, Default> radiusMode;
  Property<double, NoDefault> placeRadiusHint;

  Property<double, NoDefault> minBuildRadius;
  Property<double, NoDefault> maxBuildRadius;
  Property<bool, Default> sameParityRods;

  Layer() :
            smallDelta     ("smallDelta"     , checked()),
            bigDelta       ("bigDelta"       , checked()),
            rodOverlap     ("rodOverlap"     , checked(), 1.),
            modsPerSlice   ("modsPerSlice"   , checked(), 2),
            radiusMode     ("radiusMode"     , checked(), RadiusMode::AUTO),
            placeRadiusHint("placeRadiusHint", unchecked()),
            minBuildRadius ("minBuildRadius" , unchecked()),
            maxBuildRadius ("maxBuildRadius" , unchecked()),
            sameParityRods ("sameParityRods" , checked(), false)
  {}


  void build();

  const Container& rods() const { return rods_; }
};



#endif
