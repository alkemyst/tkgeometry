#ifndef TRACKER_H
#define TRACKER_H

#include <vector>
#include <string>
#include <memory>

#include <boost/ptr_container/ptr_vector.hpp>

#include "global_funcs.h"
#include "Property.h"
#include "Barrel.h"
#include "Endcap.h"
#include "GeometryVisitor.h"


class Tracker : public PropertyObject, public Buildable, public Identifiable<Tracker> {
public:
  typedef boost::ptr_vector<Barrel> Barrels;
  typedef boost::ptr_vector<Endcap> Endcaps;
  typedef set<Module*> Modules;

  class ModuleSetVisitor {
    Modules& modules_;
    ModuleSetVisitor(Modules& modules) : modules_(modules) {}
    void visit(Module& m) { modules_.insert(m); }
  };

private:
  Barrels barrels_;
  Endcaps endcaps_;

  Modules modules_;

  Property<int, NoDefault> numMinBiasEvents;
  Property<int, NoDefault> rError;
//  Property<int> zError;
  Property<double, NoDefault> etaCut;
  Property<int, NoDefault> ptCost;
  Property<int, NoDefault> stripCost;
  Property<double, NoDefault> efficiency;
public:
  Tracker() :
      numMinBiasEvents("numMinBiasEvents", checked()),
      etaCut("etaCut", checked()),
      ptCost("ptCost", checked()),
      stripCost("stripCost", checked()),
      efficiency("efficiency", checked())
  {}


  void build();

  const Barrels& barrels() const { return barrels_; }
  const Endcaps& endcaps() const { return endcaps_; }

  const Modules& modules() const { return modules_; }
};


#endif
