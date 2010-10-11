/**
 * @file Analyzer.cc
 * @brief This is the implementation of the class that analyses a material budget
 */
#include <TH1D.h>
#include <TH2D.h>
#include <Analyzer.h>
#include <TProfile.h>

#define MATERIAL_SHADOW

namespace insur {
    
    // public
    /**
     * A comparison function for the first elements in two pairs of integers.
     * @param p The first pair
     * @param q The second pair
     * @return True if <i>p.first</i> is smaller than <i>q.first</i>, false otherwise
     */
    bool compareIntPairFirst(std::pair<int, int> p, std::pair<int, int> q) {
        return (p.first < q.first);
    }
    
    /**
     * A comparison function for the second elements in two pairs of integers.
     * @param p The first pair
     * @param q The second pair
     * @return True if <i>p.second</i> is smaller than <i>q.second</i>, false otherwise
     */
    bool compareIntPairSecond(std::pair<int, int> p, std::pair<int, int> q) {
        return (p.second < q.second);
    }
    
    /**
     * The constructor sets a number of internal constants.
     */
    Analyzer::Analyzer() {
        // Not strictly necessary, but it's useful to keep
        // the color the same for the most used module types
        lastPickedColor = STARTCOLOR;
        colorPicker("pt");
        colorPicker("rphi");
        colorPicker("stereo");
        geomLite = NULL;   geomLiteCreated=false;
        geomLiteXY = NULL; geomLiteXYCreated=false;
        geomLiteYZ = NULL; geomLiteYZCreated=false;
        geomLiteEC = NULL; geomLiteECCreated=false;
        geometryTracksUsed = 0;
        materialTracksUsed = 0;
    }
    
    /**
     * The main analysis function provides a frame for the scan in eta, defers summing up the radiation
     * and interaction lengths for each volume category to subfunctions, and sorts those results into the
     * correct histograms.
     * @param mb A reference to the instance of <i>MaterialBudget</i> that is to be analysed
     * @param momenta A list of momentum values for the tracks that are shot through the layout
     * @param etaSteps The number of wedges in the fan of tracks covered by the eta scan
     * @param A pointer to a second material budget associated to a pixel detector; may be <i>NULL</i>
     */
    void Analyzer::analyzeMaterialBudget(MaterialBudget& mb, std::vector<double>& momenta, int etaSteps,
            MaterialBudget* pm) {
        materialTracksUsed = etaSteps;
#ifdef DEBUG_PERFORMANCE
        struct tm *localt; // timing: debug
        time_t t;          // timing: debug
        t = time(NULL);
        localt = localtime(&t);
        //std::cerr << asctime(localt) << std::endl;
        clock_t starttime = clock();
#endif
        int nTracks;
        double etaStep, eta, theta, phi;
        clearMaterialBudgetHistograms();
        clearCells();
        // prepare etaStep, phiStep, nTracks, nScans
        if (etaSteps > 1) etaStep = etaMax / (double)(etaSteps - 1);
        else etaStep = etaMax;
        nTracks = etaSteps;
        // reset the number of bins and the histogram boundaries (0.0 to etaMax) for all histograms, recalculate the cell boundaries
        setHistogramBinsBoundaries(nTracks, 0.0, etaMax);
        setCellBoundaries(nTracks, 0.0, outer_radius + volume_width, 0.0, etaMax);
        // reset the list of tracks
        tv.clear();
        // used fixed phi
        phi = PI / 2.0;
        //      loop over nTracks (eta range [0, etaMax])
        for (int i_eta = 0; i_eta < nTracks; i_eta++) {
            Material tmp;
            Track track;
            eta = i_eta * etaStep;
            theta = 2 * atan(pow(E, -1 * eta)); // TODO: switch to exp() here
            track.setTheta(theta);
            //      active volumes, barrel
            tmp = analyzeModules(mb.getBarrelModuleCaps(), eta, theta, phi, track);
            ractivebarrel.Fill(eta, tmp.radiation);
            iactivebarrel.Fill(eta, tmp.interaction);
            rbarrelall.Fill(eta, tmp.radiation);
            ibarrelall.Fill(eta, tmp.interaction);
            ractiveall.Fill(eta, tmp.radiation);
            iactiveall.Fill(eta, tmp.interaction);
            rglobal.Fill(eta, tmp.radiation);
            iglobal.Fill(eta, tmp.interaction);
            //      active volumes, endcap
            tmp = analyzeModules(mb.getEndcapModuleCaps(), eta, theta, phi, track);
            ractiveendcap.Fill(eta, tmp.radiation);
            iactiveendcap.Fill(eta, tmp.interaction);
            rendcapall.Fill(eta, tmp.radiation);
            iendcapall.Fill(eta, tmp.interaction);
            ractiveall.Fill(eta, tmp.radiation);
            iactiveall.Fill(eta, tmp.interaction);
            rglobal.Fill(eta, tmp.radiation);
            iglobal.Fill(eta, tmp.interaction);
            //      services, barrel
            tmp = analyzeInactiveSurfaces(mb.getInactiveSurfaces().getBarrelServices(), eta, theta, track);
            rserfbarrel.Fill(eta, tmp.radiation);
            iserfbarrel.Fill(eta, tmp.interaction);
            rbarrelall.Fill(eta, tmp.radiation);
            ibarrelall.Fill(eta, tmp.interaction);
            rserfall.Fill(eta, tmp.radiation);
            iserfall.Fill(eta, tmp.interaction);
            rglobal.Fill(eta, tmp.radiation);
            iglobal.Fill(eta, tmp.interaction);
            //      services, endcap
            tmp = analyzeInactiveSurfaces(mb.getInactiveSurfaces().getEndcapServices(), eta, theta, track);
            rserfendcap.Fill(eta, tmp.radiation);
            iserfendcap.Fill(eta, tmp.interaction);
            rendcapall.Fill(eta, tmp.radiation);
            iendcapall.Fill(eta, tmp.interaction);
            rserfall.Fill(eta, tmp.radiation);
            iserfall.Fill(eta, tmp.interaction);
            rglobal.Fill(eta, tmp.radiation);
            iglobal.Fill(eta, tmp.interaction);
            //      supports, barrel
            tmp = analyzeInactiveSurfaces(mb.getInactiveSurfaces().getSupports(), eta, theta, track, MaterialProperties::b_sup);
            rlazybarrel.Fill(eta, tmp.radiation);
            ilazybarrel.Fill(eta, tmp.interaction);
            rbarrelall.Fill(eta, tmp.radiation);
            ibarrelall.Fill(eta, tmp.interaction);
            rlazyall.Fill(eta, tmp.radiation);
            ilazyall.Fill(eta, tmp.interaction);
            rglobal.Fill(eta, tmp.radiation);
            iglobal.Fill(eta, tmp.interaction);
            //      supports, endcap
            tmp = analyzeInactiveSurfaces(mb.getInactiveSurfaces().getSupports(), eta, theta, track, MaterialProperties::e_sup);
            rlazyendcap.Fill(eta, tmp.radiation);
            ilazyendcap.Fill(eta, tmp.interaction);
            rendcapall.Fill(eta, tmp.radiation);
            iendcapall.Fill(eta, tmp.interaction);
            rlazyall.Fill(eta, tmp.radiation);
            ilazyall.Fill(eta, tmp.interaction);
            rglobal.Fill(eta, tmp.radiation);
            iglobal.Fill(eta, tmp.interaction);
            //      supports, tubes
            tmp = analyzeInactiveSurfaces(mb.getInactiveSurfaces().getSupports(), eta, theta, track, MaterialProperties::o_sup);
            rlazytube.Fill(eta, tmp.radiation);
            ilazytube.Fill(eta, tmp.interaction);
            rlazyall.Fill(eta, tmp.radiation);
            ilazyall.Fill(eta, tmp.interaction);
            rglobal.Fill(eta, tmp.radiation);
            iglobal.Fill(eta, tmp.interaction);
            //      supports, barrel tubes
            tmp = analyzeInactiveSurfaces(mb.getInactiveSurfaces().getSupports(), eta, theta, track, MaterialProperties::t_sup);
            rlazybtube.Fill(eta, tmp.radiation);
            ilazybtube.Fill(eta, tmp.interaction);
            rlazyall.Fill(eta, tmp.radiation);
            ilazyall.Fill(eta, tmp.interaction);
            rglobal.Fill(eta, tmp.radiation);
            iglobal.Fill(eta, tmp.interaction);
            //      supports, user defined
            tmp = analyzeInactiveSurfaces(mb.getInactiveSurfaces().getSupports(), eta, theta, track, MaterialProperties::u_sup);
            rlazyuserdef.Fill(eta, tmp.radiation);
            ilazyuserdef.Fill(eta, tmp.interaction);
            rlazyall.Fill(eta, tmp.radiation);
            ilazyall.Fill(eta, tmp.interaction);
            rglobal.Fill(eta, tmp.radiation);
            iglobal.Fill(eta, tmp.interaction);
            //      pixels, if they exist
            if (pm != NULL) {
                analyzeModules(pm->getBarrelModuleCaps(), eta, theta, phi, track, true);
                analyzeModules(pm->getEndcapModuleCaps(), eta, theta, phi, track, true);
                analyzeInactiveSurfaces(pm->getInactiveSurfaces().getBarrelServices(), eta, theta, track, MaterialProperties::no_cat, true);
                analyzeInactiveSurfaces(pm->getInactiveSurfaces().getEndcapServices(), eta, theta, track, MaterialProperties::no_cat, true);
                analyzeInactiveSurfaces(pm->getInactiveSurfaces().getSupports(), eta, theta, track, MaterialProperties::no_cat, true);
            }
            // Add the hit on the beam pipe
 	    Hit* hit = new Hit(23./sin(theta));
 	    hit->setOrientation(Hit::Horizontal);
 	    hit->setObjectKind(Hit::Inactive);
 	    Material beamPipeMat;
 	    beamPipeMat.radiation = 0.0023;
 	    beamPipeMat.interaction = 0.0019;
 	    hit->setCorrectedMaterial(beamPipeMat);
 	    track.addHit(hit);
            if (!track.noHits()) {
                track.sort();
                track.computeErrors(momenta);
                tv.push_back(track);
		Track trackIdeal = track;
		trackIdeal.removeMaterial();
		trackIdeal.computeErrors(momenta);
		tvIdeal.push_back(trackIdeal);
            }
        }
#ifdef DEBUG_PERFORMANCE
        std::cerr << "DEBUG_PERFORMANCE: material summary by analyzeMaterialBudget(): ";
        t = time(NULL);
        localt = localtime(&t);
        clock_t endtime = clock();
        std::cerr << "elapsed time: " << diffclock(endtime, starttime)/1000. << "s" << std::endl;
        t = time(NULL);
        localt = localtime(&t);
#endif
#ifdef MATERIAL_SHADOW       
        // integration over eta
        for (unsigned int i = 0; i < cells.size(); i++) {
            for (unsigned int j = 1; j < cells.at(i).size(); j++) {
                cells.at(i).at(j).rlength = cells.at(i).at(j).rlength + cells.at(i).at(j - 1).rlength;
                cells.at(i).at(j).ilength = cells.at(i).at(j).ilength + cells.at(i).at(j - 1).ilength;
            }
        }
        // transformation from (eta, r) to (z, r) coordinates
        transformEtaToZ();
#endif // MATERIAL_SHADOW

        // fill TGraph map
#ifdef DEBUG_PERFORMANCE
        t = time(NULL);
        localt = localtime(&t);
        //std::cerr << asctime(localt) << std::endl;
	starttime = clock();
#endif
        calculateProfiles(momenta, tv, rhoprofiles, phiprofiles, dprofiles, ctgThetaProfiles, z0Profiles);
        calculateProfiles(momenta, tvIdeal, rhoprofilesIdeal, phiprofilesIdeal, dprofilesIdeal, ctgThetaProfilesIdeal, z0ProfilesIdeal);
#ifdef DEBUG_PERFORMANCE
        std::cerr << "DEBUG_PERFORMANCE: tracking performance summary by analyzeMaterialBudget(): ";
        t = time(NULL);
        localt = localtime(&t);
        endtime = clock();
        std::cerr << "elapsed time: " << diffclock(endtime, starttime)/1000. << "s" << std::endl;
        t = time(NULL);
        localt = localtime(&t);
#endif
    }
    
