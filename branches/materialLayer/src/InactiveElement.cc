/**
 * @file InactiveElement.cc
 * @brief This is the base class implementation of a single inactive element
 */

#include <InactiveElement.h>
namespace insur {
    /*-----public functions-----*/
    /**
     * The constructor sets some defaults: no neighbours, intermediate element.
     */
    InactiveElement::InactiveElement() {
        isFinal_ = false;
        feederType_ = no_in;
        feederIndex_ = -1;
        neighbourType_ = no_in;
        neighbourIndex_ = -1;
    }
    
    /**
     * Get the surface of the element that is relevant for material budget calculation.
     * @return The average cylinder surface for a horizontal tube, the disc surface for a vertical disc
     */
    double InactiveElement::getSurface() {
        if (isVertical()) return ((iRadius_ + wRadius_) * (iRadius_ + wRadius_) - iRadius_ * iRadius_) * PI;
        else return 2 * PI * (iRadius_ + wRadius_ / 2.0) * zLength_;
    }
    
    /**
     * Get the orientation of the object.
     * @return True if the element points up or down, false if it points sideways
     */
    bool InactiveElement::isVertical() { return isVertical_; }
    
    /**
     * Set the orientation flag of the object.
     * @param vertical The new value for the up/down flag
     */
    void InactiveElement::setVertical(bool vertical) { isVertical_ = vertical; }
    
    /**
     * Check if the content of this element travels out of the tracking volume after this.
     * @return True if the element is not a neighbour to anything, false otherwise
     */
    bool InactiveElement::isFinal() { return isFinal_; }
    
    /**
     * Set if the content of this element travels out of the tracking volume after this.
     * @param final Should be true if the element is not a neighbour to anything and false otherwise
     */
    void InactiveElement::setFinal(bool final) { isFinal_ = final; }
    
    /**
     * Get the distance of this object's leftmost point to the xy-plane.
     * @return The offset from the origin along the z-axis
     */
    double InactiveElement::getZOffset() { return zOffset_; }
    
    /**
     * Set the distance of this object's leftmost point to the xy-plane.
     * @param zoffset The offset from the origin along the z-axis
     */
    void InactiveElement::setZOffset(double zoffset) { zOffset_ = zoffset; }
    
    /**
     * Get the length of the element.
     * @return The total length of the element along the z-axis
     */
    double InactiveElement::getZLength() { return zLength_; }
    
    /**
     * Set the length of the element.
     * @param zlength The total length of the element along the z-axis
     */
    void InactiveElement::setZLength(double zlength) { zLength_ = zlength; }
    
    /**
     * Get the inner radius of the element.
     * @return The distance from the z-axis to the innermost point of the element
     */
    double InactiveElement::getInnerRadius() { return iRadius_; }
    
    /**
     * Set the inner radius of the element.
     * @param iradius The distance from the z-axis to the innermost point of the element
     */
    void InactiveElement::setInnerRadius(double iradius) { iRadius_ = iradius; }
    
    /**
     * Get the width of the element.
     * @return The distance from the innermost to the outermost point of the element in the xy-plane
     */
    double InactiveElement::getRWidth() { return wRadius_; }
    
    /**
     * Set the width of the element.
     * @param rwidth The distance from the innermost to the outermost point of the element in the xy-plane
     */
    void InactiveElement::setRWidth(double rwidth) { wRadius_ = rwidth; }
    
    /**
     * Get the index of the element's feeder volume.
     * @return The index within the tracker object's layer or disc vector, or of the service volume; -1 if there is none
     */
    int InactiveElement::getFeederIndex() { return feederIndex_; }
    
    /**
     * Set the index of the element's feeder volume.
     * @param layer The index within the tracker object's layer or disc vector, or of the service volume
     */
    void InactiveElement::setFeederIndex(int layer) { feederIndex_ = layer; }
    
    /**
     * Get the type of the element's feeder volume.
     * @return The type of feeder as listed in the enumeration <i>InType</i>
     */
    InactiveElement::InType InactiveElement::getFeederType() { return feederType_; }
    
    /**
     * Set the type of the element's feeder volume.
     * @param type The type of feeder as listed in the enumeration <i>InType</i>
     */
    void InactiveElement::setFeederType(InType type) { feederType_ = type; }
    
