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
using std::unique_ptr;


class Layer : public PropertyObject, public Buildable, public Identifiable<Layer> {
//  friend class RodPair;
public:
  typedef boost::ptr_vector<RodPair> Container;
private:
  Container rods_;

  double calculatePlaceRadius(int numRods, double bigDelta, double smallDelta, double dsDistance, double moduleWidth, double overlap);
  pair<float, int> calculateOptimalLayerParms(const RodTemplate&);
  RodTemplate makeRodTemplate();

  Property<double, NoDefault> smallDelta, bigDelta;
  Property<double, Default> rodOverlapPhi;
  Property<int, Default> phiSegments;

  PropertyNode<int> ringNode; // to grab properties for specific rod modules

  double placeRadius_;
  int numRods_;
public:
  ReadonlyProperty<int, UncachedComputable> numModules;
  ReadonlyProperty<double, UncachedComputable> maxZ;
  enum RadiusMode { SHRINK, ENLARGE, FIXED, AUTO };
  Property<RadiusMode, Default> radiusMode;
  Property<double, NoDefault> placeRadiusHint;

  Property<double, NoDefault> minBuildRadius;
  Property<double, NoDefault> maxBuildRadius;
  Property<bool, Default> sameParityRods;

  Layer() :
            smallDelta     ("smallDelta"     , parsedAndChecked()),
            bigDelta       ("bigDelta"       , parsedAndChecked()),
            rodOverlapPhi  ("rodOverlapPhi"  , parsedAndChecked(), 1.),
            phiSegments    ("phiSegments"    , parsedAndChecked(), 4),
            ringNode       ("Ring"           , parsedOnly()),
            numModules     ("numModules"     , parsedOnly(), [this](){ return rods_.front().numModules(); }),
            maxZ           ("maxZ"           , parsedOnly(), [this](){ return rods_.front().maxZ(); }),
            radiusMode     ("radiusMode"     , parsedAndChecked(), RadiusMode::AUTO),
            placeRadiusHint("placeRadiusHint", parsedOnly()),
            minBuildRadius ("minBuildRadius" , parsedOnly()),
            maxBuildRadius ("maxBuildRadius" , parsedOnly()),
            sameParityRods ("sameParityRods" , parsedAndChecked(), false)
  {}


  void check() override;
  void build();

  const Container& rods() const { return rods_; }

  void accept(GenericGeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& r : rods_) { r.accept(v); }
  }
};



#endif