    // protected
    /**
     * The layer-level analysis function for modules forms the frame for sending a single track through the active modules.
     * It loops through all layers and adds up the results returned from the analysis of each of them.
     * @param tr A reference to the <i>ModuleCap</i> vector of vectors that sits on top of the tracker modules
     * @param eta The pseudorapidity of the track
     * @param theta The track angle in the yz-plane
     * @param phi The track angle in the xy-plane
     * @param t A reference to the current track object
     * @param A boolean flag to indicate which set of active surfaces is analysed: true if the belong to a pixel detector, false if they belong to the tracker
     * @return The summed up radiation and interaction lengths for the given track, bundled into a <i>std::pair</i>
     */
    Material Analyzer::analyzeModules(std::vector<std::vector<ModuleCap> >& tr,
            double eta, double theta, double phi, Track& t, bool isPixel) {
        std::vector<std::vector<ModuleCap> >::iterator iter = tr.begin();
        std::vector<std::vector<ModuleCap> >::iterator guard = tr.end();
        Material res, tmp;
        res.radiation= 0.0;
        res.interaction = 0.0;
        while (iter != guard) {
            tmp = findModuleLayerRI(*iter, eta, theta, phi, t, isPixel);
            res.radiation= res.radiation+ tmp.radiation;
            res.interaction= res.interaction + tmp.interaction;
            iter++;
        }
        return res;
    }
    
    /**
     * The module-level analysis function loops through all modules of a given layer, checking for collisions with the given track.
     * If one is found, the radiation and interaction lengths are scaled with respect to theta, then summed up into a grand total,
     * which is returned. As phi is fixed at the moment and the tracks hit the modules orthogonally with respect to it, it is so far
     * not used to scale the results further.
     * @param layer A reference to the <i>ModuleCap</i> vector linking the collection of material properties to the current layer
     * @param eta The pseudorapidity of the current track
     * @param theta The track angle in the yz-plane
     * @param phi The track angle in the xy-plane
     * @param t A reference to the current track object
     * @param A boolean flag to indicate which set of active surfaces is analysed: true if the belong to a pixel detector, false if they belong to the tracker
     * @return The scaled and summed up radiation and interaction lengths for the given layer and track, bundled into a <i>std::pair</i>
     */
    Material Analyzer::findModuleLayerRI(std::vector<ModuleCap>& layer,
            double eta, double theta, double phi, Track& t, bool isPixel) {
        std::vector<ModuleCap>::iterator iter = layer.begin();
        std::vector<ModuleCap>::iterator guard = layer.end();
        Material res, tmp;
        XYZVector origin, direction;
        Polar3DVector dir;
        double distance, r;
        int hits = 0;
        res.radiation = 0.0;
        res.interaction = 0.0;
        // set the track direction vector
        dir.SetCoordinates(1, theta, phi);
        direction = dir;
        while (iter != guard) {
            // collision detection: rays are in z+ only, so consider only modules that lie on that side
            // only consider modules that have type BarrelModule or EndcapModule
            if (iter->getModule().getMaxZ() > 0) {
                if ((iter->getModule().getSubdetectorType() == Module::Barrel) ||
                        (iter->getModule().getSubdetectorType() == Module::Endcap)) {
                    // same method as in Tracker, same function used
                    distance = iter->getModule().trackCross(origin, direction);
                    if (distance > 0) {
                        // module was hit
                        hits++;
                        r = distance * sin(theta);
                        tmp.radiation = iter->getRadiationLength();
                        tmp.interaction = iter->getInteractionLength();
			// 2D material maps
			fillMapRT(r, theta, tmp);
                        // radiation and interaction length scaling for barrels
                        if (iter->getModule().getSubdetectorType() == Module::Barrel) {
                            tmp.radiation = tmp.radiation / sin(theta);
                            tmp.interaction = tmp.interaction / sin(theta);
                        }
                        // radiation and interaction length scaling for endcaps
                        else {
                            tmp.radiation = tmp.radiation / cos(theta);
                            tmp.interaction = tmp.interaction / cos(theta);
                        }
                        // 2D plot and eta plot results
                        if (!isPixel) fillCell(r, eta, theta, tmp);
                        res += tmp;
                        // create Hit object with appropriate parameters, add to Track t
                        Hit* hit = new Hit(distance, &(iter->getModule()));
                        //if (iter->getModule().getSubdetectorType() == Module::Barrel) hit->setOrientation(Hit::Horizontal); // should not be necessary
                        //else if(iter->getModule().getSubdetectorType() == Module::Endcap) hit->setOrientation(Hit::Vertical); // should not be necessary
                        //hit->setObjectKind(Hit::Active); // should not be necessary
                        hit->setCorrectedMaterial(tmp);
                        t.addHit(hit);
                    }
                }
                else std::cout << msg_module_warning << std::endl;
            }
            iter++;
        }
        return res;
    }
    