    /**
     * Get the index of the element's neighbour volume.
     * @return The index of the previous service volume
     */
    int InactiveElement::getNeighbourIndex() { return neighbourIndex_; }
    
    /**
     * Set the index of the element's neighbour volume.
     * @param previous The index of the previous service volume
     */
    void InactiveElement::setNeighbourIndex(int previous) { neighbourIndex_ = previous; }
    
    /**
     * Get the type of the element's neighbour volume.
     * @return The type of neighbour as listed in the enumeration <i>InType</i>
     */
    InactiveElement::InType InactiveElement::getNeighbourType() { return neighbourType_; }
    
    /**
     * Set the type of the element-s neighbour volume.
     * @param type The type of neighbour as listed in the enumeration <i>InType</i>
     */
    void InactiveElement::setNeighbourType(InactiveElement::InType type) { neighbourType_ = type; }
    
    /**
     * Set the total mass of the inactive element.
     * @param mass The new overall mass
     */
    void InactiveElement::setTotalMass(double mass) { total_mass = mass; }
    
    /**
     * Set the total mass of the inactive element.
     * @param mass The new overall mass
     */
    void InactiveElement::setLocalMass(double mass) { local_mass = mass; }
    
    /**
     * Set the total mass of the inactive element.
     * @param mass The new overall mass
     */
    void InactiveElement::setExitingMass(double mass) { exiting_mass = mass; }
    
    /**
     * Set the radiation length of the inactive element.
     * @param rlength The new overall radiation length, averaged over all the different material that occur in the inactive element
     */
    void InactiveElement::setRadiationLength(double rlength) { r_length = rlength; }
    
    /**
     * Set the interaction length of the inactive element.
     * @param ilength The new overall interaction length, averaged over all the different material that occur in the inactive element
     */
    void InactiveElement::setInteractionLength(double ilength) { i_length = ilength; }
    
    /**
     * Calculate and return the Eta range of the element
     * @return The pair <i>(Eta_min, Eta_max)</i>
     */
    std::pair<double, double> InactiveElement::getEtaMinMax() {
        std::pair<double, double> res;
        double theta0, theta1;
        // volumes crossing z=0
        if ((getZOffset() < 0) && (getZOffset() + getZLength() > 0)) {
            // lower left of tube wall above z-axis
            theta0 = atan(getInnerRadius() / (-1 * getZOffset()));
            theta0 = PI - theta0;
            // lower right of tube wall above z-axis
            theta1 = atan(getInnerRadius() / (getZOffset() + getZLength()));
        }
        // volumes on either side of z=0
        else {
            // rings
            if (isVertical()) {
                // upper centre of tube wall above z-axis
                theta0 = atan((getInnerRadius() + getRWidth()) / (getZLength() / 2.0 + getZOffset()));
                // lower centre of tube wall above z-axis
                theta1 = atan(getInnerRadius() / (getZLength() / 2.0 + getZOffset()));
            }
            // tubes
            else {
                // centre left of tube wall above z-axis
                theta0 = atan((getRWidth() / 2.0 + getInnerRadius()) / getZOffset());
                // centre right of tube wall above z-axis
                theta1 = atan((getRWidth() / 2.0 + getInnerRadius()) / (getZOffset() + getZLength()));
            }
        }
        // convert angle theta to pseudorapidity eta
        res.first = -1 * log(tan(theta0 / 2.0));
        res.second = -1 * log(tan(theta1 / 2.0));
        return res;
    }
    
    /**
     * Print the geometry-specific parameters of the inactive element including the orientation.
     */
    void InactiveElement::print() {
        MaterialProperties::print();
        std::cout << "Inactive element properties (current state)" << std::endl;
        std::cout << "z_offset = " << zOffset_ << std::endl;
        std::cout << "z_length = " << zLength_ << std::endl;
        std::cout << "i_radius = " <<iRadius_  << std::endl;
        std::cout << "w_radius = " << wRadius_ << std::endl;
        if (isVertical()) std::cout << "Volume is vertical." << std::endl;
        else std::cout << "Volume is horizontal" << std::endl;
    }
}
