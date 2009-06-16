/**
 * @file MaterialProperties.cc
 * @brief This is the implementation for a base class for elements that have an influence on the material budget of a tracker layout
 */

#include <MaterialProperties.h>
namespace insur {
    /*-----public functions-----*/
    /**
     * The constructor sets a few defaults. The flags for the initialisation status of the material vectors are
     * set to false, the element category to none, i.e. unidentified, and the numeric values for the sums of masses
     * and the radiation and interaction lengths to -1 to indicate that they are all uninitialised.
     */
    MaterialProperties::MaterialProperties() {
        msl_set = false;
        mse_set = false;
        cat = no_cat;
        total_mass = -1;
        local_mass = -1;
        exiting_mass = -1;
        r_length = -1;
        i_length = -1;
    }
    
    /**
     * Get the category of this element.
     * @return The category identifier as defined in the enumeration <i>Category</i>
     */
    MaterialProperties::Category MaterialProperties::getCategory() { return cat; }
    
    /**
     * Set the category of this element.
     * @param c The category identifier as defined in the enumeration <i>Category</i>
     */
    void MaterialProperties::setCategory(Category c) { cat = c; }
    
    /**
     * Get the surface of this element that is relevant for tracking. This is a virtual function that is meant to be
     * implemented by the subclasses since the material properties as collected in this class know nothing about
     * the geometry of the element they belong to. As soon as a subclass contains the relevant information, it is
     * expected to provide a meaningful value that this function can return.
     * @return A constant value of <i>-1</i>
     */
    double MaterialProperties::getSurface() { return -1; }
    
    /**
     * Get the local mass of one of the materials, as identified by its name, that make up the element.
     * If the material does not appear on the list, the function throws an exception.
     * @param tag The name of the material
     * @return The mass of the requested material
     */
    double MaterialProperties::getLocalMass(std::string tag) {
        int index = findLocalIndex(tag);
        if (index < 0) throw std::runtime_error(err_local_mass + ": " + tag);
        return getLocalMass(index);
    }
    
    /**
     * Get the local mass of one of the materials, as identified by its internal index, that make up the element.
     * If the material does not appear on the list, the function throws an exception.
     * @param index The internal index of the material
     * @return The mass of the requested material
     */
    double MaterialProperties::getLocalMass(int index) {
        if (index < 0 || index >= (int)localmasses.size()) throw std::runtime_error(err_local_mass);
        return localmasses.at(index).second;
    }
    
    /**
     * Get the tag of one of the local materials that make up the element by its internal index. If the material
     * does not appear on the list, the function returns an empty string.
     * @param index The internal index of the material
     * @return The unique identifier of the requested material
     */
    std::string MaterialProperties::getLocalTag(int index) {
        std::string res;
        if ((index >= 0) && (index < (int)localmasses.size())) res = localmasses.at(index).first;
        return res;
    }
    
    /**
     * Get the exiting mass of one of the materials, as identified by its name, that make up the element.
     * If the material does not appear on the list, the function throws an exception.
     * @param tag The name of the material
     * @return The mass of the requested material
     */
    double MaterialProperties::getExitingMass(std::string tag) {
        int index = findExitingIndex(tag);
        if (index < 0) throw std::runtime_error(err_exiting_mass + ": " + tag);
        return getExitingMass(index);
    }
    
    /**
     * Get the exiting mass of one of the materials, as identified by its internal index, that make up the element.
     * If the material does not appear on the list, the function throws an exception.
     * @param index The internal index of the material
     * @return The mass of the requested material
     */
    double MaterialProperties::getExitingMass(int index) {
        if (index < 0 || index >= (int)exitingmasses.size()) throw std::runtime_error(err_exiting_mass);
        return exitingmasses.at(index).second;
    }
    
    /**
     * Get the tag of one of the exiting materials that make up the element by its internal index. If the material
     * does not appear on the list, the function returns an empty string.
     * @param index The internal index of the material
     * @return The unique identifier of the requested material
     */
    std::string MaterialProperties::getExitingTag(int index) {
        std::string res;
        if ((index >=0) && (index < (int)exitingmasses.size())) res = exitingmasses.at(index).first;
        return res;
    }
    
