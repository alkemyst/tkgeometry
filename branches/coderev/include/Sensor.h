#include <string>

#include "global_funcs.h"
#include "Polygon3d.h"
#include "Property.h"

enum class SensorType { Pixel, Strip, None };

class Sensor : public PropertyObject, public Buildable, public Identifiable<Sensor> {
  Polygon3d<4> poly_;
public:
  ReadonlyProperty<int, AutoDefault> xElements;
  ReadonlyProperty<int, AutoDefault> yElements;
  ReadonlyProperty<int, Default> numSegments;
  ReadonlyProperty<int, Default> numStripsAcross;
  ReadonlyProperty<int, Default> numROCs;
  ReadonlyProperty<double, Default> sensorThickness;
  ReadonlyProperty<double, AutoDefault> pitch;
  ReadonlyProperty<double, AutoDefault> stripLength;
  ReadonlyProperty<SensorType, Default> type;

  Sensor() : 
      numSegments("numSegments", parsedOnly(), 1),
      numStripsAcross("numStripsAcross", parsedOnly(), 1),
      numROCs("numROCs", parsedOnly(), 128),
      sensorThickness("sensorThickness", parsedOnly(), 0.1),
      type("sensorType", parsedOnly(), SensorType::None)
  {}

  Polygon3d<4>& poly() { return poly_; }

  int numChannels() const { return numStripsAcross() * numSegments(); }


  void build() { check(); cleanup(); }
  
};

