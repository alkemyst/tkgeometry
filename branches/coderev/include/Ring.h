#define MAX_WEDGE_CALC_LOOPS 10

class Ring : public PropertyObject {

  vector<shared_ptr<Module*>> modules_;

  template<class T> int roundToOdd(T x) {
    return round((x-1)/2)*2+1;
  }

  double EndcapLayer::solvex(double y) {
    return(1/8.*(3*y+2-sqrt(9*pow(y, 2)-4*y+4)));
  }

  double EndcapLayer::compute_l(double x, double y, double d) {
    double result = d;
    result /= (1-x)-sqrt((1-x)*(y-x));
    return result;
  }

  double EndcapLayer::compute_d(double x, double y, double l) {
    double result = l;
    result *= (1-x)-sqrt((1-x)*(y-x));
    return result;
  }

  double computeTentativePhiAperture(double moduleWaferSize) {
    double r = moduleWaferSize/2;

    double l = (minRadius()-r);
    double y = pow(r/l, 2);
    double x = solvex(y);


    double tempd;

    int i = 0;
    for (; i < MAX_WEDGE_CALC_LOOPS; i++) {
      l = compute_l(x, y, minRadius());
      y = pow(r/l, 2);
      x = solvex(y);

      tempd = compute_d(x, y, l);

      if (fabs(minRadius() - tempd)<1e-15) break;
    }

    if (i >= MAX_WEDGE_CALC_LOOPS) {
      logWarning("Maximum number of iterations hit while computing wedge geometry");
    }

    alpha=asin(sqrt(x))*2;

    return alpha;
  }

  std::pair<double, int> computeOptimalRingParametersWedge(double moduleWaferDiameter) {
    double delta = overlap()/minRadius();// SM: The needed overlap becomes an angle delta by
                                         //     checking the unsafest point (r=r_min)

    double tentativeAlpha = computeTentativePhiAperture(first->waferDiameter()) - delta;
    float tentativeNumMods = M_2_PI / tentativeAlpha; 
    int optimalNumMods = ((!requireOddModsPerSlice() ? round(tentativeNumMods/numSlices())) + additionalModules()) * numSlices();
    float optimalAlpha = M_2_PI/optimalNumMods + delta; // CUIDADO check this!

    return make_pair(optimalAlpha, optimalNumMods);
  }

  std::pair<double, int> computeOptimalRingParametersRectangle(double moduleWidth) {
    double delta = overlap()/maxRadius();
    double optimalAlpha = 2*asin(moduleWidth/2. / maxRadius()) - delta;
    double tentativeNumMods = M_2_PI/alpha;
    int modsPerSlice = ceil(tentativeNumMods/numSlices());
    if ((modsPerSlice % 2) == 0 && requireOddModsPerSlice()) modsPerSlice++;
    int optimalNumMods = modsPerSlice * numSlices();

    return make_pair(optimalAlpha, optimalNumMods);
  }


public:
  LocalProperty<float> minRadius;
  LocalProperty<float> maxRadius;
  LocalProperty<int> additionalModules;
  
  void build(PropertyTree pt) {
    parse(pt);
   
    Module* first = moduleShape() == Module::RECTANGULAR ? RectangularEndcapModule() : WedgeEndcapModule();
    first->store(propertyTree());

    if (minRadius.isSet()) maxRadius(minRadius() + first->length()); 
    else if (maxRadius.isSet()) minRadius(maxRadius() - first->length());
    
    int numMods;
    double alpha;

    if (moduleShape() == Module::WEDGE) {
      WedgeEndcapModule* wmod = new WedgeEndcapModule();
      optimalRingParms = computeOptimalRingParametersWedge(wmod->waferDiameter());
      alpha = optimalRingParms.first;
      numMods = optimalRingParms.second;
      wmod->phiAperture(alpha);
      wmod->apertureRadius(minRadius());
      first = wmod;
    } else {
      RectangularEndcapModule* emod = new RectangularEndcapModule();
      optimalRingParms = computeOptimalRingParametersRectangle(emod->width());
      alpha = optimalRingParms.first;
      numMods = optimalRingParms.second;
      first = emod;
    }
    first->ring(ringId());
    first->phiIndex(0);
    first->check();
    first->build();
    first->translate(XYZVector(minRadius() + first->length()/2, 0, 0));
    modules_.push_back(first);

    double alignmentRotation = alignEdge() ? 0.5 : 0.;
    for (int i = 0, parity = -1; i < optimalNumMods; i++, parity *= -1) {
      Module* mod = moduleShape() == Module::RECTANGULAR ? new RectangularEndcapModule(*first) : new WedgeEndcapModule(*first);
      mod->phiIndex(i+1);
      mod->rotatePhi(M_2_PI*(i+alignmentRotation)/numMods); // CUIDADO had a rotation offset of PI/2
      mod->translate(XYZVector(0, 0, parity*smallDelta())); 
      
    //XYZVector shift = XYZVector(0, 0, diskZ + nearDirection*ringParity*smallDelta + nearDirection*bigDelta);
    //myModule->translate(shift);
      modules_.push_back(mod);  
    }
  }



  void translateZ(float z) {
    for (auto& m : modules_) {
      m->translate(XYZVector(0, 0, z));
      m->zSide(signum(z));
    }
  }
};


/*
class GeometryStrategy {

};

class RectangleGeometry : public GeometryStrategy {

};

class WedgeGeometry : public GeometryStrategy {

};*/