    /**
     * The analysis function for inactive volumes loops through the given vector of elements, checking for collisions with
     * the given track. If one is found, the radiation and interaction lengths are scaled with respect to theta, then summed
     * up into a grand total, which is returned. As all inactive volumes are symmetric with respect to rotation around the
     * z-axis, the track angle phi is not necessary.
     * @param elements A reference to the collection of inactive surfaces that is to be checked for collisions with the track
     * @param eta The pseudorapidity of the current track
     * @param theta The track angle in the yz-plane
     * @param t A reference to the current track object
     * @param cat The category of inactive surfaces that need to be considered within the collection; none if the function is to look at all of them
     * @param A boolean flag to indicate which set of active surfaces is analysed: true if the belong to a pixel detector, false if they belong to the tracker
     * @return The scaled and summed up radiation and interaction lengths for the given collection of elements and track, bundled into a <i>std::pair</i>
     */
    Material Analyzer::analyzeInactiveSurfaces(std::vector<InactiveElement>& elements, double eta,
            double theta, Track& t, MaterialProperties::Category cat, bool isPixel) {
        std::vector<InactiveElement>::iterator iter = elements.begin();
        std::vector<InactiveElement>::iterator guard = elements.end();
        Material res, corr;
        std::pair<double, double> tmp;
        double s = 0.0;
        while (iter != guard) {
            // collision detection: rays are in z+ only, so only volumes in z+ need to be considered
            // only volumes of the requested category, or those without one (which should not exist) are examined
            if (((iter->getZOffset() + iter->getZLength()) > 0)
                    && ((cat == MaterialProperties::no_cat) || (cat == iter->getCategory()))) {
                // collision detection: check eta range
                tmp = iter->getEtaMinMax();
                // volume was hit
                if ((tmp.first < eta) && (tmp.second > eta)) {
                    double r, z;
                    // radiation and interaction lenth scaling for vertical volumes
                    if (iter->isVertical()) {
                        z = iter->getZOffset() + iter->getZLength() / 2.0;
                        r = z * tan(theta);
			// 2D maps for vertical surfaces
			fillMapRZ(r,z,iter->getMaterialLengths());
                        // special treatment for user-defined supports as they can be very close to z=0
                        if (cat == MaterialProperties::u_sup) {
                            s = iter->getZLength() / cos(theta);
                            if (s > (iter->getRWidth() / sin(theta))) s = iter->getRWidth() / sin(theta);
                            // add the hit if it's declared as inside the tracking volume, add it to 'others' if not
                            if (iter->track()) {
                                corr.radiation = iter->getRadiationLength() * s / iter->getZLength();
                                corr.interaction = iter->getInteractionLength() * s / iter->getZLength();
                                res += corr;
                                if (!isPixel) {
                                    Material thisLength;
                                    thisLength.radiation = iter->getRadiationLength() * s / iter->getZLength();
                                    thisLength.interaction = iter->getInteractionLength() * s / iter->getZLength(); 
                                    fillCell(r, eta, theta, thisLength); 
                                }
                            }
                            else {
                                if (!isPixel) {
                                    rextrasupports.Fill(eta, iter->getRadiationLength() * s / iter->getZLength());
                                    iextrasupports.Fill(eta, iter->getInteractionLength() * s / iter->getZLength());
                                }
                            }
                        }
                        else {
                            // add the hit if it's declared as inside the tracking volume, add it to 'others' if not
                            if (iter->track()) {
                                corr.radiation = iter->getRadiationLength() / cos(theta);
                                corr.interaction = iter->getInteractionLength() / cos(theta);
                                res += corr;
                                if (!isPixel) {
                                    Material thisLength;
                                    thisLength.radiation = iter->getRadiationLength() / cos(theta); 
                                    thisLength.interaction = iter->getInteractionLength() / cos(theta);
                                    fillCell(r, eta, theta, thisLength);
                                }
                            }
                            else {
                                if (!isPixel) {
                                    if ((iter->getCategory() == MaterialProperties::b_ser)
                                            || (iter->getCategory() == MaterialProperties::e_ser)) {
                                        rextraservices.Fill(eta, iter->getRadiationLength() / cos(theta));
                                        iextraservices.Fill(eta, iter->getInteractionLength() / cos(theta));
                                    }
                                    else if ((iter->getCategory() == MaterialProperties::b_sup)
                                            || (iter->getCategory() == MaterialProperties::e_sup)
                                            || (iter->getCategory() == MaterialProperties::o_sup)
                                            || (iter->getCategory() == MaterialProperties::t_sup)) {
                                        rextrasupports.Fill(eta, iter->getRadiationLength() / cos(theta));
                                        iextrasupports.Fill(eta, iter->getInteractionLength() / cos(theta));
                                    }
                                }
                            }
                        }
                    }
                    // radiation and interaction length scaling for horizontal volumes
                    else {
                        r = iter->getInnerRadius() + iter->getRWidth() / 2.0;
			// 2D maps for horizontal surfaces
			fillMapRT(r,theta,iter->getMaterialLengths());
                        // special treatment for user-defined supports; should not be necessary for now
                        // as all user-defined supports are vertical, but just in case...
                        if (cat == MaterialProperties::u_sup) {
                            s = iter->getZLength() / sin(theta);
                            if (s > (iter->getRWidth() / cos(theta))) s = iter->getRWidth() / cos(theta);
                            // add the hit if it's declared as inside the tracking volume, add it to 'others' if not
                            if (iter->track()) {
                                corr.radiation = iter->getRadiationLength() * s / iter->getZLength();
                                corr.interaction = iter->getInteractionLength() * s / iter->getZLength();
                                res += corr;
                                if (!isPixel) {
                                    Material thisLength;
                                    thisLength.radiation = iter->getRadiationLength() * s / iter->getZLength(); 
                                    thisLength.interaction = iter->getInteractionLength() * s / iter->getZLength();
                                    fillCell(r, eta, theta, thisLength);
                                }
                            }
                            else {
                                if (!isPixel) {
                                    rextrasupports.Fill(eta, iter->getRadiationLength() * s / iter->getZLength());
                                    iextrasupports.Fill(eta, iter->getInteractionLength() * s / iter->getZLength());
                                }
                            }
                        }
                        else {
                            // add the hit if it's declared as inside the tracking volume, add it to 'others' if not
                            if (iter->track()) {
                                corr.radiation = iter->getRadiationLength() / sin(theta);
                                corr.interaction = iter->getInteractionLength() / sin(theta);
                                res += corr;
                                if (!isPixel) {
                                    Material thisLength;
                                    thisLength.radiation = iter->getRadiationLength() / sin(theta);
                                    thisLength.interaction =  iter->getInteractionLength() / sin(theta);
                                    fillCell(r, eta, theta, thisLength); 
                                }
                            }
                            else {
                                if (!isPixel) {
                                    if ((iter->getCategory() == MaterialProperties::b_ser)
                                            || (iter->getCategory() == MaterialProperties::e_ser)) {
                                        rextraservices.Fill(eta, iter->getRadiationLength() / sin(theta));
                                        iextraservices.Fill(eta, iter->getInteractionLength() / sin(theta));
                                    }
                                    else if ((iter->getCategory() == MaterialProperties::b_sup)
                                            || (iter->getCategory() == MaterialProperties::e_sup)
                                            || (iter->getCategory() == MaterialProperties::o_sup)
                                            || (iter->getCategory() == MaterialProperties::t_sup)) {
                                        rextrasupports.Fill(eta, iter->getRadiationLength() / sin(theta));
                                        iextrasupports.Fill(eta, iter->getInteractionLength() / sin(theta));
                                    }
                                }
                            }
                        }
                    }
                    // create Hit object with appropriate parameters, add to Track t
                    Hit* hit = new Hit((theta == 0) ? r : (r / sin(theta)));
                    if (iter->isVertical()) hit->setOrientation(Hit::Vertical);
                    else hit->setOrientation(Hit::Horizontal);
                    hit->setObjectKind(Hit::Inactive);
                    hit->setCorrectedMaterial(corr);
                    t.addHit(hit);
                }
            }
            iter++;
        }
        return res;
    }
    
