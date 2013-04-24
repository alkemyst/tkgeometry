
class Sensor : public PropertyObject, public Buildable, public Identifiable<Sensor> {
  Polygon3d<4> poly_; 
  
public:
  Property<int, NoDefault> xElements;
  Property<int, NoDefault> yElements;
  Property<string, NoDefault> type;

  Sensor(const Polygon3d<4>& poly) : 
      poly_(poly),
      xElements("xElements", checked()),
      yElements("yElements", checked()),
      type("type", checked())
  {}
  
};
