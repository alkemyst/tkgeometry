
double BarrelLayer::calculatePlaceRadius(int numRods,
                                         double bigDelta,
                                         double smallDelta,
                                         double dsDistance,
                                         double moduleWidth,
                                         double overlap) {

  double d = dsDistance/2;

  double f = (moduleWidth/2) - (overlap/2);

  double R = bigDelta + smallDelta + d;
  double S = -bigDelta + smallDelta + d;

  double T = tan(2*M_PI/numRods);

  double a = T;
  double b = R*T + S*T - 2*f;
  double c = R*S*T - R*f - S*f - T*f*f;

  double r = (-b + sqrt(b*b - 4*a*c))/(2*a);

  return r;
}

std::pair<float, int> BarrelLayer::calculateOptimalLayerParms(const RodTemplate& rodTemplate) {
                                                              
  // CUIDADO fix placeRadiusHint!!!!!
  double maxDsDistance = std::max_element(rodTemplate.begin(), 
                                          rodTemplate.end(), 
                                          [](Module* m1, Module* m2) { return m1->dsDistance() > m2->dsDistance() } )->dsDistance();
  float modWidth = rodTemplate.rbegin()->moduleWidth();
  float f = moduleWidth/2 - overlap()/2;
  float gamma = atan(f/placeRadiusHint() + bigDelta() + smallDelta() + maxDsDistance/2) + atan(f/(placeRadiusHint() - bigDelta() + smallDelta() + maxDsDistance/2));
  float tentativeSlices = M_2_PI/(gamma * modsPerSlice());

  float optimalRadius;
  int optimalSlices;

  switch (radiusMode()) {
  case SHRINK:
    optimalSlices = floor(tentativeSlices);
    optimalRadius = calculatePlaceRadius(optimalSlices*modsPerSlice(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, overlap());
    break;
  case ENLARGE:
    optimalSlices = ceil(tentativeSlices);
    optimalRadius = calculatePlaceRadius(optimalSlices*modsPerSlice(), bigDelta(), smallDelta(), maxDsDistance, moduleWidth, overlap());
    break;
  case FIXED:
    optimalSlices = ceil(tentativeSlices);
    optimalRadius = placeRadiusHint();
    break;
  case AUTO:
    int slicesLo = floor(slices);
    int slicesHi = ceil(slices);
    float radiusLo = calculatePlaceRadius(slicesLo*modsPerSlice(), bigDelta(), smallDelta(), maxdsDistance, moduleWidth, overlap());
    float radiusHi = calculatePlaceRadius(slicesHi*modsPerSlice(), bigDelta(), smallDelta(), maxdsDistance, moduleWidth, overlap());

    if (fabs(radiusHi - placeRadiusHint()) < fabs(radiusLo - placeRadiusHint())) {
      optimalRadius = radiusHi;
      optimalslices = slicesHi;
    } else {
      optimalRadius = radiusLo;
      optimalslices = slicesLo;
    }
    break;
  }

  return std::make_pair(optimalRadius, optimalSlices*modsPerSlice());
}


vector<shared_ptr<Module>> Layer::makeRodTemplate(const PropertyTree& pt) {
  vector<shared_ptr<Module> > rodTemplate(numModules.isSet() ? numModules() : 0);
  for (auto& c : pt.getChildren("Module")) {
    int ring = c.getValue<int>(0);
    if (ring > 0) {
      if (rodTemplate.size() < ring) rodTemplate.resize(ring);
      (rodTemplate[ring-1] = new Module())->build(c);
    }
  }
  if (!numModules.isSet()) rodTemplate.push_back(NULL);// additional module built with default inherited properties to use in case the maxZ placement strategy requires additional modules than the ones constructed from mod-specific props
  for (auto& m : rodTemplate) {
    if (m == NULL) {
      m = new Module();
      m->build(pt);
    }
  }
  return rodTemplate;
}

void Layer::build(PropertyTree pt) {
  pt.processProperties(properties_);
  properties_.check();
  
  RodTemplate rodTemplate = makeRodTemplate(pt);

  std::pair<double, int> optimalLayerParms = calculateOptimalLayerParms(rodTemplate);
  placeRadius(optimalLayerParms.first); 
  numRods(optimalLayerParms.second);

  float rodPhiRotation = M_2_PI/numRods;

  RodPair* first = new RodPair(*this);
  first.build(rodTemplate);
  first.translate(XYZVector(placeRadius()+bigDelta(), 0, 0));
  rods_.push_back(first);

  RodPair* second = new RodPair(*this);
  second.build(rodTemplate);
  second.translate(second, XYZVector(placeRadius()-bigDelta(), 0, 0));
  second.rotatePhi(rodPhiRotation);
  rods_.push_back(second);

  for (int i = 2; i < numRods(); i++) {
    RodPair* rod = new RodPair(i%2 ? second : first); // clone rods
    rod.rotatePhi(rodPhiRotation*i);
    rods_.push_back(rod);
  }
}






void Layer::build(PropertyTree& pt) {
  pt.processProperties(properties_);

  
}
