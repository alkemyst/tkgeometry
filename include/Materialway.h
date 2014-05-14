/**
 * @file Materialway.h
 *
 * @date 31/mar/2014
 * @author Stefano Martina
 */

#ifndef MATERIALWAY_H_
#define MATERIALWAY_H_

#include <map>
#include <vector>
#include <utility>
#include <set>
#include "DetectorModule.h"
#include "global_constants.h"
#include "Tracker.h"

namespace materialRouting {

  /**
   * @class Materialway
   * @brief Represents a track where the materials are
   * routed from the modules to the appropriate end
   *
   * The materialway is make up from single elements (Element),
   * every element point to the possible single next element.
   * Every module is linked to a materialway element where
   * it puts its materials to be routed, the material is then deposited
   * on the element and forwarded to the next element, passing through the
   * chain from an element to the subsequent one until the material reach
   * its designated end.
   * Every materialway element can perform some transformation on the routed
   * material before forward it to the next element.
   */
  class Materialway {
  private:
    enum Direction { HORIZONTAL, VERTICAL };

    /**
     * @class Section
     * @brief Represents a single element of the materialway
     */
    class Section {
    public:
      Section(int minZ, int minR, int maxZ, int maxR, Direction bearing, Section* nextSection);
      Section(int minZ, int minR, int maxZ, int maxR, Direction bearing);
      virtual ~Section();

      int isHit(int z, int r, int end, Direction aDirection) const;
      void minZ(int minZ);
      void minR(int minR);
      void maxZ(int maxZ);
      void maxR(int maxR);
      void bearing(Direction bearing);
      void nextSection(Section* nextSection);
      int minZ() const;
      int minR() const;
      int maxZ() const;
      int maxR() const;
      Direction bearing() const;
      Section* nextSection() const;
    private:
      int minZ_, minR_, maxZ_, maxR_;
      Section* nextSection_;
      Direction bearing_;
    }; //class Section

    /**
     * @class Boundary
     * @brief Represents a boundary where the services are routed around
     */
    class Boundary {
    public:
      Boundary();
      Boundary(int minZ, int minR, int maxZ, int maxR);
      virtual ~Boundary();

      int isHit(int z, int r, Direction aDirection) const;

      void minZ(int minZ);
      void minR(int minR);
      void maxZ(int maxZ);
      void maxR(int maxR);
      void outgoingSection(Section* outgoingSection);
      int minZ() const;
      int minR() const;
      int maxZ() const;
      int maxR() const;
      Section* outgoingSection();
    private:
      int minZ_, minR_, maxZ_, maxR_;
      Section* outgoingSection_;
    }; //class Boundary

    struct BoundaryComparator {
      bool operator()(const Boundary& one, const Boundary& two) {
        return ((one.maxZ() > two.maxZ()) || (one.maxR() > two.maxR()));
      }
    };

    typedef std::set<Boundary, BoundaryComparator> BoundariesSet;

    /**
     * @class SectionUsher
     * @brief Is the core of the functionality that builds sections across boundaries
     * starting from a point and ending to another section or to the upper right angle
     */
    class SectionUsher {
     public:
      SectionUsher(std::vector<Section>& sectionsList, BoundariesSet& boundariesList);
      virtual ~SectionUsher();

      void go(Boundary& boundary, Direction direction);         /**< start the process of section building, returns pointer to the first */
    private:
      std::vector<Section>& sectionsList_;
      BoundariesSet& boundariesList_;

      bool findBoundaryCollision(int& collision, int& border, int startZ, int startR, Direction direction);
      bool findSectionCollision(std::pair<int,Section*>& sectionCollision, int startZ, int startR, int end, Direction direction);
      bool buildSection(Section*& firstSection, Section*& lastSection, int& startZ, int& startR, int end, Direction direction);
      bool buildSectionPair(Section*& firstSection, Section*& lastSection, int& startZ, int& startR, int collision, int border, Direction direction);
      Section* splitSection(Section* section, int collision, Direction direction);
      Direction inverseDirection(Direction direction) const;
      void updateLastSectionPointer(Section* lastSection, Section* newSection);
    }; //class SectionUsher


  public:
    Materialway();
    virtual ~Materialway();

    bool build(Tracker& tracker);

  private:
    BoundariesSet boundariesList_;       /**< Vector for storing all the boundaries */
    std::vector<Section> sectionsList_;          /**< Vector for storing all the sections */

    SectionUsher usher;

    static const double gridFactor;                                     /**< the conversion factor for using integers in the algorithm (helps finding collisions),
                                                                            actually transforms millimiters in microns */
    static const int sectionWidth;     /**< the width of a section */
    static const int safetySpace;           /**< the safety space between sections */
    static const double globalMaxZ_mm;                     /**< the Z coordinate of the end point of the sections */
    static const double globalMaxR_mm;                   /**< the rho coordinate of the end point of the sections */
    static const int globalMaxZ;
    static const int globalMaxR;
    static const int boundaryPadding;
    static const int boundaryRightPadding;

    static int discretize(double input);
    static double undiscretize(int input);

    bool buildBoundaries(const Tracker& tracker);             /**< build the boundaries around barrels and endcaps */
    bool buildExternalSections(const Tracker& tracker);       /**< build the sections outside the boundaries */
    bool buildInternalSections(const Tracker& tracker);       /**< build the sections inside the boundaries */


    //std::map<BarrelModule&, Section*> barrelModuleSectionAssociations; /**< Map that associate each barrel module with the section that it feeds */
    //std::map<Boundary&, Section*> boundarySectionAssociations;         /**< Map that associate each boundary with the outgoing section (for the construction) */
  };

} /* namespace materialRouting */

#endif /* MATERIALWAY_H_ */
