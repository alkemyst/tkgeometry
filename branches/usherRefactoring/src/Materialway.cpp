/**
 * @file Materialway.cpp
 *
 * @date 31/mar/2014
 * @author Stefano Martina
 */

#include "Materialway.h"

namespace materialRouting {

  //=====================================================================================================================
  //START Materialway::Boundary
  Materialway::Boundary::Boundary(int minZ, int minR, int maxZ, int maxR)
  :minZ_(minZ), maxZ_(maxZ), minR_(minR), maxR_(maxR) {}

  Materialway::Boundary::Boundary() :minZ_(0), maxZ_(0), minR_(0), maxR_(0) {}
  Materialway::Boundary::~Boundary() {}


  int Materialway::Boundary::isHit(int z, int r, Direction aDirection) const {
    if (aDirection==HORIZONTAL) {
      if ((minR_<r)&&(maxR_>r)) {
        if (minZ_>z) return minZ_;
        else if (maxZ_>z) return -1;
      }
    } else {
      if ((minZ_<z)&&(maxZ_>z)) {
        if (minR_>r) return minR_;
        else if (maxR_>r) return -1;
      }
    }
    return 0;
  }

  void Materialway::Boundary::minZ(int minZ) {
    minZ_ = minZ;
  }
  void Materialway::Boundary::minR(int minR) {
    minR_ = minR;
  }
  void Materialway::Boundary::maxZ(int maxZ) {
    maxZ_ = maxZ;
  }
  void Materialway::Boundary::maxR(int maxR) {
    maxR_ = maxR;
  }
  void Materialway::Boundary::outgoingSection(Section* outgoingSection) {
    outgoingSection_ = outgoingSection;
  }
  int Materialway::Boundary::minZ() const {
    return minZ_;
  }
  int Materialway::Boundary::minR() const {
    return minR_;
  }
  int Materialway::Boundary::maxZ() const {
    return maxZ_;
  }
  int Materialway::Boundary::maxR() const {
    return maxR_;
  }
  Materialway::Section* Materialway::Boundary::outgoingSection() {
    return outgoingSection_;
  }
  //END Materialway::Boundary
  //=====================================================================================================================
  //START Materialway::Section
  Materialway::Section::Section(int minZ, int minR, int maxZ, int maxR, Direction bearing, Section* nextSection) :
              minZ_(minZ), minR_(minR), maxZ_(maxZ), maxR_(maxR), bearing_(bearing), nextSection_(nextSection) {}

  Materialway::Section::Section(int minZ, int minR, int maxZ, int maxR, Direction bearing) :
              Section(minZ, minR, maxZ, maxR, bearing, nullptr) {}
  Materialway::Section::~Section() {}

  int Materialway::Section::isHit(int z, int r, int end, Direction aDirection) const {
    if (aDirection==HORIZONTAL) {
      if ((minR() - sectionWidth - safetySpace < r)&&(maxR() + sectionWidth + safetySpace > r)) {
        if (minZ_>z){
          if (minZ() <= end + safetySpace) {
            return minZ();
          }
        } else if (maxZ()>z) {
          return -1;
        }
      }
    } else {
      if ((minZ() - sectionWidth - safetySpace < z)&&(maxZ() + sectionWidth + safetySpace > z)) {
        if (minR()>r) {
          if (minR() <= end + safetySpace) {
            return minR();
          }
        } else if (maxR()>r) {
          return -1;
        }
      }
    }
    return 0;
  }

  void Materialway::Section::minZ(int minZ){
    minZ_ = minZ;
  }
  void Materialway::Section::minR(int minR){
    minR_ = minR;
  }
  void Materialway::Section::maxZ(int maxZ){
    maxZ_ = maxZ;
  }
  void Materialway::Section::maxR(int maxR){
    maxR_ = maxR;
  }
  void Materialway::Section::bearing(Direction bearing) {
    bearing_ = bearing;
  }
  void Materialway::Section::nextSection(Section* nextSection) {
    nextSection_ = nextSection;
  }