    /**
     * Calculate the error profiles for the radius curvature, the distance and the angle, for each momentum,
     * and store them internally for later visualisation.
     * @param p The list of different momenta that the error profiles are calculated for
     */
  void Analyzer::calculateProfiles(std::vector<double>& p, 
                                  std::vector<Track>& trackVector,
                                  std::map<double, TGraph>& thisRhoProfiles,
                                  std::map<double, TGraph>& thisPhiProfiles,
                                  std::map<double, TGraph>& thisDProfiles,
                                  std::map<double, TGraph>& thisCtgThetaProfiles,
                                  std::map<double, TGraph>& thisZ0Profiles) {
        std::map<double, double>::const_iterator miter, mguard;
        std::vector<double>::const_iterator iter, guard = p.end();
        int n = trackVector.size();
        double eta, R;
        thisRhoProfiles.clear();
        thisPhiProfiles.clear();
        thisDProfiles.clear();
	thisCtgThetaProfiles.clear();
	thisZ0Profiles.clear();
        // momentum loop
        std::ostringstream aName;
        for (iter = p.begin(); iter != guard; iter++) {
            std::pair<double, TGraph> elem;
            TGraph graph;
            elem.first = *iter;
            elem.second = graph;
	    // Prepare plots: pT
            thisRhoProfiles.insert(elem);
	    thisRhoProfiles[elem.first].SetTitle("Transverse momentum error;#eta;#sigma (#delta p_{T}/p_{T}) [%]");
            aName.str(""); aName << "pt_vs_eta" << *iter;
            thisRhoProfiles[elem.first].SetName(aName.str().c_str());
	    // Prepare plots: phi
            thisPhiProfiles.insert(elem);
	    thisPhiProfiles[elem.first].SetTitle("Track azimuthal angle error;#eta;#sigma (#delta #phi) [rad]");
            aName.str(""); aName << "phi_vs_eta" << *iter;
            thisPhiProfiles[elem.first].SetName(aName.str().c_str());
	    // Prepare plots: d
            thisDProfiles.insert(elem);
	    thisDProfiles[elem.first].SetTitle("Transverse impact parameter error;#eta;#sigma (#delta d_{0}) [cm]");
            aName.str(""); aName << "d_vs_eta" << *iter;
            thisDProfiles[elem.first].SetName(aName.str().c_str());
	    // Prepare plots: ctg(theta)
	    thisCtgThetaProfiles.insert(elem);
	    thisCtgThetaProfiles[elem.first].SetTitle("Track polar angle error;eta;#sigma (#delta ctg(#theta))");
            aName.str(""); aName << "ctgTheta_vs_eta" << *iter;
            thisCtgThetaProfiles[elem.first].SetName(aName.str().c_str());
	    // Prepare plots: z0
	    thisZ0Profiles.insert(elem);
	    thisZ0Profiles[elem.first].SetTitle("Longitudinal impact parameter error;#eta;#sigma (#delta z_{0}) [cm]");
            aName.str(""); aName << "z_vs_eta" << *iter;
            thisZ0Profiles[elem.first].SetName(aName.str().c_str());
        }
        // track loop
	std::map<double,int> rhoPointCount;
	std::map<double,int> phiPointCount;
	std::map<double,int> dPointCount;
	std::map<double,int> ctgPointCount;
	std::map<double,int> z0PointCount;
        double graphValue;
        for (int i = 0; i < n; i++) {
            std::map<double, double>& drho = trackVector.at(i).getDeltaRho();
            std::map<double, double>& dphi = trackVector.at(i).getDeltaPhi();
            std::map<double, double>& dd = trackVector.at(i).getDeltaD();
            std::map<double, double>& dctg = trackVector.at(i).getDeltaCtgTheta();
            std::map<double, double>& dz0 = trackVector.at(i).getDeltaZ0();
            eta = - log(tan(trackVector.at(i).getTheta() / 2));
            mguard = drho.end();
            // error by momentum loop
            for (miter = drho.begin(); miter != mguard; miter++) {
                if (thisRhoProfiles.find(miter->first) != thisRhoProfiles.end()) {
                  R = miter->first / magnetic_field / 0.3 * 1E3; // radius in mm
		  if ((miter->second)>0) {
		    // deltaRho / rho = deltaRho * R
		    graphValue = (miter->second * R) * 100; // in percent
		    thisRhoProfiles[miter->first].SetPoint(rhoPointCount[miter->first]++, eta, graphValue);
		  }
                }
            }
            mguard = dphi.end();
            for (miter = dphi.begin(); miter != mguard; miter++) {
	      if (thisPhiProfiles.find(miter->first) != thisPhiProfiles.end())
		if ((miter->second)>0) {
		  graphValue = miter->second; // radians is ok
		  thisPhiProfiles[miter->first].SetPoint(phiPointCount[miter->first]++, eta, graphValue);
		}
            }
            mguard = dd.end();
            for (miter = dd.begin(); miter != mguard; miter++) {
	      if (thisDProfiles.find(miter->first) != thisDProfiles.end())
		if ((miter->second)>0) {
		  graphValue =  (miter->second) / 10.; // in cm
		  thisDProfiles[miter->first].SetPoint(dPointCount[miter->first]++, eta, graphValue );
		}
            }
            mguard = dctg.end();
            for (miter = dctg.begin(); miter != mguard; miter++) {
	      // Ctg theta (absolute number)
              if (thisCtgThetaProfiles.find(miter->first) != thisCtgThetaProfiles.end()) {
		graphValue = miter->second; // An absolute number
		thisCtgThetaProfiles[miter->first].SetPoint(ctgPointCount[miter->first]++, eta, graphValue);
	      }
            }
            mguard = dz0.end();
            for (miter = dz0.begin(); miter != mguard; miter++) {
              if (thisZ0Profiles.find(miter->first) != thisZ0Profiles.end()) {
		graphValue =  (miter->second) / 10.; // in cm
		thisZ0Profiles[miter->first].SetPoint(z0PointCount[miter->first]++, eta, graphValue);
	      }
            }	    
        }
    }
    