    /**
     * Set the local mass of one of the materials, as identified by its name, that make up the element.
     * If no material with the given name is found on the list, nothing happens.
     * @param tag The name of the material
     * @param ms The new mass of the material
     */
    void MaterialProperties::setLocalMass(std::string tag, double ms) {
        std::pair<std::string, double> p;
        setLocalMass(p);
    }
    
    /**
     * Add the local mass for a material, as specified by its tag, to the internal list.
     * If the given material is already listed with a mass value, the new value is added
     * to the existing one.
     * @param tag The name of the material
     * @param ms The mass value
     */
    void MaterialProperties::addLocalMass(std::string tag, double ms) {
        std::pair<std::string, double> p(tag, ms);
        addLocalMass(p);
    }
    
    /**
     * Set the exiting mass of one of the materials, as identified by its name, that make up the element.
     * If no material with the given name is found on the list, nothing happens.
     * @param tag The name of the material
     * @param ms The new mass of the material
     */
    void MaterialProperties::setExitingMass(std::string tag, double ms) {
        std::pair<std::string, double> p;
        setExitingMass(p);
    }
    
    /**
     * Add the exiting mass for a material, as specified by its tag, to the internal list.
     * If the given material is already listed with a mass value, the new value is added
     * to the existing one.
     * @param tag The name of the material
     * @param ms The mass value
     */
    void MaterialProperties::addExitingMass(std::string tag, double ms) {
        std::pair<std::string, double> p(tag, ms);
        addExitingMass(p);
    }
    
    /**
     * Get the number of registered local masses for the materials found in the inactive element.
     * @return The size of the internal mass vector
     */
    unsigned int MaterialProperties::localMassCount() { return localmasses.size(); }
    
    /**
     * Get the number of registered exiting masses for the materials found in the inactive element.
     * @return The size of the internal mass vector
     */
    unsigned int MaterialProperties::exitingMassCount() { return exitingmasses.size(); }
    
    /**
     * Reset the state of the internal mass vector to empty, discarding all entries.
     */
    void MaterialProperties::clearMassVectors() {
        localmasses.clear();
        exitingmasses.clear();
    }
    
    /**
     * Copy the entire mass vector to another instance of <i>MaterialProperties</i>
     * @param mp The destination object
     */
    void MaterialProperties::copyMassVectors(MaterialProperties& mp) {
        mp.clearMassVectors();
        for (unsigned int i = 0; i < localMassCount(); i++) mp.addLocalMass(localmasses.at(i).first, localmasses.at(i).second);
        for (unsigned int i = 0; i < exitingMassCount(); i++) mp.addExitingMass(exitingmasses.at(i).first, exitingmasses.at(i).second);
        if (localmasses.size() > 0) msl_set = true;
        if (exitingmasses.size() > 0) mse_set = true;
    }
    
    /**
     * Get the cumulative mass of the inactive element.
     * @return The overall mass, taking into account all registered materials; -1 if the value has not yet been computed
     */
    double MaterialProperties::getTotalMass() { return total_mass; }
    
    /**
     * Get the local mass of the inactive element.
     * @return The overall mass, taking into account all registered materials; -1 if the value has not yet been computed
     */
    double MaterialProperties::getLocalMass() { return local_mass; }
    
    /**
     * Get the exiting mass of the inactive element.
     * @return The overall mass, taking into account all registered materials; -1 if the value has not yet been computed
     */
    double MaterialProperties::getExitingMass() { return exiting_mass; }
    
    /**
     * Get the radiation length of the inactive element.
     * @return The overall radiation length, taking into account all registered materials; -1 if the value has not yet been computed
     */
    double MaterialProperties::getRadiationLength() { return r_length; }
    
    /**
     * Get the intraction length of the inactive element.
     * @return The overall radiation length, taking into account all registered materials; -1 if the value has not yet been computed
     */
    double MaterialProperties::getInteractionLength() { return i_length; }
    
    /**
     * Calculate the overall mass of the inactive element from the internal mass vectors,
     * if the one of the mass vectors has at least one element in it, and an offset value.
     * @param offset A starting value for the calculation
     */
    void MaterialProperties::calculateTotalMass(double offset) {
        calculateLocalMass(offset);
        calculateExitingMass(offset);
        if (msl_set && mse_set) total_mass = local_mass + exiting_mass + offset;
        else if (!msl_set) total_mass = exiting_mass + offset;
        else if (!mse_set) total_mass = local_mass + offset;
        else total_mass = offset;
    }
    