  int Materialway::Section::minZ() const {
    return minZ_;
  }
  int Materialway::Section::minR() const {
    return minR_;
  }
  int Materialway::Section::maxZ() const {
    return maxZ_;
  }
  int Materialway::Section::maxR() const {
    return maxR_;
  }
  Materialway::Direction Materialway::Section::bearing() const {
    return bearing_;
  }
  Materialway::Section* Materialway::Section::nextSection() const {
    return nextSection_;
  }
  //END Materialway::Section
  //=====================================================================================================================
  //START Materialway::SectionUsher
  Materialway::SectionUsher::SectionUsher(std::vector<Section>& sectionsList, BoundariesSet& boundariesList) :
    sectionsList_(sectionsList),
    boundariesList_(boundariesList) {}
  Materialway::SectionUsher::~SectionUsher() {}

  void Materialway::SectionUsher::go(Boundary& boundary, Direction direction) {
    int startZ, startR, collision, border;
    bool foundBoundaryCollision, noSectionCollision;
    Section* lastSection = nullptr;
    Section* firstSection = nullptr;

    startZ = boundary.maxZ();
    startR = boundary.maxR();

    bool going=true;
    while (going) {
      foundBoundaryCollision = findBoundaryCollision(collision, border, startZ, startR, direction);
      noSectionCollision = buildSectionPair(firstSection, lastSection, startZ, startR, collision, border, direction);

      going = foundBoundaryCollision && noSectionCollision;
    }

    boundary.outgoingSection(firstSection);
  }

  /**
   * Look for the nearest collision in every defined boundary, starting from the (startZ, startR) point.
   * If no collision is found return the external perimeter coordinates
   * @param collision is a reference for the return value of the coordinate of the collision point (Z if horizontal, rho if vertical)
   * @param border is a reference for the return value of the coordinate of the border of boundary (rho if horizontal, Z if vertical)
   * @param startZ is the Z coordinate of the starting point
   * @param startR is the rho coordinate of the starting point
   * @param direction
   * @return true if a collision is found, false otherwise
   */
  bool Materialway::SectionUsher::findBoundaryCollision(int& collision, int& border, int startZ, int startR, Direction direction) {
    int hitCoord;
    std::map<int, BoundariesSet::const_iterator> hitBoundariesCoords;
    bool foundCollision = false;

    //test all the boundaries for an hit (keep it on a map ordered for key = collision coords, so the first is the nearest collision)
    //for (Boundary& currBoundary : boundariesList_) {
    for(BoundariesSet::const_iterator it = boundariesList_.cbegin(); it != boundariesList_.cend(); ++it) {
      hitCoord = it->isHit(startZ, startR, direction);
      if (hitCoord>0) {
        hitBoundariesCoords[hitCoord] = it;
      }
    }

    if (hitBoundariesCoords.size()) {
      collision = hitBoundariesCoords.begin()->first;
      if(direction == HORIZONTAL) {
        border = hitBoundariesCoords.begin()->second->maxR();
      } else {
        border = hitBoundariesCoords.begin()->second->maxZ();
      }
      foundCollision = true;
    } else {
      if(direction == HORIZONTAL) {
        collision = globalMaxZ;
        border = globalMaxR + safetySpace;
      } else {
        collision = globalMaxR;
        border = globalMaxZ + safetySpace;
      }
    }

    return foundCollision;
  }

  /**
   * Look for the nearest collision in every defined section, starting from the (startZ, startR) point.
   * @param sectionCollision is a reference for the return value of the coordinate of the collision point (Z if horizontal, rho if vertical)
   * and the pointer to the collided section
   * @param startZ is the Z coordinate of the starting point
   * @param startR is the rho coordinate of the starting point
   * @param end is the maximum range of searching for a collision, around (startZ, startR)
   * @param direction
   * @return true if a collision is found, false otherwise
   */
  bool Materialway::SectionUsher::findSectionCollision(std::pair<int,Section*>& sectionCollision, int startZ, int startR, int end, Direction direction) {
    int hitCoord;
    std::map<int, Section*> hitSectionsCoords;

    //test all the sections for an hit (keep it on a map ordered for key = collision coords, so the first is the nearest collision)
    for (Section& currSection : sectionsList_) {
      hitCoord = currSection.isHit(startZ, startR, end, direction);
      if (hitCoord>0) {
        hitSectionsCoords[hitCoord] = &currSection;
      }
    }

    if (hitSectionsCoords.size()) {
      sectionCollision = std::make_pair(hitSectionsCoords.begin()->first, hitSectionsCoords.begin()->second);
      return true;
    }
    return false;
  }

