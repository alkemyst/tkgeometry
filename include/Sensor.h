#include "Property.h"

class Sensor : public PropertyObject, public Buildable, public Identifiable<Sensor> {
  Polygon3d<4> poly_;
public:
  Property<int, NoDefault> xElements;
  Property<int, NoDefault> yElements;
  Property<double, Default> sensorThickness;

  Sensor() : 
      xElements("xElements", parsedAndChecked()),
      yElements("yElements", parsedAndChecked()),
      sensorThickness("sensorThickness", 0.1)
  {}

  Polygon3d<4>& poly() { return poly_; }

  void build() { check(); }
  
};