    /**
     * Calculate the overall local mass of the inactive element from the internal mass vector,
     * if that mass vectors has at least one element in it, and an offset value.
     * @param offset A starting value for the calculation
     */
    void MaterialProperties::calculateLocalMass(double offset) {
        if (msl_set) {
            local_mass = offset;
            for (unsigned int i = 0; i < localmasses.size(); i++) local_mass = local_mass + localmasses.at(i).second;
        }
    }
    
    
    /**
     * Calculate the overall exiting mass of the inactive element from the internal mass vector,
     * if that mass vectors has at least one element in it, and an offset value.
     * @param offset A starting value for the calculation
     */
    void MaterialProperties::calculateExitingMass(double offset) {
        if (mse_set) {
            exiting_mass = offset;
            for (unsigned int i = 0; i < exitingmasses.size(); i++) exiting_mass = exiting_mass + exitingmasses.at(i).second;
        }
    }
    /**
     * Calculate the overall radiation length of the inactive element from the material table and the material vectors,
     * if they have at least one element in them, and an offset value.
     * @param materials A reference to the external material table
     * @param offset A starting value for the calculation
     */
    void MaterialProperties::calculateRadiationLength(MaterialTable& materials, double offset) {
        if (getSurface() > 0) {
            r_length = offset;
            if (msl_set) {
                for (unsigned int i = 0; i < localmasses.size(); i++) {
                    try {
                        r_length = r_length + localmasses.at(i).second / (materials.getMaterial(localmasses.at(i).first).rlength * getSurface() / 100.0);
                    }
                    catch(std::runtime_error& re) {
                        std::cerr << re.what() << std::endl;
                    }
                    catch(std::exception& e) {
                        std::cout << msg_mattab_except_local << e.what() << std::endl;
                    }
                }
            }
            if (mse_set) {
                for (unsigned int i = 0; i < exitingmasses.size(); i++) {
                    try {
                        r_length = r_length + exitingmasses.at(i).second / (materials.getMaterial(exitingmasses.at(i).first).rlength * getSurface() / 100.0);
                    }
                    catch(std::runtime_error& re) {
                        std::cerr << re.what() << std::endl;
                    }
                    catch(std::exception& e) {
                        std::cout << msg_mattab_except_exiting << e.what() << std::endl;
                    }
                }
            }
        }
    }
    
    /**
     * Calculate the overall interaction length of the inactive element from the material table and the material vectors,
     * if they have at least one element in them, and an offset value.
     * @param materials A reference to the external material table
     * @param offset A starting value for the calculation
     */
    void MaterialProperties::calculateInteractionLength(MaterialTable& materials, double offset) {
        if (getSurface() > 0) {
            i_length = offset;
            if (msl_set) {
                for (unsigned int i = 0; i < localmasses.size(); i++) {
                    try {
                        i_length = i_length + localmasses.at(i).second / (materials.getMaterial(localmasses.at(i).first).ilength * getSurface() / 100.0);
                    }
                    catch(std::runtime_error& re) {
                        std::cerr << re.what() << std::endl;
                    }
                    catch(std::exception& e) {
                        std::cout << msg_mattab_except_local << e.what() << std::endl;
                    }
                }
            }
            if (mse_set) {
                for (unsigned int i = 0; i < exitingmasses.size(); i++) {
                    try {
                        i_length = i_length + exitingmasses.at(i).second / (materials.getMaterial(exitingmasses.at(i).first).ilength * getSurface() / 100.0);
                    }
                    catch(std::runtime_error& re) {
                        std::cerr << re.what() << std::endl;
                    }
                    catch(std::exception& e) {
                        std::cout << msg_mattab_except_exiting << e.what() << std::endl;
                    }
                }
            }
        }
    }
    
    /**
     * Print the material properties.
     */
    void MaterialProperties::print() {
        std::cout << "Material properties (current state)" << std::endl;
        std::cout << "localmasses: vector with " << localmasses.size() << " elements." << std::endl;
        for (unsigned int i = 0; i < localmasses.size(); i++) {
            std::cout << "element " << i << "(material, mass): (" << localmasses.at(i).first << ", " << localmasses.at(i).second << ")" << std::endl;
        }
        std::cout << "exitingmasses: vector with " << exitingmasses.size() << " elements." << std::endl;
        for (unsigned int i = 0; i < exitingmasses.size(); i++) {
            std::cout << "element " << i << "(material, mass): (" << exitingmasses.at(i).first << ", " << exitingmasses.at(i).second << ")" << std::endl;
        }
        std::cout << "total_mass = " << total_mass << std::endl;
        std::cout << "local_mass = " << local_mass << std::endl;
        std::cout << "exiting_mass = " << exiting_mass << std::endl;
        std::cout << "r_length = " << r_length << std::endl;
        std::cout << "i_length = " <<i_length  << std::endl;
    }
    
