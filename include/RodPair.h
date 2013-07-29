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
using std::unique_ptr;

typedef vector<unique_ptr<RectangularModule>> RodTemplate;

class RodPair : public PropertyObject, public Buildable, public Identifiable<RodPair> {
public:
  typedef boost::ptr_vector<BarrelModule> Container;
private:
  Container modules_;

  enum class BuildDirection { RIGHT = 1, LEFT = -1 };

  // Templated because they need to work both with forward and reverse iterators (mezzanines are built right to left and the rodTemplate vector is iterated backwards)
  double computeNextZ(double newDsDistance, double lastDsDistance, double lastZ, BuildDirection direction, int parity);
  template<typename Iterator> vector<double> computeZList(Iterator begin, Iterator end, double startZ, BuildDirection direction, int smallParity, bool looseStartZ);
  template<typename Iterator> pair<vector<double>, vector<double>> computeZListPair(Iterator begin, Iterator end, double startZ, int recursionCounter);
  void buildModules(const RodTemplate& rodTemplate, const vector<double>& posList, BuildDirection direction, int parity);
  void buildFull(const RodTemplate& rodTemplate); 
  void buildMezzanine(const RodTemplate& rodTemplate); 

  void clearComputables();
public:
  Property<double, NoDefault> smallDelta;
  Property<double, NoDefault> minBuildRadius;
  Property<double, NoDefault> maxBuildRadius;

  Property<double, Default> minModuleOverlap;
  Property<double, NoDefault> zError;
  Property<int, NoDefault> zPlusParity;
  Property<int, NoDefault> buildNumModules;
  Property<double, NoDefault> maxZ;
  Property<double, Computable> minZ, maxR, minR;
  Property<bool, Default> mezzanine;
  Property<double, NoDefault> startZ;

  PropertyNode<int> ringNode;

  ReadonlyProperty<double, Computable> minAperture;
  ReadonlyProperty<double, Computable> maxAperture;

  
  RodPair() :
              minModuleOverlap("minModuleOverlap", parsedAndChecked() , 1.),
              zError          ("zError"          , parsedAndChecked()),
              zPlusParity     ("zPlusParity"     , parsedAndChecked()),
              mezzanine       ("mezzanine"       , parsedOnly(), false),
              startZ          ("startZ"          , parsedOnly()),
              ringNode        ("Ring"            , parsedOnly())
  {}

  void setup() {
    minAperture.setup([this]() { double min = 999; for (auto& m : modules_) { min = MIN(min, m.phiAperture()); } return min; });
    maxAperture.setup([this]() { double max = 0; for (auto& m : modules_) { max = MAX(max, m.phiAperture()); } return max; });
    minZ.setup([&]() { double min = 99999; for (const auto& m : modules_) { min = MIN(min, m.minZ()); } return min; });
    minR.setup([&]() { double min = 99999; for (const auto& m : modules_) { min = MIN(min, m.minR()); } return min; });
    maxR.setup([&]() { double max = 0; for (const auto& m : modules_) { max = MAX(max, m.maxR()); } return max; });
    for (auto& m : modules_) m.setup();
  }

  int numModules() const { return modules_.size(); }
  
  void build(const RodTemplate& rodTemplate);

  void translate(const XYZVector& translation);
  void translateR(double radius);
  void rotateZ(double angle);

  void compressToZ(double z);

  const Container& modules() const { return modules_; }
  
  void accept(GeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& m : modules_) { m.accept(v); }
  }
  void accept(ConstGeometryVisitor& v) const { 
    v.visit(*this); 
    for (const auto& m : modules_) { m.accept(v); }
  }
};


#endif
