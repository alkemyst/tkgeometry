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
    MaterialSection(double minZ, double minR, double maxZ, double maxR, Direction bearing, MaterialSection* nextSection);
    MaterialSection(double minZ, double minR, double maxZ, double maxR, Direction bearing);
    virtual ~MaterialSection();

    double isHit(double z, double r, double end, Direction direction);
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

    void getServicesAndPass(MaterialObject& source);
  }
}

#endif /* MATERIALSECTION_H_ */

