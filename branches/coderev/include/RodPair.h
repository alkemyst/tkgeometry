#ifndef RODPAIR_H
#define RODPAIR_H

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <algorithm>

#include <boost/ptr_container/ptr_vector.hpp>

#include "clone_ptr.h"
#include "global_funcs.h"
#include "Property.h"
#include "Module.h"

using std::string;
using std::vector;
using std::pair;
using std::shared_ptr;

typedef vector<shared_ptr<BarrelModule>> RodTemplate;

class RodPair : public PropertyObject, public Buildable, public Identifiable<RodPair> {
public:
  typedef boost::ptr_vector<BarrelModule> Container;
private:
  Container modules_;

  enum BuildDirection { RIGHTWARD_BUILD = 1, LEFTWARD_BUILD = -1 };


  bool built_;

  // Templated because they need to work both with forward and reverse iterators (mezzanines are built right to left and the rodTemplate vector is iterated backwards)
  template<typename Iterator> vector<double> maxZStrategy(Iterator begin, Iterator end, double startZ, int direction, int smallParity, bool looseStartZ); 
  template<typename Iterator> vector<double> numModulesStrategy(Iterator begin, Iterator end, double startZ, int direction, int smallParity, bool looseStartZ);
  template<typename Iterator> vector<double> computeZList(Iterator begin, Iterator end, double startZ, int direction, int smallParity, bool looseStartZ);
  template<typename Iterator> pair<vector<double>, vector<double>> computeZListPair(Iterator begin, Iterator end, double startZ, int recursionCounter);
  void buildFull(const RodTemplate& rodTemplate); 
  void buildMezzanine(const RodTemplate& rodTemplate); 
public:
  Property<double, NoDefault> smallDelta;
  Property<double, NoDefault> minBuildRadius;
  Property<double, NoDefault> maxBuildRadius;

  Property<double, Default> minModuleOverlap;
  Property<double, NoDefault> zError;
  Property<int, NoDefault> zPlusParity;
  Property<int, NoDefault> numModules;
  Property<double, NoDefault> maxZ;
  Property<bool, NoDefault> mezzanine;
  Property<double, NoDefault> startZ;

  ReadonlyProperty<double, Computable> minAperture;
  ReadonlyProperty<double, Computable> maxAperture;
  
  RodPair() :
              minModuleOverlap("minModuleOverlap", checked(), 1.),
              zError("zError", checked()),
              zPlusParity("zPlusParity", checked()),
              numModules("numModules", unchecked()),
              maxZ("maxZ", unchecked()),
              minAperture(Computable<double>([this]() { double min = 999; for (auto& m : modules_) { min = MIN(min, m.aperture()); } return min; })),
              maxAperture(Computable<double>([this]() { double max = 0; for (auto& m : modules_) { max = MAX(max, m.aperture()); } return max; }))
  {}
  
  void build(const RodTemplate& rodTemplate);

  void translate(const XYZVector& translation);
  void rotatePhi(double phi);


  const Container& modules() const { return modules_; }
  

};


#endif