    /**
     * This convenience function resets and empties all histograms for the
     * material budget, so they are ready for a new round of analysis.
     */
    void Analyzer::clearMaterialBudgetHistograms() {
        // single category
        ractivebarrel.Reset();
        ractivebarrel.SetNameTitle("ractivebarrels", "Barrel Modules Radiation Length");
        ractiveendcap.Reset();
        ractiveendcap.SetNameTitle("ractiveendcap", "Endcap Modules Radiation Length");
        rserfbarrel.Reset();
        rserfbarrel.SetNameTitle("rserfbarrel", "Barrel Services Radiation Length");
        rserfendcap.Reset();
        rserfendcap.SetNameTitle("rserfendcap", "Endcap Services Radiation Length");
        rlazybarrel.Reset();
        rlazybarrel.SetNameTitle("rlazybarrel", "Barrel Supports Radiation Length");
        rlazyendcap.Reset();
        rlazyendcap.SetNameTitle("rlazyendcap", "Endcap Supports Radiation Length");
        rlazytube.Reset();
        rlazytube.SetNameTitle("rlazytube", "Support Tubes Radiation Length");
        rlazyuserdef.Reset();
        rlazyuserdef.SetNameTitle("rlazyuserdef", "Userdefined Supports Radiation Length");
        iactivebarrel.Reset();
        iactivebarrel.SetNameTitle("iactivebarrel", "Barrel Modules Interaction Length");
        iactiveendcap.Reset();
        iactiveendcap.SetNameTitle("iactiveendcap", "Endcap Modules Interaction Length");
        iserfbarrel.Reset();
        iserfbarrel.SetNameTitle("iserfbarrel", "Barrel Services Interaction Length");
        iserfendcap.Reset();
        iserfendcap.SetNameTitle("iserfendcap", "Endcap Services Interaction Length");
        ilazybarrel.Reset();
        ilazybarrel.SetNameTitle("ilazybarrel", "Barrel Supports Interaction Length");
        ilazyendcap.Reset();
        ilazyendcap.SetNameTitle("ilazyendcap", "Endcap Supports Interaction Length");
        ilazytube.Reset();
        ilazytube.SetNameTitle("ilazytube", "Support Tubes Interaction Length");
        ilazyuserdef.Reset();
        ilazyuserdef.SetNameTitle("ilazyuserdef", "Userdefined Supports Interaction Length");
        // composite
        rbarrelall.Reset();
        rbarrelall.SetNameTitle("rbarrelall", "Barrel Radiation Length");
        rendcapall.Reset();
        rendcapall.SetNameTitle("rendcapall", "Endcap Radiation Length");
        ractiveall.Reset();
        ractiveall.SetNameTitle("ractiveall", "Modules Radiation Length");
        rserfall.Reset();
        rserfall.SetNameTitle("rserfall", "Services Radiation Length");
        rlazyall.Reset();
        rlazyall.SetNameTitle("rlazyall", "Supports Radiation Length");
        ibarrelall.Reset();
        ibarrelall.SetNameTitle("ibarrelall", "Barrel Interaction Length");
        iendcapall.Reset();
        iendcapall.SetNameTitle("iendcapall", "Endcap Interaction Length");
        iactiveall.Reset();
        iactiveall.SetNameTitle("iactiveall", "Modules Interaction Length");
        iserfall.Reset();
        iserfall.SetNameTitle("iserfall", "Services Interaction Length");
        ilazyall.Reset();
        ilazyall.SetNameTitle("ilazyall", "Supports Interaction Length");
        // outside tracking volume
        rextraservices.Reset();
        rextraservices.SetNameTitle("rextraservices", "Services Outside Tracking Volume: Radiation Length");
        rextrasupports.Reset();
        rextrasupports.SetNameTitle("rextrasupports", "Supports Outside Tracking Volume: Radiation Length");
        iextraservices.Reset();
        iextraservices.SetNameTitle("iextraservices", "Services Outside Tracking Volume: Interaction Length");
        iextrasupports.Reset();
        iextrasupports.SetNameTitle("iextrasupports", "Supports Outside Tracking Volume: Interaction Length");
        // global
        rglobal.Reset();
        rglobal.SetNameTitle("rglobal", "Overall Radiation Length");
        iglobal.Reset();
        iglobal.SetNameTitle("iglobal", "Overall Interaction Length");
        // isolines
        isor.Reset();
        isor.SetNameTitle("isor", "Radiation Length Contours");
        isoi.Reset();
        isoi.SetNameTitle("isoi", "Interaction Length Contours");
	mapRadiation.Reset();
	mapRadiation.SetName("mapRadiation");
	mapRadiation.SetTitle("Radiation length map (raw);z [mm];r [mm]");
	mapInteraction.Reset();
	mapInteraction.SetName("mapInteraction");
	mapInteraction.SetTitle("Interaction length map (raw);z [mm];r [mm]");
	mapRadiationCount.Reset();
	mapRadiationCount.SetName("mapRadiationCount");
	mapRadiationCount.SetTitle("Radiation length hit count map;z [mm];r [mm]");
	mapInteractionCount.Reset();
	mapInteractionCount.SetName("mapInteractionCount");
	mapInteractionCount.SetTitle("Interaction length hit count map;z [mm];r [mm]");
	mapRadiationCalib.Reset();
	mapRadiationCalib.SetName("mapRadiationCalib");
	mapRadiationCalib.SetTitle("Radiation length map;z [mm];r [mm]");
	mapInteractionCalib.Reset();
	mapInteractionCalib.SetName("mapInteractionCalib");
	mapInteractionCalib.SetTitle("Interaction length map;z [mm];r [mm]");
    }
    
    /**
     * This convenience function resets and empties all histograms for the
     * geometry so they are ready for a new round of analysis.
     */
    void Analyzer::clearGeometryHistograms() {
        // geometry analysis
        mapPhiEta.Reset();
        mapPhiEta.SetNameTitle("mapPhiEta", "Number of hits;#phi;#eta");
        etaProfileCanvas.SetName("etaProfileCanvas"); etaProfileCanvas.SetTitle("Eta Profiles");
        hitDistribution.Reset();
        hitDistribution.SetNameTitle("hitDistribution", "Hit distribution");
        //geomLite->SetName("geometryLite");   geomLite->SetTitle("Modules geometry");
        //geomLiteXY->SetName("geometryLiteXY"); geomLiteXY->SetTitle("Modules geometry (XY Section)");
        //geomLiteYZ->SetName("geometryLiteYZ"); geomLiteYZ->SetTitle("Modules geometry (EC Section)");
        //geomLiteEC->SetName("geometryLiteEC"); geomLiteEC->SetTitle("Modules geometry (Endcap)");
    }
    
    /**
     * This convenience function sets all values in the internal array <i>cells</i> to zero.
     */
    void Analyzer::clearCells() {
        for (unsigned int i = 0; i < cells.size(); i++) {
            cells.at(i).clear();
        }
        cells.clear();
    }
    
    /**
     * This convenience function sets the number of bins and the lower and upper range for their contents for
     * each of the available histograms.
     * @param bins The number of bins in each 1D histogram
     * @param min The minimal eta value that should be plotted
     * @param max the maximal eta value that should be plotted
     */
    void Analyzer::setHistogramBinsBoundaries(int bins, double min, double max) {
        // single category
        ractivebarrel.SetBins(bins, min, max);
        ractiveendcap.SetBins(bins, min, max);
        rserfbarrel.SetBins(bins, min, max);
        rserfendcap.SetBins(bins, min, max);
        rlazybarrel.SetBins(bins, min, max);
        rlazyendcap.SetBins(bins, min, max);
        rlazytube.SetBins(bins, min, max);
        rlazyuserdef.SetBins(bins, min, max);
        iactivebarrel.SetBins(bins, min, max);
        iactiveendcap.SetBins(bins, min, max);
        iserfbarrel.SetBins(bins, min, max);
        iserfendcap.SetBins(bins, min, max);
        ilazybarrel.SetBins(bins, min, max);
        ilazyendcap.SetBins(bins, min, max);
        ilazytube.SetBins(bins, min, max);
        ilazyuserdef.SetBins(bins, min, max);
        // composite
        rbarrelall.SetBins(bins, min, max);
        rendcapall.SetBins(bins, min, max);
        ractiveall.SetBins(bins, min, max);
        rserfall.SetBins(bins, min, max);
        rlazyall.SetBins(bins, min, max);
        ibarrelall.SetBins(bins, min, max);
        iendcapall.SetBins(bins, min, max);
        iactiveall.SetBins(bins, min, max);
        iserfall.SetBins(bins, min, max);
        ilazyall.SetBins(bins, min, max);
        // outside tracking volume
        rextraservices.SetBins(bins, min, max);
        rextrasupports.SetBins(bins, min, max);
        iextraservices.SetBins(bins, min, max);
        iextrasupports.SetBins(bins, min, max);
        // global
        rglobal.SetBins(bins, min, max);
        iglobal.SetBins(bins, min, max);
        // isolines
        isor.SetBins(bins, 0.0, max_length, bins / 2, 0.0, outer_radius + volume_width);
        isoi.SetBins(bins, 0.0, max_length, bins / 2, 0.0, outer_radius + volume_width);
	// Material distribution maps
	int materialMapBinsY = int( (outer_radius + volume_width) * 1.1 / 5.); // every half a cm
	int materialMapBinsX = int( (max_length) * 1.1 / 5.); // every half a cm
	mapRadiation.SetBins(materialMapBinsX, 0.0, max_length*1.1, materialMapBinsY, 0.0, (outer_radius + volume_width) * 1.1);
	mapInteraction.SetBins(materialMapBinsX, 0.0, max_length*1.1, materialMapBinsY, 0.0, (outer_radius + volume_width) * 1.1);
	mapRadiationCount.SetBins(materialMapBinsX, 0.0, max_length*1.1, materialMapBinsY, 0.0, (outer_radius + volume_width) * 1.1);
	mapInteractionCount.SetBins(materialMapBinsX, 0.0, max_length*1.1, materialMapBinsY, 0.0, (outer_radius + volume_width) * 1.1);
	mapRadiationCalib.SetBins(materialMapBinsX, 0.0, max_length*1.1, materialMapBinsY, 0.0, (outer_radius + volume_width) * 1.1);
	mapInteractionCalib.SetBins(materialMapBinsX, 0.0, max_length*1.1, materialMapBinsY, 0.0, (outer_radius + volume_width) * 1.1);
    }
    
