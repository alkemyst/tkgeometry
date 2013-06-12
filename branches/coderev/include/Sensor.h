#include <string>

#include "global_funcs.h"
#include "Polygon3d.h"
#include "Property.h"

enum class SensorType { Pixel, Strip };

class Sensor : public PropertyObject, public Buildable, public Identifiable<Sensor> {
  Polygon3d<4> poly_;
public:
  ReadonlyProperty<int, NoDefault> xElements;
  ReadonlyProperty<int, NoDefault> yElements;
  ReadonlyProperty<int, NoDefault> numChannels;
  ReadonlyProperty<int, Default> numROCs;
  ReadonlyProperty<double, Default> sensorThickness;
  ReadonlyProperty<SensorType, NoDefault> type;

  Sensor() : 
      xElements("xElements", parsedAndChecked()),
      yElements("yElements", parsedAndChecked()),
      numChannels("numChannels", parsedAndChecked()),
      numROCs("numROCs", parsedOnly(), 128),
      sensorThickness("sensorThickness", parsedOnly(), 0.1),
      type("sensorType", parsedAndChecked())
  {}

  Polygon3d<4>& poly() { return poly_; }

  void build() { check(); cleanup(); }
  
};

define_enum_strings(SensorType) = { "pixel", "strip" };
