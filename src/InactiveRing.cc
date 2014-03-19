/**
 * @file InactiveRing.cc
 * @brief This is the derived class implementation of a single ring-shaped inactive element
 */

#include <InactiveRing.h>
namespace insur {
    /**
     * The constructors make sure the UP/DOWN flag is set correctly (it is provided by the parent class, but not set).
     */
    InactiveRing::InactiveRing() : InactiveElement() {
        isVertical_ = true;
    }
    
    InactiveRing::InactiveRing(InactiveElement& previous) : InactiveElement(previous) {
        isVertical_ = true;
    }
    
    /**
     * Nothing to do for the destructor...
     */
    InactiveRing::~InactiveRing() {}
    
    /**
     * This function prints a representation of the element to <i>cout</i>
     */
    void InactiveRing::print() {
        InactiveElement::print();
        if (sanityCheck()) std::cout << "Volume is sane." << std::endl;
        else std::cout << "WARNING: Volume is not sane!" << std::endl;
    }
    
    /**
     * The sanity check virtual function tests if the object has the geometric properties of a ring.
     * @return True if the length is less than or equal to the width, false otherwise
     */
    bool InactiveRing::sanityCheck() {
        return (zLength_ <= wRadius_) && isVertical_;
    }
}
