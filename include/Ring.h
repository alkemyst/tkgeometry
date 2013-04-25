#ifndef RING_H
#define RING_H

#include <vector>
#include <string>
#include <memory>

#include <boost/ptr_container/ptr_vector.hpp>

using std::vector;
using std::string;

#include "global_funcs.h"
#include "Property.h"
#include "Module.h"

#define MAX_WEDGE_CALC_LOOPS 100

class Ring : public PropertyObject, public Buildable, public Identifiable<Ring> {

  boost::ptr_vector<EndcapModule> modules_;

  template<class T> int roundToOdd(T x) {
    return round((x-1)/2)*2+1;
  }
  double solvex(double y);
  double compute_l(double x, double y, double d);
  double compute_d(double x, double y, double l);
  double computeTentativePhiAperture(double moduleWaferDiameter);
  std::pair<double, int> computeOptimalRingParametersWedge(double moduleWaferDiameter);
  std::pair<double, int> computeOptimalRingParametersRectangle(double moduleWidth);

  Property<ModuleShape, NoDefault> moduleShape;
  Property<ModuleType, NoDefault> moduleType;
  Property<double, NoDefault> moduleOverlapPhi;
  Property<bool, Default> requireOddModsPerSlice;
  Property<int, NoDefault> numSlices;
  Property<int, Default> additionalModules;
  Property<int, Default> alignEdge;
  Property<double, NoDefault> smallDelta;
public:
  Property<int, AutoDefault> disk;
  Property<float, NoDefault> minRadius;
  Property<float, NoDefault> maxRadius;
  Property<float, Computable> thickness;

  Ring() :
      moduleShape("moduleShape", checked()),
      moduleOverlapPhi("moduleOverlapPhi", checked()),
      requireOddModsPerSlice("requireOddModsPerSlice", unchecked(), false),
      numSlices("numSlices", checked()),
      additionalModules("additionalModules", unchecked(), 0),
      alignEdge("alignEdge", unchecked(), 0),
      smallDelta("smallDelta", checked()),
      minRadius("minRadius", checked()),
      maxRadius("maxRadius", checked()),
      thickness(Computable<float>([&]() { return 2*smallDelta() + modules_.front().thickness(); }))
  {}
  
  void build();
  void translateZ(float z);
};

#endif
