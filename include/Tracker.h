#ifndef TRACKER_H
#define TRACKER_H

#include <vector>
#include <string>
#include <memory>
#include <set>

#include <boost/ptr_container/ptr_vector.hpp>

#include "global_funcs.h"
#include "Property.h"
#include "Barrel.h"
#include "Endcap.h"
#include "Visitor.h"

using std::set;


class Tracker : public PropertyObject, public Buildable, public Identifiable<Tracker> {
  class ModuleSetVisitor : public GenericGeometryVisitor {
  public:
    typedef set<Module*> Modules;
  private:
    Modules modules_;
  public:
    void visit(Module& m) { modules_.insert(&m); }
    Modules& modules() { return modules_; }
    const Modules& modules() const { return modules_; }
    Modules::iterator begin() { return modules_.begin(); }
    Modules::iterator end() { return modules_.end(); }
    Modules::const_iterator begin() const { return modules_.begin(); }
    Modules::const_iterator end() const { return modules_.end(); }
  };

public:
  typedef boost::ptr_vector<Barrel> Barrels;
  typedef boost::ptr_vector<Endcap> Endcaps;
  typedef ModuleSetVisitor::Modules Modules;

  ReadonlyProperty<double, Computable> maxR;

private:
  Barrels barrels_;
  Endcaps endcaps_;

  ModuleSetVisitor moduleSetVisitor_;

  PropertyNode<string> barrelNode;
  PropertyNode<string> endcapNode;
public:

  Tracker() :
      barrelNode("Barrel", parsedOnly()),
      endcapNode("Endcap", parsedOnly()),
      maxR([&]() { 
             double max = 0; 
             for (const auto& b : barrels_) max = MAX(max, b.maxR());
             for (const auto& e : endcaps_) max = MAX(max, e.maxR());
             return max;
           })
  {}


  void build();

  const Barrels& barrels() const { return barrels_; }
  const Endcaps& endcaps() const { return endcaps_; }

  const Modules& modules() const { return moduleSetVisitor_.modules(); }

  void accept(GenericGeometryVisitor& v) { 
    v.visit(*this); 
    for (auto& b : barrels_) { b.accept(v); }
    for (auto& e : endcaps_) { e.accept(v); }
  }
};


#endif
