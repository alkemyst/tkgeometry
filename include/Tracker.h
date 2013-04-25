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

  PropertyNode<string> barrelNode;
  PropertyNode<string> endcapNode;
public:
  Tracker() :
      numMinBiasEvents("numMinBiasEvents", parsedAndChecked()),
      etaCut("etaCut", parsedAndChecked()),
      ptCost("ptCost", parsedAndChecked()),
      stripCost("stripCost", parsedAndChecked()),
      efficiency("efficiency", parsedAndChecked()),
      barrelNode("Barrel", parsedOnly()),
      endcapNode("Endcap", parsedOnly())
  {}


  void build();

  const Barrels& barrels() const { return barrels_; }
  const Endcaps& endcaps() const { return endcaps_; }

  const Modules& modules() const { return modules_; }
};


#endif