    /*-----protected-----*/
    /**
     * Set the local mass of one of the materials, as identified by its name, that make up the element.
     * If no material with the given name is found on the list, nothing happens.
     * @param ms The <i>string, double</i> pair that defines an entry in the mass vector
     */
    void MaterialProperties::setLocalMass(std::pair<std::string, double> ms) {
        int index = findLocalIndex(ms.first);
        if (index >= 0)  {
            localmasses.at(index) = ms;
            msl_set = true;
        }
    }
    
    /**
     * Add the local mass for a material, as specified by its tag, to the internal list.
     * If the given material is already listed with a mass value, that value is replaced.
     * @param tk The <i>string, double</i> pair that defines an entry in the mass vector
     */
    void MaterialProperties::addLocalMass(std::pair<std::string, double> ms) {
        if (newLocalMaterial(ms.first)) {
            localmasses.push_back(ms);
            msl_set = true;
        }
        else {
            setLocalMass(ms.first, getLocalMass(ms.first) + ms.second);
        }
    }
    
    /**
     * Set the exiting mass of one of the materials, as identified by its name, that make up the element.
     * If no material with the given name is found on the list, nothing happens.
     * @param ms The <i>string, double</i> pair that defines an entry in the mass vector
     */
    void MaterialProperties::setExitingMass(std::pair<std::string, double> ms) {
        int index = findExitingIndex(ms.first);
        if (index >= 0)  {
            exitingmasses.at(index) = ms;
            mse_set = true;
        }
    }
    
    /**
     * Add the exiting mass for a material, as specified by its tag, to the internal list.
     * If the given material is already listed with a mass value, that value is replaced.
     * @param tk The <i>string, double</i> pair that defines an entry in the mass vector
     */
    void MaterialProperties::addExitingMass(std::pair<std::string, double> ms) {
        if (newExitingMaterial(ms.first)) {
            exitingmasses.push_back(ms);
            mse_set = true;
        }
        else {
            setExitingMass(ms.first, getExitingMass(ms.first) + ms.second);
        }
    }
    
    /**
     * Find the index of an entry in the local mass vector from the material tag.
     * @param The name of the material
     * @return The material index in the internal mass vector; -1 if the material is not listed
     */
    int MaterialProperties::findLocalIndex(std::string tag) {
        bool found = false;
        int index = 0;
        while ((index < (int)localmasses.size()) && !found) {
            if (tag.compare(localmasses.at(index).first) == 0) found = true;
            else index++;
        }
        if (!found) return -1;
        return index;
    }
    
    /**
     * Find the index of an entry in the exiting mass vector from the material tag.
     * @param The name of the material
     * @return The material index in the internal mass vector; -1 if the material is not listed
     */
    int MaterialProperties::findExitingIndex(std::string tag) {
        bool found = false;
        int index = 0;
        while ((index < (int)exitingmasses.size()) && !found) {
            if (tag.compare(exitingmasses.at(index).first) == 0) found = true;
            else index++;
        }
        if (!found) return -1;
        return index;
    }
    
    /**
     * Check if a material is already listed with in the local mass vector
     * @param tag The material name
     * @return True if the material is not listed in the vector, false otherwise
     */
    bool MaterialProperties::newLocalMaterial(std::string tag) {
        for (unsigned int i = 0; i < localmasses.size(); i++) {
            if (tag.compare(localmasses.at(i).first) == 0) return false;
        }
        return true;
    }
    
    /**
     * Check if a material is already listed with in the exiting mass vector
     * @param tag The material name
     * @return True if the material is not listed in the vector, false otherwise
     */
    bool MaterialProperties::newExitingMaterial(std::string tag) {
        for (unsigned int i = 0; i < exitingmasses.size(); i++) {
            if (tag.compare(exitingmasses.at(i).first) == 0) return false;
        }
        return true;
    }
}
