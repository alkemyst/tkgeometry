
class RodPair {

  enum BuildDirection { RIGHTWARD_BUILD = 1, LEFTWARD_BUILD = -1 };

  const Layer& parent_;

  std::vector<shared_ptr<Module> > modules_;
public:
  DerivedProperty<double> minAperture;
  DerivedProperty<double> maxAperture;
  
  RodPair(const Layer& parent) :
      parent_(parent),
      minAperture([&modules_]() { return std::min_element(modules_.begin(), 
                                                         modules_.end(), 
                                                         [](Module* m1, Module* m2) { return m1->aperture() < m2->aperture(); } )->aperture(); } ),
      maxAperture([&modules_]() { return std::max_element(modules_.begin(), 
                                                         modules_.end(), 
                                                         [](Module* m1, Module* m2) { return m1->aperture() > m2->aperture(); } )->aperture(); } )
  {}
  
  void build(const vector<shared_ptr<Module>>& rodTemplate);

  void translate(const XYZVector& translation);
  void rotatePhi(double phi);

};

