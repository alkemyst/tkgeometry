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

  template<class T> int roundToOdd(T x) { return round((x-1)/2)*2+1; }
  double solvex(double y);
  double compute_l(double x, double y, double d);
  double compute_d(double x, double y, double l);
  double computeTentativePhiAperture(double moduleWaferDiameter, double minRadius);
  std::pair<double, int> computeOptimalRingParametersWedge(double moduleWaferDiameter, double minRadius);
  std::pair<double, int> computeOptimalRingParametersRectangle(double moduleWidth, double maxRadius);

  void buildModules(EndcapModule* templ, int numMods, double smallDelta);
  void buildBottomUp();
  void buildTopDown();

  Property<ModuleShape, NoDefault> moduleShape;
  Property<string, AutoDefault> moduleType;
  Property<double, Default> moduleOverlapPhi;
  Property<bool, Default> requireOddModsPerSlice;
  Property<int, Default> phiSegments;
  Property<int, Default> additionalModules;
  Property<int, Default> alignEdge;
  Property<double, NoDefault> smallDelta;

  double minRadius_, maxRadius_;

public:
  enum BuildDirection { TOPDOWN, BOTTOMUP };
  Property<int, AutoDefault> disk;
  Property<BuildDirection, NoDefault> buildDirection;
  Property<double, NoDefault> buildStartRadius;
  Property<double, NoDefault> buildCropRadius;

  double minRadius() const { return minRadius_; }
  double maxRadius() const { return maxRadius_; }

  Ring() :
      moduleShape           ("moduleShape"           , parsedAndChecked()),
      moduleOverlapPhi      ("moduleOverlapPhi"      , parsedAndChecked(), 1.),
      requireOddModsPerSlice("requireOddModsPerSlice", parsedOnly()      , false),
      phiSegments           ("phiSegments"           , parsedAndChecked(), 4),
      additionalModules     ("additionalModules"     , parsedOnly()      , 0),
      alignEdge             ("alignEdge"             , parsedOnly()      , 0),
      smallDelta            ("smallDelta"            , parsedAndChecked())
  {}
  
  void build();
  void check() override;

  void translateZ(double z);

  void accept(GenericGeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& m : modules_) { m.accept(v); }
  }
};


#endif
