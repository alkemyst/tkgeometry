
class Sensor : public PropertyObject, public Buildable, public Identifiable<Sensor> {
public:
  Property<int, NoDefault> xElements;
  Property<int, NoDefault> yElements;
  Property<double, Default> sensorThickness;
  Property<Polygon3d<4>, AutoDefault> poly;

  Sensor() : 
      xElements("xElements", checked()),
      yElements("yElements", checked()),
      sensorThickness("sensorThickness", 0.1)
  {}

  void build() { check(); }
  
};
