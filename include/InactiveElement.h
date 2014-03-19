//
// File:   InactiveElement.h
// Author: ndemaio
//
// Created on October 16, 2008, 5:11 PM
//

/**
 * @file InactiveElement.h
 * @brief This is the base class header file for a single inactive element
 */

#ifndef _INACTIVEELEMENT_H
#define	_INACTIVEELEMENT_H

#include <cmath>
#include <iostream>
#include <MaterialProperties.h>
#include <global_constants.h>
namespace insur {
  /**
   * @class InactiveElement
   * @brief This is the base class for the elements that make up the inactive surfaces.
   *
   * Since all inactive elements are simplified to tube shapes in the geometrical model, the geometry parameters have been
   * packed into the base class. All of these parameters apply to descendants of a different shape as well, though, because
   * they describe relations between the object and the origin, not between points within the object.
   */
  class InactiveElement : public MaterialProperties {
  public:
    /**
     * @enum InType A list of the various types of neighbour or feeder an element can have
     */
    enum InType { no_in, tracker, barrel, endcap };
    InactiveElement();
    virtual ~InactiveElement() {}
    virtual double getSurface();
    bool isVertical();
    void setVertical(bool vertical);
    bool isFinal();
    void setFinal(bool final);
    double getZOffset();
    void setZOffset(double zoffset);
    double getZLength();
    void setZLength(double zlength);
    double getInnerRadius();
    void setInnerRadius(double iradius);
    double getRWidth();
    void setRWidth(double width);
    int getFeederIndex();
    void setFeederIndex(int feederIndex);
    InType getFeederType();
    void setFeederType(InType type);
    int getNeighbourIndex();
    InType getNeighbourType();
    void setNeighbourType(InType type);
    void setNeighbourIndex(int previous);
    void setTotalMass(double mass);
    void setLocalMass(double mass);
    void setExitingMass(double mass);
    void setRadiationLength(double rlength);
    void setInteractionLength(double ilength);
    std::pair<double, double> getEtaMinMax();
    virtual void print();
  protected:
    bool isVertical_, isFinal_;
    double zOffset_, zLength_, iRadius_, wRadius_;
    int feederIndex_, neighbourIndex_;
    InType feederType_, neighbourType_;
  };
}
#endif	/* _INACTIVEELEMENT_H */