    /**
     * This convenience function sets the number of bins and the lower and upper range for their contents for
     * each of the cells that make up the basis for the 2D histograms.
     * @param bins The number of bins in eta; the number of bins in r will be half that
     * @param minr The minimum radius that will be considered for tracking
     * @param maxr The maximum radius that will be considered for tracking
     * @param mineta The minimum value of eta that will be considered for tracking
     * @param maxeta The maximum value of eta that will be considered for tracking
     */
    void Analyzer::setCellBoundaries(int bins, double minr, double maxr, double mineta, double maxeta) {
        double rstep, etastep;
        rstep = 2 * (maxr - minr) / bins;
        etastep = (maxeta - mineta) / bins;
        Cell c;
        c.rlength = 0.0;
        c.ilength = 0.0;
        cells.resize(bins);
        for (unsigned int i = 0; i < cells.size(); i++) if (bins != 1) cells.at(i).resize(bins / 2, c);
        for (unsigned int i = 0; i < cells.size(); i++) {
            for (unsigned int j = 0; j < cells.at(i).size(); j++) {
                cells.at(i).at(j).rmin = minr + j * rstep;
                cells.at(i).at(j).rmax = minr + (j+1) * rstep;
                cells.at(i).at(j).etamin = mineta + i * etastep;;
                cells.at(i).at(j).etamax = mineta + (i+1) * etastep;
            }
        }
    }


  /**
   * Fills the material distribution maps
   * @param r The radius at which the hit was detected
   * @param theta The angle of the track used for meterial detection
   * @param rl The local radiation length
   * @param il The local interaction length
   */
  void Analyzer::fillMapRT(const double& r, const double& theta, const Material& mat) {
    double z = r /tan(theta);
    if (mat.radiation>0){
      mapRadiation.Fill(z,r,mat.radiation);
      mapRadiationCount.Fill(z,r);
    } 
    if (mat.interaction>0) {
      mapInteraction.Fill(z,r,mat.interaction);
      mapInteractionCount.Fill(z,r);
    }
  }

  /**
   * Fills the material distribution maps
   * @param r The radius at which the hit was detected
   * @param z The z coordinate of the hit
   * @param rl The local radiation length
   * @param il The local interaction length
   */
  void Analyzer::fillMapRZ(const double& r, const double& z, const Material& mat) {
    if (mat.radiation>0){
      mapRadiation.Fill(z,r,mat.radiation);
      mapRadiationCount.Fill(z,r);
    } 
    if (mat.interaction>0) {
      mapInteraction.Fill(z,r,mat.interaction);
      mapInteractionCount.Fill(z,r);
    }
  }

  /**
   * @return a (hit-scaled) map of radiation length
   */
  TH2D& Analyzer::getHistoMapRadiation() {
    int nBins = mapRadiation.GetNbinsX()*mapRadiation.GetNbinsY();
    double content;
    double count;
    for (int iBin=1; iBin<=nBins; ++iBin) {
      content = mapRadiation.GetBinContent(iBin);
      count = mapRadiationCount.GetBinContent(iBin);
      //mapRadiationCalib.SetBinContent(iBin,content);
      if (count==1) mapRadiationCalib.SetBinContent(iBin,content);
      else if (count>1) mapRadiationCalib.SetBinContent(iBin,content/double(count));
      //else if (count==0) mapRadiationCalib.SetBinContent(iBin, 0.);
    }
    return mapRadiationCalib;
  }

  /**
   * @return a (hit-scaled) map of interaction length
   */
  TH2D& Analyzer::getHistoMapInteraction() {
    int nBins = mapInteraction.GetNbinsX()*mapInteraction.GetNbinsY();
    double content;
    double count;
    for (int iBin=1; iBin<=nBins; ++iBin) {
      content = mapInteraction.GetBinContent(iBin);
      count = mapInteractionCount.GetBinContent(iBin);
      //mapInteractionCalib.SetBinContent(iBin,content);
      if (count==1) mapInteractionCalib.SetBinContent(iBin,content);
      else if (count>1) mapInteractionCalib.SetBinContent(iBin,content/double(count));
      //else if (count==0) mapInteractionCalib.SetBinContent(iBin, 0.);
    }
    return mapInteractionCalib;
  }


    /**
     * This function assigns the local radiation and interaction lengths of a detected hit to their position in the
     * (eta, r) space.
     * @param r The radius at which the hit was detected
     * @param eta The eta value of the current track
     * @param rl The local radiation length
     * @param il The local interaction length
     */
  void Analyzer::fillCell(double r, double eta, double theta, Material mat) {
        double rl = mat.radiation;
        double il = mat.interaction;
        int rindex, etaindex;
        if (cells.size() > 0) {
            for (rindex = 0; (unsigned int) rindex < cells.at(0).size(); rindex++) {
                if ((cells.at(0).at(rindex).rmin <= r) && (cells.at(0).at(rindex).rmax > r)) break;
            }
            if ((unsigned int) rindex < cells.at(0).size()) {
                for (etaindex = 0; (unsigned int) etaindex < cells.size(); etaindex++) {
                    if ((cells.at(etaindex).at(rindex).etamin <= eta) && (cells.at(etaindex).at(rindex).etamax > eta)) break;
                }
                if ((unsigned int) etaindex < cells.size()) {
                    cells.at(etaindex).at(rindex).rlength = cells.at(etaindex).at(rindex).rlength + rl;
                    cells.at(etaindex).at(rindex).ilength = cells.at(etaindex).at(rindex).ilength + il;
                }
            }
        }
    }
    
    /**
     * The integrated radiation and interaction lengths in (eta, r)
     * are converted to (z, r) coordinates and stored in <i>isor</i>
     * and <i>isoi</i> in this function.
     */
    void Analyzer::transformEtaToZ() {
        int size_z, size_r, rindex, etaindex;
        double z, r, eta, z_max, z_min, r_max, r_min, z_c, r_c;
        // init: sizes and boundaries
        size_z = isor.GetNbinsX();
        size_r = isor.GetNbinsY();
        z_max = isor.GetXaxis()->GetXmax();
        z_min = isor.GetXaxis()->GetXmin();
        r_max = isor.GetYaxis()->GetXmax();
        r_min = isor.GetYaxis()->GetXmin();
        // new number of bins in z and r
        z_c = (z_max - z_min) / (2 * size_z);
        r_c = (r_max - r_min) / (2 * size_r);
        // radiation length loop
        for (int i = 1; i <= size_z; i++) {
            // calculate current z bin
            z = z_min + 2 * (i - 1) * z_c + z_c;
            for (int j = 1; j <= size_r; j++) {
                // calculate current r bin
                r = r_min + 2 * (j - 1) * r_c + r_c;
                eta = -log(tan(atan(r / z) / 2.0));
                // find corresponding r and eta positions
                etaindex = findCellIndexEta(eta);
                rindex = findCellIndexR(r);
                // fill in radiation length in r and z
                if ((etaindex >= 0) && (rindex >= 0)) isor.Fill(z, r, cells.at(etaindex).at(rindex).rlength);
            }
        }
        // init: sizes and boundaries
        size_z = isoi.GetNbinsX();
        size_r = isoi.GetNbinsY();
        z_max = isoi.GetXaxis()->GetXmax();
        z_min = isoi.GetXaxis()->GetXmin();
        r_max = isoi.GetYaxis()->GetXmax();
        r_min = isoi.GetYaxis()->GetXmin();
        // new number of bins in z and r
        z_c = (z_max - z_min) / (2 * size_z);
        r_c = (r_max - r_min) / (2 * size_r);
        // interaction length loop
        for (int i = 0; i < size_z; i++) {
            // calculate current z bin
            z = z_min + 2 * i * z_c + z_c;
            for (int j = 0; j < size_r; j++) {
                // calculate current r bin
                r = r_min + 2 * j * r_c + r_c;
                eta = -log(tan(atan(r / z) / 2.0));
                // find corresponding r and eta positions
                etaindex = findCellIndexEta(eta);
                rindex = findCellIndexR(r);
                // fill in interaction length in r and z
                if ((etaindex >= 0) && (rindex >= 0)) isoi.Fill(z, r, cells.at(etaindex).at(rindex).ilength);
            }
        }
    }
    
    // private
    /**
     * This is a convenience function that converts a radius to an index into the second dimension of <i>cells</i>.
     * @param r The given radius value
     * @return The corresponding index into the vector
     */
    int Analyzer::findCellIndexR(double r) {
        int index = -1;
        if (r >= 0) {
            if (cells.size() > 0) {
                index = 0;
                while (index < (int)cells.at(0).size()) {
                    if (cells.at(0).at(index).rmax < r) index++;
                    else break;
                }
                if (index == (int)cells.at(0).size()) index = -1;
                return index;
            }
        }
        return index;
    }
    
