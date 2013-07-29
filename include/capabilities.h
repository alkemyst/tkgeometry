#ifndef CAPABILITIES_H
#define CAPABILITIES_H

#include <typeinfo>
#include <string>

using std::string;


class Buildable {
protected:
  bool built_;
public:
  Buildable() : built_(false) {}
  bool builtok() const { return built_; }
  void builtok(bool state) { built_ = state; }
};

class Placeable {
protected:
  bool placed_;
public:
  Placeable() : placed_(false) {}
  bool placed() const { return placed_; }
  void placed(bool state) { placed_ = state; }
};

typedef string IdentifiableType;

template<class T>
class Identifiable {
  const IdentifiableType base_;
  IdentifiableType myid_;
public:
  Identifiable() : base_(typeid(T).name()), myid_("NOID") {}
  template<class U> void myid(U id) { myid_ = any2str(id); }
  IdentifiableType myid() const { return myid_; }
  IdentifiableType fullid() const { return base_ + "(" + myid() + ")"; }
};

#endif