  /**
   * Build a section segment from the start point to the end in given direction. Also update the value of the start
   * with the new start point
   * @param firstSection is a pointer for returning the first built section
   * @param lastSection is a pointer to the previous section, is used for updating the nextSection pointer to this
   * @param startZ is a reference to the Z start coordinate
   * @param startR is a reference to the rho start coordinate
   * @param end is a reference to the end coordinate (Z if horizontal, rho if vertical)
   * @param direction is the direction
   * @return true if no section collision found, false otherwise
   */
  bool Materialway::SectionUsher::buildSection(Section*& firstSection, Section*& lastSection, int& startZ, int& startR, int end, Direction direction) {
    int minZ, minR, maxZ, maxR;
    int trueEnd = end;
    int cutCoordinate;
    Section* newSection;
    bool foundSectionCollision = false;
    bool returnValue = true;
    std::pair<int,Section*> sectionCollision;

    //search for collisions
    foundSectionCollision = findSectionCollision(sectionCollision, startZ, startR, end, direction);

    //if section collision found update the end
    if(foundSectionCollision) {
      trueEnd = sectionCollision.first - safetySpace;
      returnValue = false;
    }

    //set coordinates
    if (direction == HORIZONTAL) {
      minZ = startZ;
      minR = startR;
      maxZ = trueEnd;
      maxR = startR + sectionWidth;

      startZ = trueEnd + safetySpace;
    } else {
      minZ = startZ;
      minR = startR;
      maxZ = startZ + sectionWidth;
      maxR = trueEnd;

      startR = trueEnd + safetySpace;
    }

    //build section and update last section's nextSection pointer
    if ((maxZ > minZ) && (maxR > minR)) {
      newSection = new Section(minZ, minR, maxZ, maxR, direction);
      sectionsList_.push_back(*newSection);
      updateLastSectionPointer(lastSection, newSection);
      lastSection = newSection;
      //if firstSection is not yet set, set it
      if(firstSection == nullptr) {
        firstSection = newSection;
      }
    }

    //if found a collision and the section is perpendicular split the collided section and update nextSection pointer
    if(foundSectionCollision) {
      if (sectionCollision.second->bearing() != direction) {
        if (direction == HORIZONTAL) {
          cutCoordinate = minR;
        } else {
          cutCoordinate = minZ;
        }
        newSection = splitSection(sectionCollision.second, cutCoordinate, direction);
        sectionsList_.push_back(*newSection);
        updateLastSectionPointer(lastSection, newSection);
      } else {
        //set directly the pointer
        updateLastSectionPointer(lastSection, sectionCollision.second);
      }
    }

    return returnValue;
  }

  /**
   * Build a pair of section around a border in the shape of an L, the first section from the start point to the boundary collision,
   * the second section from the collision to the boundary border
   * @param firstSection is a pointer for returning the first built section
   * @param lastSection is a pointer to the previous section, is used for updating the nextSection pointer to this
   * @param startZ is a reference to the Z start coordinate
   * @param startR is a reference to the rho start coordinate
   * @param collision is a coordinate for the first collision point with boundary (Z if horizontal, rho if vertical)
   * @param border is a coordinate for the second point, the border of the boundary (rho if horizontal, Z if vertical)
   * @param direction is the direction
   * @return true if no section collision found, false otherwise
   */
  bool Materialway::SectionUsher::buildSectionPair(Section*& firstSection, Section*& lastSection, int& startZ, int& startR, int collision, int border, Direction direction) {

    bool noCollision = true;

    //build first section and if no collision happened build also the second section
    noCollision = buildSection(firstSection, lastSection, startZ, startR, collision - sectionWidth - safetySpace, direction);
    if (noCollision) {
      noCollision = buildSection(firstSection, lastSection, startZ, startR, border - safetySpace, inverseDirection(direction));
    }

    return noCollision;
  }

  Materialway::Section* Materialway::SectionUsher::splitSection(Section* section, int collision, Direction direction) {
    Section* retValue = nullptr;
    Section* useless = nullptr;
    int secMinZ = section->minZ();
    int secMinR = section->minR();
    int secCollision = collision;

      if (direction == HORIZONTAL) {
        buildSection(useless, retValue, secMinZ, secCollision, section->maxR(), inverseDirection(direction));

        section->maxR(collision - safetySpace);
        updateLastSectionPointer(section, retValue);
      } else {
        buildSection(useless, retValue, secCollision, secMinR, section->maxZ(), inverseDirection(direction));

        section->maxZ(collision - safetySpace);
        updateLastSectionPointer(section, retValue);
      }
      return retValue;
  }