    /**
     * This is a convenience function that converts an eta value to an index into the first dimension of <i>cells</i>.
     * @param eta The given eta value
     * @return the corresponding index into the vector
     */
    int Analyzer::findCellIndexEta(double eta) {
        int index = -1;
        if (eta >= 0) {
            index = 0;
            while (index < (int)cells.size()) {
                if (cells.at(index).size() > 0) {
                    if (cells.at(index).at(0).etamax < eta) index++;
                    else break;
                }
                else {
                    index = -1;
                    break;
                }
            }
            if (index == (int)cells.size()) index = -1;
            return index;
        }
        return index;
    }
    
    // public
    /**
     * Creates the histograms to analyze the tracker coverage
     * @param tracker the tracker to be analyzed
     * @param nTracker the number of tracks to be used to analyze the coverage (defaults to 1000)
     */
    void Analyzer::analyzeGeometry(Tracker& tracker, int nTracks /*=1000*/ ) {
        geometryTracksUsed = nTracks;
        savingGeometryV.clear();
        clearGeometryHistograms();
        
        
        // A bunch of pointers
        std::map <std::string, int> moduleTypeCount;
        std::map <std::string, TH2D> etaProfileByType;
        TH2D* aPlot;
        std::string aType;
        
        
        // Optimize the track creation on the real tracker
        std::pair <double, double> etaMinMax = tracker.getEtaMinMax();
        double absMinEta = fabs(etaMinMax.first);
        double absMaxEta = fabs(etaMinMax.second);
        double maxEta = (absMinEta>absMaxEta) ? absMinEta : absMaxEta;
        
        // Computing the margin of the tracks to shoot
        double randomPercentMargin = 0.1;
        double randomSpan = (etaMinMax.second - etaMinMax.first)*(1. + randomPercentMargin);
        double randomBase = etaMinMax.first - (etaMinMax.second - etaMinMax.first)*(randomPercentMargin)/2.;
        
        // Initialize random number generator, counters and histograms
        myDice.SetSeed(MY_RANDOM_SEED);
        createResetCounters(tracker, moduleTypeCount);
        
        /*for (std::map <std::string, TH2D*>::iterator it = etaProfileByType.begin();
         it!=etaProfileByType.end(); it++) {
      aPlot = (*it).second;
      if (aPlot) delete aPlot;
      }*/
        etaProfileByType.clear();
        
        for (std::map <std::string, int>::iterator it = moduleTypeCount.begin();
        it!=moduleTypeCount.end(); it++) {
            // std::cerr << "Creating plot for module type " << (*it).first << std::endl; //debug
            aPlot = new TH2D( (*it).first.c_str(), (*it).first.c_str(),
                    100, 0., maxEta*1.1,
                    1000, 0., 10.);
            etaProfileByType[(*it).first]=(*aPlot);
            delete aPlot;
        }
        
        LayerVector::iterator layIt;
        ModuleVector* moduleV;
        ModuleVector::iterator modIt;
        ModuleVector allModules;
        LayerVector& layerSet = tracker.getLayers();
        double zError = tracker.getZError();
        
        // Build the proper list of modules
        for (layIt=layerSet.begin(); layIt!=layerSet.end(); layIt++) {
            moduleV = (*layIt)->getModuleVector();
            for (modIt=moduleV->begin(); modIt!=moduleV->end(); modIt++) {
                // I pre-compute the boxes to reduce the calculations
                (*modIt)->computeBoundaries(zError);
                allModules.push_back(*modIt);
            }
        }
        
        // The real simulation
        std::pair <XYZVector, double> aLine;
        ModuleVector hitModules;
        
#ifdef DEBUG_PERFORMANCE
        struct tm *localt; // timing: debug
        time_t t;          // timing: debug
        t = time(NULL);
        localt = localtime(&t);
        //std::cerr << asctime(localt) << std::endl;
        clock_t starttime = clock();
#endif
        
        std::cout << "Shooting tracks..." << std::endl;
        int nTrackHits;
        int nTracksPerSide = int(pow(nTracks, 0.5));
        int nBlocks = int(nTracksPerSide/2.);
        nTracks = nTracksPerSide*nTracksPerSide;
        mapPhiEta.SetBins(nBlocks, -1*M_PI, M_PI, nBlocks, -3., 3.);
        TH2I mapPhiEtaCount("mapPhiEtaCount ", "phi Eta hit count", nBlocks, -1*M_PI, M_PI, nBlocks, -3., 3.);
        TH2D total2D("total2d", "Total 2D", 100, 0., maxEta*1.2, 4000 , 0., 40.);
        
        // Shoot nTracksPerSide^2 tracks
        for (int i=0; i<nTracksPerSide; i++) {
            for (int j=0; j<nTracksPerSide; j++) {
                // Reset the hit counter
                nTrackHits=0;
                // Generate a straight track and collect the list of hit modules
                aLine = shootDirection(randomBase, randomSpan);
                hitModules = trackHit( XYZVector(0, 0, myDice.Gaus(0, zError)), aLine.first, &allModules);
                // Reset the per-type hit counter and fill it
                resetTypeCounter(moduleTypeCount);
                for (ModuleVector::iterator it = hitModules.begin(); it!=hitModules.end(); it++) {
                    moduleTypeCount[(*it)->getType()]++;
                    nTrackHits++;
                }
                // Fill the module type hit plot
                for (std::map <std::string, int>::iterator it = moduleTypeCount.begin(); it!=moduleTypeCount.end(); it++) {
                    etaProfileByType[(*it).first].Fill(fabs(aLine.second), (*it).second);
                }
                // Fill other plots
                total2D.Fill(fabs(aLine.second), hitModules.size());                // Total number of hits
                mapPhiEta.Fill(aLine.first.Phi(), aLine.second, hitModules.size()); // phi, eta 2d plot
                mapPhiEtaCount.Fill(aLine.first.Phi(), aLine.second);               // Number of shot tracks
            }
        }
        
        // Create and archive for saving our 2D map of hits
        double hitCount;
        double trackCount;
        for (int nx=0; nx<=mapPhiEtaCount.GetNbinsX()+1; nx++) {
            for (int ny=0; ny<=mapPhiEtaCount.GetNbinsY()+1; ny++) {
                trackCount=mapPhiEtaCount.GetBinContent(nx, ny);
                if (trackCount>0) {
                    hitCount=mapPhiEta.GetBinContent(nx, ny);
                    mapPhiEta.SetBinContent(nx, ny, hitCount/trackCount);
                    //hitCount=mapPhiEta.GetBinContent(nx, ny); //debug
                    //std::cerr << "hitCount " << hitCount << std::endl; //debug
                }
            }
        }
        
        savingGeometryV.push_back(mapPhiEta);
#ifdef DEBUG_PERFORMANCE
        std::cerr << "DEBUG_PERFORMANCE: tracks for analyzeGeometry(): ";
        t = time(NULL);
        localt = localtime(&t);
        clock_t endtime = clock();
        std::cerr << "elapsed time: " << diffclock(endtime, starttime)/1000. << "s" << std::endl;
        t = time(NULL);
        localt = localtime(&t);
#endif
        
        // Eta profile compute
        //TProfile *myProfile;
        
        etaProfileCanvas.cd();
        savingGeometryV.push_back(etaProfileCanvas);
        int plotCount=0;
        
        //TProfile* total = total2D.ProfileX("etaProfileTotal");
        totalEtaProfile = TProfile(*total2D.ProfileX("etaProfileTotal"));
        savingGeometryV.push_back(totalEtaProfile);
        totalEtaProfile.SetMarkerStyle(8);
        totalEtaProfile.SetMarkerColor(1);
        totalEtaProfile.SetMarkerSize(1.5);
        totalEtaProfile.SetTitle("Number of hit modules;#eta;Number of hits");
        if (totalEtaProfile.GetMaximum()<9) totalEtaProfile.SetMaximum(9.);
        totalEtaProfile.Draw();
        std::string profileName;
        TProfile* myProfile;
        for (std::map <std::string, TH2D>::iterator it = etaProfileByType.begin();
        it!=etaProfileByType.end(); it++) {
            plotCount++;
            myProfile=(*it).second.ProfileX();
            savingGeometryV.push_back(*myProfile);
            myProfile->SetMarkerStyle(8);
            myProfile->SetMarkerColor(colorPicker((*it).first));
            myProfile->SetMarkerSize(1);
            profileName = "etaProfile"+(*it).first;
            myProfile->SetName(profileName.c_str());
            myProfile->SetTitle((*it).first.c_str());
            myProfile->GetXaxis()->SetTitle("eta");
            myProfile->GetYaxis()->SetTitle("Number of hits");
            myProfile->Draw("same");
            typeEtaProfile.push_back(*myProfile);
        }
        
        // Record the fraction of hits per module
        hitDistribution.SetBins(nTracks, 0 , 1);
        savingGeometryV.push_back(hitDistribution);
        for (modIt=moduleV->begin(); modIt!=moduleV->end(); modIt++) {
            hitDistribution.Fill((*modIt)->getNHits()/double(nTracks));
        }
        
        return;
    }
    
