class Disk {
  vector<shared_ptr<Ring>> rings_;

public:
  void build(PropertyTree& pt) {
    
    auto children = pt.getChildren("Ring");
    map<int, PropertyTree*> childmap;
    std::transform(children.begin(), children.end(), std::back_inserter(childmap), [](PropertyTree& pt) { return std::make_pair(pt.getValue<int>(), &pt); } );
    Ring* prevRing = 0;
    double nextRho;
    for (int i = numRings(), ringParity = 1; i >= 0; i--, ringParity *= -1) {
      Ring* ring = new Ring();
      if (i == numRings()) {
        ring->maxRadius(outerRadius());
        nextRho = ring->minRadius();
      } else {
        double newZ  = buildZ() + (ringParity > 0 ? + bigDelta() : - bigDelta()) + ring->thickness()/2; // CUIDADO was smallDelta + dsDistances[nRing-1]/2;
        double lastZ = buildZ() + (ringParity > 0 ? - bigDelta(): + bigDelta()) - prevRing->thickness()/2; // CUIDADO was smallDelta - dsDistances[nRing-1]/2;
        double originZ = ringParity > 0 ? zError() : -zError();
        double nextRhoOrigin = (nextRho + rOverlap())/lastZ * newZ;
        double nextRhoShifted = nextRho/(lastZ - originZ) * (newZ - originZ);
        nextRho = nextRhoOrigin > nextRhoShifted ? nextRhoOrigin : nextRhoShifted;
        ring->maxRadius(nextRho);
      }
      ring->build(childmap[i].count() > 0 ? *(childmap[i]) : pt);
      ring->translateZ(placeZ());
      rings_.push_back(ring);
      prevRing = ring;
    }
  }
};
