/**
 * @file MaterialSection.h
 *
 * @date 17/Oct/2014
 * @author Stefano Martina
 */

#ifndef MATERIALSECTION_H_
#define MATERIALSECTION_H_

namespace material {
  class MaterialSection : public MaterialObject {
  public:
    MaterialSection(int minZ, int minR, int maxZ, int maxR, Direction bearing, Section* nextSection);
    MaterialSection(int minZ, int minR, int maxZ, int maxR, Direction bearing);
    virtual ~MaterialSection();

    int isHit(int z, int r, int end, Direction direction);
    nextSection(MaterialSection* nextSection);
    MaterialSection* nextSection();
    inactiveElement(InactiveElement* inactiveElement);
    InactiveElement* inactiveElement();

    virtual void getServicesAndPass(MaterialObject& source);
  protected:
    Property<double, noDefault> minZ_;
    Property<double, noDefault> maxZ_;
    Property<double, noDefault> minR_;
    Property<double, noDefault> maxR_;
    Property<Direction, noDefault> bearing_;
    MaterialSection* nextSection_;
    InactiveElement* inactiveElement_;
  }

  class MaterialStation : public MaterialSection {
  public:
    MaterialStation();
    virtual ~MaterialStation();
    
  }
}

#endif /* MATERIALSECTION_H_ */