  Materialway::Direction Materialway::SectionUsher::inverseDirection(Direction direction) const {
    return (direction == HORIZONTAL)?VERTICAL:HORIZONTAL;
  }

  void Materialway::SectionUsher::updateLastSectionPointer(Section* lastSection, Section* newSection) {
    if(lastSection != nullptr) {
      lastSection->nextSection(newSection);
    }
  }
  //END Materialway::SectionUsher
  //=====================================================================================================================
  //START Materialway

  const double Materialway::gridFactor = 1000.0;                                     /**< the conversion factor for using integers in the algorithm (helps finding collisions),
                                                                              actually transforms millimiters in microns */
  const int Materialway::sectionWidth = discretize(insur::volume_width);     /**< the width of a section */
  const int Materialway::safetySpace = discretize(insur::epsilon);           /**< the safety space between sections */
  const double Materialway::globalMaxZ_mm = insur::max_length;                     /**< the Z coordinate of the end point of the sections */
  const double Materialway::globalMaxR_mm = insur::outer_radius;                   /**< the rho coordinate of the end point of the sections */
  const int Materialway::globalMaxZ = discretize(globalMaxZ_mm);
  const int Materialway::globalMaxR = discretize(globalMaxR_mm);
  const int Materialway::boundaryPadding = discretize(5.0);
  const int Materialway::boundaryRightPadding = discretize(15.0);


  Materialway::Materialway() :
    usher(sectionsList_,boundariesList_),
    boundariesList_() {}
  Materialway::~Materialway() {}

  int Materialway::discretize(double input) {
    return int(input * gridFactor);
  }
  double Materialway::undiscretize(int input) {
    return double(input / gridFactor);
  }

  bool Materialway::build(Tracker& tracker) {
    bool retValue = false;

    if (buildBoundaries(tracker))
      retValue = buildExternalSections(tracker) && buildInternalSections(tracker);

    return retValue;
  }

  bool Materialway::buildBoundaries(const Tracker& tracker) {
    bool retValue = false;

    class BarrelVisitor : public ConstGeometryVisitor {
    public:
      BarrelVisitor(BoundariesSet& boundariesList) :
        boundariesList_(boundariesList) {}
      virtual ~BarrelVisitor() {}

      void visit(const Barrel& barrel) {
        int boundMinZ = discretize(barrel.minZ()) - boundaryPadding;
        int boundMinR = discretize(barrel.minR()) - boundaryPadding;
        int boundMaxZ = discretize(barrel.maxZ()) + boundaryRightPadding;
        int boundMaxR = discretize(barrel.maxR()) + boundaryPadding;

        boundariesList_.insert(Boundary(boundMinZ, boundMinR, boundMaxZ, boundMaxR));
      }

      void visit(const Endcap& endcap) {
        int boundMinZ = discretize(endcap.minZ()) - boundaryPadding;
        int boundMinR = discretize(endcap.minR()) - boundaryPadding;
        int boundMaxZ = discretize(endcap.maxZ()) + boundaryRightPadding;
        int boundMaxR = discretize(endcap.maxR()) + boundaryPadding;

        boundariesList_.insert(Boundary(boundMinZ, boundMinR, boundMaxZ, boundMaxR));
      }

    private:
      BoundariesSet& boundariesList_;
    };


    BarrelVisitor visitor (boundariesList_);
    tracker.accept(visitor);

    if (boundariesList_.size() > 0)
      retValue = true;

    return retValue;
  }

  bool Materialway::buildExternalSections(const Tracker& tracker) {
    bool retValue = false;

    //for(Boundary& boundary : boundariesList_) {
    for(BoundariesSet::iterator it = boundariesList_.begin(); it != boundariesList_.end(); ++it) {
      usher.go(const_cast<Boundary&>(*it), VERTICAL);
    }

  }

  bool Materialway::buildInternalSections(const Tracker& tracker) {
    return true;
  }


  //END Materialway

} /* namespace materialRouting */