    // public
    // TODO!!!
    // Creates the geometry objects geomLite
    // (now they are canvases, but in the future
    // they could be just lists of 3d poly objects)
    // Moreover now we create them by calling the tracker object, which seems improper
    void Analyzer::createGeometryLite(Tracker& tracker) {
        if (!(geomLiteCreated &&
                geomLiteXYCreated &&
                geomLiteYZCreated &&
                geomLiteECCreated)) {
            tracker.createGeometry(true);
            geomLiteCreated=true;
            geomLiteXYCreated=true;
            geomLiteYZCreated=true;
            geomLiteECCreated=true;
        }
        geomLite = tracker.getGeomLite();
        geomLiteXY = tracker.getGeomLiteXY();
        geomLiteYZ = tracker.getGeomLiteYZ();
        geomLiteEC = tracker.getGeomLiteEC();
    }
    
    // private
    /**
     * Creates a module type map
     * It sets a different integer for each one
     * @param tracker the tracker to be analyzed
     * @param moduleTypeCount the map to count the different module types
     * @return the total number of module types
     */
    int Analyzer::createResetCounters(Tracker& tracker, std::map <std::string, int> &moduleTypeCount) {
        ModuleVector result;
        LayerVector::iterator layIt;
        ModuleVector* moduleV;
        ModuleVector::iterator modIt;
        
        std::string aType;
        int typeCounter=0;
        
        LayerVector& layerSet = tracker.getLayers();
        for (layIt=layerSet.begin(); layIt!=layerSet.end(); layIt++) {
            moduleV = (*layIt)->getModuleVector();
            for (modIt=moduleV->begin(); modIt!=moduleV->end(); modIt++) {
                aType = (*modIt)->getType();
                (*modIt)->resetNHits();
                if (moduleTypeCount.find(aType)==moduleTypeCount.end()) {
                    moduleTypeCount[aType]=typeCounter++;
                }
            }
        }
        
        return(typeCounter);
    }
    
    // private
    /**
     * Shoots directions with random (flat) phi, random (flat) pseudorapidity
     * gives also the direction's eta
     * @param minEta minimum eta to shoot tracks
     * @param spanEta difference between minimum and maximum eta
     * @return the pair of value: pointing XYZVector and eta of the track
     */
    std::pair <XYZVector, double > Analyzer::shootDirection(double minEta, double spanEta) {
        std::pair <XYZVector, double> result;
        
        double eta;
        double phi;
        double theta;
        
        // phi is random [0, 2pi)
        phi = myDice.Rndm() * 2 * M_PI; // debug
        
        // eta is random (-4, 4]
        eta = myDice.Rndm() * spanEta + minEta;
        theta=2*atan(exp(-1*eta));
        
        // Direction
        result.first  = XYZVector(cos(phi)*sin(theta), sin(phi)*sin(theta), cos(theta));
        result.second = eta;
        return result;
    }
    
    // private
    /**
     * Checks whether a track would hit a module
     * @param origin XYZVector of origin of the track
     * @param direction pointing XYZVector of the track
     * @param moduleV vector of modules to be checked
     * @return the vector of hit modules
     */
    ModuleVector Analyzer::trackHit(const XYZVector& origin, const XYZVector& direction, ModuleVector* moduleV) {
        ModuleVector result;
        ModuleVector::iterator modIt;
        double distance;
        
        for (modIt=moduleV->begin(); modIt!=moduleV->end(); modIt++) {
            // A module can be hit if it fits the phi (precise) contraints
            // and the eta constaints (taken assuming origin within 5 sigma)
            if ((*modIt)->couldHit(direction.Eta(), direction.Phi())) {
                distance=(*modIt)->trackCross(origin, direction);
                if (distance>0) {
                    result.push_back(*modIt);
                }
            }
        }
        return result;
    }
    
    // Resets a module type counter
    void Analyzer::resetTypeCounter(std::map <std::string, int> &modTypes) {
        for (std::map <std::string, int>::iterator it = modTypes.begin();
        it!=modTypes.end(); it++) {
            (*it).second = 0;
        }
    }
    
    double Analyzer::diffclock(clock_t clock1, clock_t clock2) {
        double diffticks=clock1-clock2;
        double diffms=(diffticks*1000)/CLOCKS_PER_SEC;
        return diffms;
    }
    
    // private
    /**
     * Returns the same color for the same module type across
     * all the program
     * @param type string containing the type identifier
     * @return a color
     */
    
    Color_t Analyzer::colorPicker(std::string type) {
        if (type=="") return COLOR_INVALID_MODULE;
        if (colorPickMap[type]==0) {
            // New type! I'll pick a new color
            colorPickMap[type]=++lastPickedColor;
        }
        return colorPickMap[type];
    }
    
    
    std::vector<TObject> Analyzer::getSavingVector() {
        std::vector<TObject> result;
        std::vector<TObject>::iterator it;
        
        for (it=savingGeometryV.begin(); it!=savingGeometryV.end(); ++it) {
            result.push_back(*it);
        }
        for (it=savingMaterialV.begin(); it!=savingMaterialV.end(); ++it) {
            result.push_back(*it);
        }
        return result;
        
    }
    
    
    void Analyzer::computeBandwidth(Tracker& tracker) {
        LayerVector::iterator layIt;
        ModuleVector::iterator modIt;
        ModuleVector* aLay;
        double hitChannels;
        
        // Clear and reset the histograms
        chanHitDistribution.Reset();
        bandwidthDistribution.Reset();
        bandwidthDistributionSparsified.Reset();
        chanHitDistribution.SetNameTitle("NHitChannels", "Number of hit channels;Hit Channels;Modules");
        bandwidthDistribution.SetNameTitle("BandWidthDist", "Module Needed Bandwidth;Bandwidth (bps);Modules");
        bandwidthDistributionSparsified.SetNameTitle("BandWidthDistSp", "Module Needed Bandwidth (sparsified);Bandwidth (bps);Modules");
        chanHitDistribution.SetBins(200, 0., 400);
        bandwidthDistribution.SetBins(100, 0., 6E+8);
        bandwidthDistributionSparsified.SetBins(100, 0., 6E+8);
        bandwidthDistribution.SetLineColor(kBlack);
        bandwidthDistributionSparsified.SetLineColor(kRed);
        
        int nChips;
        LayerVector layerSet = tracker.getLayers();
        double nMB = tracker.getNMB();
        for (layIt=layerSet.begin(); layIt!=layerSet.end(); layIt++) {
            aLay = (*layIt)->getModuleVector();
            for (modIt=aLay->begin(); modIt!=aLay->end(); modIt++) {
                if ((*modIt)->getReadoutType()==Module::Strip) {
                    hitChannels = (*modIt)->getOccupancyPerEvent()*nMB*((*modIt)->getNChannelsPerFace());
                    chanHitDistribution.Fill(hitChannels);
                    
                    for (int nFace=0; nFace<(*modIt)->getNFaces() ; nFace++) {
                        nChips=int(ceil((*modIt)->getNChannelsPerFace()/128.));
                        
                        // TODO: place the computing model choice here
                        
                        // ACHTUNG!!!! whenever you change the numbers here, you have to change
                        // also the numbers in the summary
                        
                        // Binary unsparsified (bps)
                        bandwidthDistribution.Fill((16*nChips+(*modIt)->getNChannelsPerFace())*100E3);
                        // Binary sparsified
                        bandwidthDistributionSparsified.Fill((23*nChips+hitChannels*9)*100E3);
                    }
                }
            }
        }
        
        savingGeometryV.push_back(chanHitDistribution);
        savingGeometryV.push_back(bandwidthDistribution);
        savingGeometryV.push_back(bandwidthDistributionSparsified);
    }
    
}

