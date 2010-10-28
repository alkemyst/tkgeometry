/**
 * @file Vizard.cc
 * @brief This class takes care of visualisation for both geometry and analysis results
 */

#include <Vizard.h>


namespace insur {
    // public
    /**
     * The constructor builds the logical structure within the <i>TGeoManager</i> that is used to display
     * a tracker geometry in ROOT. It assigns different materials and media to the various categories of
     * geometry elements.
     */
    Vizard::Vizard() {
        // internal flag
        geometry_created = false;
        // ROOT geometry manager
        gm = new TGeoManager("display", "Tracker");
        // dummy material definitions for each category
        matvac = new TGeoMaterial("Vacuum", 0, 0, 0);
        matact = new TGeoMaterial("Si", a_silicon, z_silicon, d_silicon);
        matserf = new TGeoMaterial("C ", a_carbon, z_carbon, d_carbon);
        matlazy = new TGeoMaterial("Cu", a_copper, z_copper, d_copper);
        // dummy medium definitions for each category
        medvac = new TGeoMedium("Vacuum", 0, matvac);
        medact = new TGeoMedium("Silicon", 1, matact);
        medserf = new TGeoMedium("Copper", 2, matserf);
        medlazy = new TGeoMedium("Carbon", 3, matlazy);
        // hierarchy definitions to group individual volumes
        barrels = new TGeoVolumeAssembly("Barrels");
        endcaps = new TGeoVolumeAssembly("Endcaps");
        services = new TGeoVolumeAssembly("Services");
        supports = new TGeoVolumeAssembly("Supports");
        active = new TGeoVolumeAssembly("Active Modules");
        inactive = new TGeoVolumeAssembly("Inactive Surfaces");
        // top-level volume definition
        top = gm->MakeBox("WORLD", medvac, outer_radius + top_volume_pad, outer_radius + top_volume_pad, max_length + top_volume_pad);
        // definition of tree hierarchy for visualisation
        active->AddNode(barrels, 0);
        active->AddNode(endcaps, 0);
        inactive->AddNode(services, 0);
        inactive->AddNode(supports, 0);
        top->AddNode(active, 0);
        top->AddNode(inactive, 0);
        // declaration of top volume within ROOT geometry manager
        gm->SetTopVolume(top);
        // Some stylish option
        gStyle->SetOptStat(0);
        const UInt_t numberOfSteps = 5;
        Double_t stops[numberOfSteps] = { 0.00, 0.34, 0.61, 0.84, 1.00 };
        Double_t red[numberOfSteps]   = { 0.00, 0.00, 0.87, 1.00, 0.51 };
        Double_t green[numberOfSteps] = { 0.00, 0.81, 1.00, 0.20, 0.00 };
        Double_t blue[numberOfSteps]  = { 0.51, 1.00, 0.12, 0.00, 0.00 };
        Int_t myPalette[temperature_levels];
        
        Int_t colorIndex = TColor::CreateGradientColorTable(numberOfSteps, stops, red, green, blue, temperature_levels);
        for (int i=0;i<temperature_levels;i++) myPalette[i] = colorIndex+i;
        gStyle->SetPalette(temperature_levels, myPalette);
    }
    
    /**
     * The destructor deletes the instance of <i>TGeoManager</i> that was created in the constructor. Since
     * any object that was added to it in the constructor is now owned by it, this is the only step that is necessary
     * to clean up memory: everything else will be deleted from within the <i>TGeoManager</i>.
     */
    Vizard::~Vizard() {
        delete gm;
    }
    
    /**
     * This function turns the abstract representation of the active and inactive surfaces in a tracker geometry into
     * a series of ROOT shapes. Those shapes are added as leaves to the collections of volumes that were previously
     * initialised in the constructor. Once all of them have been added to the volume tree, the geometry manager is
     * closed, making it ready to be displayed or written to file.
     *
     * NOTE: It is highly recommended to use the <i>simplified</i> flag to have layers and discs represented as
     * bounding boxes rather than by individual module. In a typical case, the modules in a tracker number in the
     * thousands; since volume creation and placement in this function is at this point unoptimised, this will cause
     * considerable strain on the resources of whatever is used to visualise the geometry tree later.
     *
     * @param am A reference to the tracker object that contains the collection of active surfaces
     * @param is A reference to the collection of inactive surfaces
     * @param simplified A flag indicating whether to draw bounding boxes around the layers/discs or whether to display each module individually
     */
    void Vizard::buildVisualization(Tracker& am, InactiveSurfaces& is, bool simplified) {
        int c = 0;
        TGeoVolume* vol=NULL;
        TGeoTranslation* trans=NULL;
        TGeoCombiTrans* trafo=NULL;
        Layer* current=NULL;
        std::vector<Module*> templates;
        // barrels
        if (simplified) {
            // layer loop, one tube per layer
            for (unsigned int i = 0; i < am.getBarrelLayers()->size(); i++) {
                current = am.getBarrelLayers()->at(i);
                // short layers
                if ((current->getMinZ() > 0) || (current->getMaxZ() < 0)) {
                    vol = gm->MakeTube("", medact, current->getMinRho(), current->getMaxRho(), (current->getMaxZ() - current->getMinZ()) / 2.0);
                    vol->SetLineColor(kRed);
                    trans = new TGeoTranslation(0, 0, current->getMaxZ() - (current->getMaxZ() - current->getMinZ()) / 2.0);
                    barrels->AddNode(vol, c, trans);
                }
                // regular layers
                else {
                    vol = gm->MakeTube("", medact, current->getMinRho(), current->getMaxRho(), current->getMaxZ());
                    vol->SetLineColor(kRed);
                    barrels->AddNode(vol, c);
                }
                c++;
            }
        }
        else c = detailedModules(am.getBarrelLayers(), vol, trafo, barrels, c);
        // endcaps
        if (simplified) {
            // disc loop, one (very short) tube per disc
            for (unsigned int i = 0; i < am.getEndcapLayers()->size(); i++) {
                current = am.getEndcapLayers()->at(i);
                vol = gm->MakeTube("", medact, current->getMinRho(), current->getMaxRho(),
                        (current->getMaxZ() - current->getMinZ()) / 2.0);
                vol->SetLineColor(kRed);
                trans = new TGeoTranslation(0, 0, current->getMaxZ() - (current->getMaxZ() - current->getMinZ()) / 2.0);
                endcaps->AddNode(vol, c, trans);
                c++;
            }
        }
        else c = detailedModules(am.getEndcapLayers(), vol, trafo, endcaps, c);
        
        // services
        int skip = is.getBarrelServices().size() / 2;
        // barrel services loop using symmetries with respect to z=0
        for (int i = 0; i < skip; i++) {
            vol = gm->MakeTube("", medserf, is.getBarrelServicePart(i).getInnerRadius(),
                    is.getBarrelServicePart(i).getInnerRadius() + is.getBarrelServicePart(i).getRWidth(),
                    is.getBarrelServicePart(i).getZLength() / 2.0);
            vol->SetLineColor(kBlue);
            trans = new TGeoTranslation(0, 0, (is.getBarrelServicePart(i).getZOffset() + is.getBarrelServicePart(i).getZLength() / 2.0));
            services->AddNode(vol, c, trans);
            trans = new TGeoTranslation(0, 0, (is.getBarrelServicePart(i + skip).getZOffset() + is.getBarrelServicePart(i + skip).getZLength() / 2.0));
            services->AddNode(vol, c + skip, trans);
            c++;
        }
        c = c + skip;
        skip = is.getEndcapServices().size() / 2;
        // endcap services loop using symmetries with respect to z=0
        for (int i = 0; i < skip; i++) {
            vol = gm->MakeTube("", medserf, is.getEndcapServicePart(i).getInnerRadius(),
                    is.getEndcapServicePart(i).getInnerRadius() + is.getEndcapServicePart(i).getRWidth(),
                    is.getEndcapServicePart(i).getZLength() / 2.0);
            vol->SetLineColor(kBlue);
            trans = new TGeoTranslation(0, 0, (is.getEndcapServicePart(i).getZOffset() + is.getEndcapServicePart(i).getZLength() / 2.0));
            services->AddNode(vol, c, trans);
            trans = new TGeoTranslation(0, 0, (is.getEndcapServicePart(i + skip).getZOffset() + is.getEndcapServicePart(i + skip).getZLength() / 2.0));
            services->AddNode(vol, c + skip, trans);
            c++;
        }
        c = c + skip;
        
        // supports
        skip = is.getSupports().size();
        // support parts loop, using all entries
        for (int i = 0; i < skip; i++) {
            // process entry if its rightmost point is in z+ - this includes tubes that cross z=0 but not disc supports in z-
            if ((is.getSupportPart(i).getZOffset() + is.getSupportPart(i).getZLength()) > 0) {
                vol = gm->MakeTube("", medlazy, is.getSupportPart(i).getInnerRadius(),
                        is.getSupportPart(i).getInnerRadius() + is.getSupportPart(i).getRWidth(),
                        is.getSupportPart(i).getZLength() / 2.0);
                vol->SetLineColor(kGray);
                trans = new TGeoTranslation(0, 0, (is.getSupportPart(i).getZOffset() + is.getSupportPart(i).getZLength() / 2.0));
                supports->AddNode(vol, c, trans);
                c++;
                // use symmetries with respect to z=0: if volume is completely in z+, it will have a twin in z-
                if (is.getSupportPart(i).getZOffset() > 0) {
                    trans = new TGeoTranslation(0, 0, (0.0 - is.getSupportPart(i).getZOffset() - is.getSupportPart(i).getZLength() / 2.0));
                    supports->AddNode(vol, c, trans);
                    c++;
                }
            }
        }
        // check overlaps, write status to cout
        gm->CloseGeometry();
        geometry_created = true;
    }
    
    /**
     * This function writes the previously created geometry tree (including the geometry manager) to a ROOT
     * file. If it finds that the internal representation using ROOT shapes has not been initialised, it prints an
     * error message and does nothing.
     * @param rootfilename The name of the output file that will be written to the application's default directory for root files
     */
    void Vizard::display(std::string rootfilename) {
        if (geometry_created) {
            std::string outfilename = default_rootfiledir + "/";
            if (rootfilename.empty()) outfilename = outfilename + default_rootfile;
            else outfilename = outfilename + rootfilename;
            TFile f(outfilename.c_str(), "recreate");
            if (f.IsZombie()) {
                std::cout << root_wrong << std::endl;
            }
            else {
                gm->Write();
                f.Close();
                std::cout << "Geometry written to file '" << outfilename << "'." << std::endl;
            }
            // display top volume after opening file in ROOT with:
            // TFile f("rootfiles/output.root");
            // TGeoManager* gm = (TGeoManager*)f.Get("display");
            // gm->GetMasterVolume()->Draw("ogl");
            // press 'w' for wireframe or 't' for outline view
            // when done: delete gm, f.Close()
        }
        else std::cout << msg_uninitialised << std::endl;
    }
    
    /**
     * This convenience function provides a frame for creation of a geometry tree from a tracker object and a
     * collection of inactive surfaces and for writing them to a ROOT file in a single step.
     * @param am A reference to the tracker object that contains the collection of active surfaces
     * @param is A reference to the collection of inactive surfaces
     * @param rootfilename The name of the output file that will be written to the application's default directory for ROOT files
     * @param simplified A flag indicating whether to draw bounding boxes around the layers/discs or whether to display each module individually
     */
    void Vizard::display(Tracker& am, InactiveSurfaces& is, std::string rootfilename, bool simplified) {
        buildVisualization(am, is, simplified);
        display(rootfilename);
    }
    
    /**
     * This convenience function writes the feeder/neighbour relations of a collection of inactive surfaces to a
     * default file.
     * @param is A reference to the collection of inactive surfaces
     */
    void Vizard::writeNeighbourGraph(InactiveSurfaces& is) {
        writeNeighbourGraph(is, default_graphfile);
    }
    
    /**
     * This function writes the feeder/neighbour relations in a collection of inactive surfaces to a very simple text
     * file. It essentially lists all edges of the neighbour graph, first those in the barrels, then those in the endcaps,
     * but otherwise in a more or less unordered heap: the more order in the source collection, the more order in
     * the output. If no name is given for the output file, a default filename is used.
     * @param is A reference to the collection of inactive surfaces
     * @param outfile The name of the output file that will be written to the application's default directory for graph files
     */
    void Vizard::writeNeighbourGraph(InactiveSurfaces& is, std::string outfile) {
        std::string filename = default_graphdir + "/";
        if (outfile.empty()) filename = filename + default_graphfile;
        else filename = filename + outfile;
        std::cout << "Preparing to write neighbour graph to " << filename << "..." << std::endl;
        try {
            std::ofstream outstream(filename.c_str());
            if (outstream) {
                // barrel services loop
                outstream << "BARREL SERVICES:" << std::endl << std::endl;
                for (unsigned int i = 0; i < is.getBarrelServices().size(); i++) {
                    outstream << "Barrel element " << i << ": service is ";
                    if (is.getBarrelServicePart(i).isFinal()) outstream << "final and ";
                    else outstream << "not final and ";
                    if (is.getBarrelServicePart(i).isVertical()) outstream << "vertical.";
                    else outstream << "horizontal.";
                    outstream << std::endl << "Feeder type: ";
                    switch (is.getBarrelServicePart(i).getFeederType()) {
                        case InactiveElement::no_in: outstream << "none, ";
                        break;
                        case InactiveElement::tracker: outstream << "tracker, ";
                        break;
                        case InactiveElement::barrel: outstream << "barrel service, ";
                        break;
                        case InactiveElement::endcap: outstream << "endcap service, ";
                        break;
                        default: outstream << "something weird, ";
                    }
                    outstream << "feeder index = " << is.getBarrelServicePart(i).getFeederIndex() << ".";
                    outstream << std::endl << "Neighbour type: ";
                    switch (is.getBarrelServicePart(i).getNeighbourType()) {
                        case InactiveElement::no_in: outstream << "none, ";
                        break;
                        case InactiveElement::tracker: outstream << "tracker, ";
                        break;
                        case InactiveElement::barrel: outstream << "barrel service, ";
                        break;
                        case InactiveElement::endcap: outstream << "endcap service, ";
                        break;
                        default: outstream << "something weird, ";
                    }
                    outstream << "neighbour index = " << is.getBarrelServicePart(i).getNeighbourIndex() << ".";
                    outstream << std::endl << std::endl;
                }
                // endcap services
                outstream << "ENDCAP SERVICES:" << std::endl << std::endl;
                for (unsigned int i = 0; i < is.getEndcapServices().size(); i++) {
                    outstream << "Endcap element " << i << ": service is ";
                    if (is.getEndcapServicePart(i).isFinal()) outstream << "final and ";
                    else outstream << "not final and ";
                    if (is.getEndcapServicePart(i).isVertical()) outstream << "vertical.";
                    else outstream << "horizontal.";
                    outstream << std::endl << "Feeder type: ";
                    switch (is.getEndcapServicePart(i).getFeederType()) {
                        case InactiveElement::no_in: outstream << "none, ";
                        break;
                        case InactiveElement::tracker: outstream << "tracker, ";
                        break;
                        case InactiveElement::barrel: outstream << "barrel service, ";
                        break;
                        case InactiveElement::endcap: outstream << "endcap service, ";
                        break;
                        default: outstream << "something weird, ";
                    }
                    outstream << "feeder index = " << is.getEndcapServicePart(i).getFeederIndex() << ".";
                    outstream << std::endl << "Neighbour type: ";
                    switch (is.getEndcapServicePart(i).getNeighbourType()) {
                        case InactiveElement::no_in: outstream << "none, ";
                        break;
                        case InactiveElement::tracker: outstream << "tracker, ";
                        break;
                        case InactiveElement::barrel: outstream << "barrel service, ";
                        break;
                        case InactiveElement::endcap: outstream << "endcap service, ";
                        break;
                        default: outstream << "something weird, ";
                    }
                    outstream << "neighbour index = " << is.getEndcapServicePart(i).getNeighbourIndex() << ".";
                    outstream << std::endl << std::endl;
                }
                outstream.close();
                std::cout << "Neighbour graph written to " << filename << "." << std::endl;
            }
            else std::cout << graph_wrong << std::endl;
        }
        catch (std::bad_alloc ba) {
            std::cerr << exc_badalloc_graph << graph_nowrite << std::endl;
        }
    }
    
    /**
     * This function is meant to write the feeder/neighbour relations of a given collection of inactive surfaces
     * to a DOT file instead of the quick and dirty text format that is used at the moment.
     *
     * NOTE: This function is currently in DEVELOPMENT HELL. There is no way of knowing if it will ever
     * finished, or when. So for now, the function can be called, but it does NOTHING AT ALL. Don't say you
     * haven't been warned.
     * @param is A reference to the collection of inactive surfaces
     * @param outfile The name of the output file that will be written to the application's default directory for graph files
     */
    void Vizard::dotGraph(InactiveSurfaces& is, std::string outfile) {
        const std::string preamble = "digraph tracker";
        const std::string ori = "rankdir=DU"; // check if this is possible!
        const std::string shape = "node [shape=box]";
        const std::string label = "label=";
        const std::string edge = "->";
    }
   

    /**
     * This function draws some of the histograms that were filled during material budget analysis and
     * embeds the resulting image in an HTML file for easy access. If no name is given for the output file,
     * a default filename is used.
     * @param a A reference to the analysing class that examined the material budget and filled the histograms
     * @param outfilename The name of the output file that will be written to the application's default directory for material budget summaries
     */
    void Vizard::histogramSummary(Analyzer& a, std::string outfilename) {
        THStack rcontainer("rstack", "Radiation Length by Category");
        THStack icontainer("istack", "Interaction Length by Category");
        TH1D *cr = NULL, *ci = NULL, *fr1 = NULL, *fi1 = NULL, *fr2 = NULL, *fi2 = NULL;
        TH1D *acr = NULL, *aci = NULL, *ser = NULL, *sei = NULL, *sur = NULL, *sui = NULL;
        TH2D *ir = NULL, *ii = NULL;
        TVirtualPad* pad;
        // filename gymnastics
        std::string outfile = default_summarypath + "/";
        std::string pngoutfile, pngout, pngpath;
        std::string svgout, svgpath;
        std::string Cout, Cpath;
        if (outfilename.empty()) {
            outfile = outfile + default_summary;
            pngoutfile = default_summary;
        }
        else {
            outfile = outfile + outfilename;
            pngoutfile = outfilename;
        }
        if (pngoutfile.substr(pngoutfile.size() - 4) == "html") {
            pngoutfile = pngoutfile.substr(0, pngoutfile.size() - 4);
            if (pngoutfile.substr(pngoutfile.size() - 1) == ".") pngoutfile = pngoutfile.substr(0, pngoutfile.size() - 1);
        }
        // output initialisation and headers
        std::ostringstream htmlstream;
        std::ofstream outstream(outfile.c_str());
        htmlstream << "<html><title>" << outfilename << "</title><body>";
        htmlstream << "<p><big><b>1D Overview</b></big></p>";
        TCanvas c("matbudgetcanvas", "Material Budgets over Eta", 900, 400);
        c.SetFillColor(color_plot_background);
        c.Divide(2, 1);
        pad = c.GetPad(0);
        pad->SetFillColor(color_pad_background);
        pad = c.GetPad(1);
        pad->cd();
        // total tracking volume rlength
        cr = (TH1D*)a.getHistoGlobalR().Clone();
        fr1 = (TH1D*)a.getHistoExtraServicesR().Clone();
        fr2 = (TH1D*)a.getHistoExtraSupportsR().Clone();
        fr1 = (TH1D*)a.getHistoExtraServicesR().Clone();
        fr2 = (TH1D*)a.getHistoExtraSupportsR().Clone();
        cr->Add(fr1);
        cr->Add(fr2);
        cr->SetFillColor(kGray + 2);
        cr->SetNameTitle("rfullvolume", "Radiation Length Over Full Tracker Volume");
        cr->SetXTitle("#eta");
        cr->Draw();
        pad = c.GetPad(2);
        pad->cd();
        // total tracking volume ilength
        ci = (TH1D*)a.getHistoGlobalI().Clone();
        fi1 = (TH1D*)a.getHistoExtraServicesI().Clone();
        fi2 = (TH1D*)a.getHistoExtraSupportsI().Clone();
        fi1 = (TH1D*)a.getHistoExtraServicesI().Clone();
        fi2 = (TH1D*)a.getHistoExtraSupportsI().Clone();
        ci->Add(fi1);
        ci->Add(fi2);
        ci->SetFillColor(kGray + 1);
        ci->SetNameTitle("ifullvolume", "Interaction Length Over Full Tracker Volume");
        ci->SetXTitle("#eta");
        ci->Draw();
        // write total plots to file
        pngout = pngoutfile + ".fullvolume.png";
        pngpath = default_summarypath + "/" + pngout;
        svgout = pngoutfile + ".fullvolume.svg";
        svgpath = default_summarypath + "/" + svgout;
        Cout = pngoutfile + ".fullvolume.C";
        Cpath = default_summarypath + "/" + Cout;
        c.SaveAs(pngpath.c_str());
        c.SaveAs(svgpath.c_str());
        c.SaveAs(Cpath.c_str());
        htmlstream << "<img src=\"" << pngout << "\" /><br><p><small><b>Average radiation length in full volume ";
        htmlstream << "(eta = [0, 2.4]): " << averageHistogramValues(*cr, etaMaxAvg) << "</b></small></p>";
        htmlstream << "<p><small><b>Average interaction length in full volume (eta = [0, 2.4]): ";
        htmlstream << averageHistogramValues(*ci, etaMaxAvg) << "</b></small></p>";
        // work area cleanup an re-init
        c.Clear("D");
        if (cr) delete cr;
        if (ci) delete ci;
        pad = c.GetPad(1);
        pad->cd();
        // global plots in tracking volume: radiation length
        cr = (TH1D*)a.getHistoGlobalR().Clone();
        cr->SetFillColor(kGray + 2);
        cr->SetNameTitle("rglobal", "Overall Radiation Length");
        cr->SetXTitle("#eta");
        cr->Draw();
        pad = c.GetPad(2);
        pad->cd();
        // global plots in tracking volume: interaction length
        ci = (TH1D*)a.getHistoGlobalI().Clone();
        ci->SetFillColor(kGray + 1);
        ci->SetNameTitle("iglobal", "Overall Interaction Length");
        ci->SetXTitle("#eta");
        ci->Draw();
        // write global tracking volume plots to file
        pngout = pngoutfile + ".global.png";
        pngpath = default_summarypath + "/" + pngout;
        svgout = pngoutfile + ".global.svg";
        svgpath = default_summarypath + "/" + svgout;
        Cout = pngoutfile + ".global.C";
        Cpath = default_summarypath + "/" + Cout;
        c.SaveAs(pngpath.c_str());
        c.SaveAs(svgpath.c_str());
        c.SaveAs(Cpath.c_str());
        htmlstream << "<br><img src=\"" << pngout << "\" /><br><p><small><b>Average radiation length in tracking volume ";
        htmlstream << "(eta = [0, 2.4]): " << averageHistogramValues(*cr, etaMaxAvg) << "</b></small></p>";
        htmlstream << "<p><small><b>Average interaction length in tracking volume (eta = [0, 2.4]): ";
        htmlstream << averageHistogramValues(*ci, etaMaxAvg) << "</b></small></p>";
        // work area cleanup and re-init
        c.Clear("D");
        pad = c.GetPad(1);
        pad->cd();
        // radiation length in tracking volume by active, serving or passive
        sur = (TH1D*)a.getHistoSupportsAllR().Clone();
        sur->SetFillColor(kOrange + 4);
        sur->SetXTitle("#eta");
        rcontainer.Add(sur);
        ser = (TH1D*)a.getHistoServicesAllR().Clone();
        ser->SetFillColor(kBlue);
        ser->SetXTitle("#eta");
        rcontainer.Add(ser);
        acr = (TH1D*)a.getHistoModulesAllR().Clone();
        acr->SetFillColor(kRed);
        acr->SetXTitle("#eta");
        rcontainer.Add(acr);
        rcontainer.Draw();
        // interaction length in tracking volume by active, serving or passive
        pad = c.GetPad(2);
        pad->cd();
        sui = (TH1D*)a.getHistoSupportsAllI().Clone();
        sui->SetFillColor(kOrange + 2);
        sui->SetXTitle("#eta");
        icontainer.Add(sui);
        sei = (TH1D*)a.getHistoServicesAllI().Clone();
        sei->SetFillColor(kAzure - 2);
        sei->SetXTitle("#eta");
        icontainer.Add(sei);
        aci = (TH1D*)a.getHistoModulesAllI().Clone();
        aci->SetFillColor(kRed - 3);
        aci->SetXTitle("#eta");
        icontainer.Add(aci);
        icontainer.Draw();
        // write asl category plots to file
        pngout = pngoutfile + ".asl.png";
        pngpath = default_summarypath + "/" + pngout;
        svgout = pngoutfile + ".asl.svg";
        svgpath = default_summarypath + "/" + svgout;
        Cout = pngoutfile + ".asl.C";
        Cpath = default_summarypath + "/" + Cout;
        c.SaveAs(pngpath.c_str());
        c.SaveAs(svgpath.c_str());
        c.SaveAs(Cpath.c_str());
        htmlstream << "<img src=\"" << pngout << "\" /><br>";
        // average values by active, service and passive
        htmlstream << "<br><p><small><b>Average radiation length in modules ";
        htmlstream << "(eta = [0, 2.4]): " << averageHistogramValues(*acr, etaMaxAvg) << "</b></small></p>";
        htmlstream << "<p><small><b>Average radiation length in services ";
        htmlstream << "(eta = [0, 2.4]): " << averageHistogramValues(*ser, etaMaxAvg) << "</b></small></p>";
        htmlstream << "<p><small><b>Average radiation length in supports ";
        htmlstream << "(eta = [0, 2.4]): " << averageHistogramValues(*sur, etaMaxAvg) << "</b></small></p>";
        htmlstream << "<br><p><small><b>Average interaction length in modules (eta = [0, 2.4]): ";
        htmlstream << averageHistogramValues(*aci, etaMaxAvg) << "</b></small></p>";
        htmlstream << "<p><small><b>Average interaction length in services (eta = [0, 2.4]): ";
        htmlstream << averageHistogramValues(*sei, etaMaxAvg) << "</b></small></p>";
        htmlstream << "<p><small><b>Average interaction length in supports (eta = [0, 2.4]): ";
        htmlstream << averageHistogramValues(*sui, etaMaxAvg) << "</b></small></p>";
        // work area cleanup and re-init
        c.Clear("D");
        pad = c.GetPad(1);
        pad->cd();
        // radiation length in isolines
        ir = (TH2D*)a.getHistoIsoR().Clone();
        ir->SetNameTitle("isor", "Radiation Length Contours");
        ir->SetContour(temperature_levels, NULL);
        ir->SetXTitle("z");
        ir->SetYTitle("r");
        ir->Draw("COLZ");
        pad = c.GetPad(2);
        pad->cd();
        // interaction length in isolines
        ii = (TH2D*)a.getHistoIsoI().Clone();
        ii->SetNameTitle("isoi", "Interaction Length Contours");
        ii->SetContour(temperature_levels, NULL);
        ii->SetXTitle("z");
        ii->SetYTitle("r");
        ii->Draw("COLZ");
        // write isoline plots to file
        pngout = pngoutfile + ".twodee.png";
        pngpath = default_summarypath + "/" + pngout;
        svgout = pngoutfile + ".twodee.svg";
        svgpath = default_summarypath + "/" + svgout;
        Cout = pngoutfile + ".twodee.C";
        Cpath = default_summarypath + "/" + Cout;
        c.SaveAs(pngpath.c_str());
        c.SaveAs(svgpath.c_str());
        c.SaveAs(Cpath.c_str());
        htmlstream << "<br><p><big><b>2D Overview</b></big></p><img src=\"" << pngout << "\" /><br>";
        // general cleanup
        if (cr) delete cr;
        if (ci) delete ci;
        if (fr1) delete fr1;
        if (fi1) delete fi1;
        if (fr2) delete fr2;
        if (fi2) delete fi2;
        if (acr) delete acr;
        if (aci) delete aci;
        if (ser) delete ser;
        if (sei) delete sei;
        if (sur) delete sur;
        if (sui) delete sui;
        // write html frame to file
        htmlstream << "</body></html>";
        outstream << htmlstream.str() << std::endl;
        outstream.close();
        std::cout << "HTML file written to " << outfile << std::endl;
    }
    
#ifdef USING_ROOTWEB

    void Vizard::histogramSummary(Analyzer& a, RootWSite& site) {
        histogramSummary(a, site, "outer");
    }

    /**
     * This function draws some of the histograms that were filled during material budget analysis
     * with the rootweb library
     * @param a A reference to the analysing class that examined the material budget and filled the histograms
     * @param site the RootWSite object for the output
     */
    void Vizard::histogramSummary(Analyzer& a, RootWSite& site, std::string name) {
        // Initialize the page with the material budget
        RootWPage* myPage;
        RootWContent* myContent;
        RootWTable* myTable;
        RootWImage* myImage;
        TCanvas* myCanvas;
        TVirtualPad* myPad;
        std::string pageTitle="Material";
        if (name!="") pageTitle+=" (" +name+")";
        myPage = new RootWPage(pageTitle);
        std::string pageAddress="material"+name+".html";
        myPage->setAddress(pageAddress);
        site.addPage(myPage);
        
        // 1D Overview
        myContent = new RootWContent("1D Overview");
        myPage->addContent(myContent);
        
        // Book histograms
        THStack* rcontainer = new THStack("rstack", "Radiation Length by Category");
        THStack* icontainer = new THStack("istack", "Interaction Length by Category");
        TH1D *cr = NULL, *ci = NULL, *fr1 = NULL, *fi1 = NULL, *fr2 = NULL, *fi2 = NULL;
        TH1D *acr = NULL, *aci = NULL, *ser = NULL, *sei = NULL, *sur = NULL, *sui = NULL;
#ifdef MATERIAL_SHADOW
        TH2D *ir = NULL, *ii = NULL;
#endif
	TH2D *mapRad = NULL, *mapInt = NULL;
        
        // Output initialisation and headers
        myCanvas = new TCanvas("overviewMaterial");
        myCanvas->SetFillColor(color_plot_background);
        myCanvas->Divide(2, 1);
        myPad = myCanvas->GetPad(0);
        myPad->SetFillColor(color_pad_background);
        myPad = myCanvas->GetPad(1);
        myPad->cd();
        // Total tracking volume rlength
        cr = (TH1D*)a.getHistoGlobalR().Clone();
        fr1 = (TH1D*)a.getHistoExtraServicesR().Clone();
        fr2 = (TH1D*)a.getHistoExtraSupportsR().Clone();
        fr1 = (TH1D*)a.getHistoExtraServicesR().Clone();
        fr2 = (TH1D*)a.getHistoExtraSupportsR().Clone();
        cr->Add(fr1);
        cr->Add(fr2);
        cr->SetFillColor(kGray + 2);
        cr->SetNameTitle("rfullvolume", "Radiation Length Over Full Tracker Volume");
        cr->SetXTitle("#eta");
        cr->Draw();
        myPad = myCanvas->GetPad(2);
        myPad->cd();
        // Total Tracking volume ilength
        ci = (TH1D*)a.getHistoGlobalI().Clone();
        fi1 = (TH1D*)a.getHistoExtraServicesI().Clone();
        fi2 = (TH1D*)a.getHistoExtraSupportsI().Clone();
        fi1 = (TH1D*)a.getHistoExtraServicesI().Clone();
        fi2 = (TH1D*)a.getHistoExtraSupportsI().Clone();
        ci->Add(fi1);
        ci->Add(fi2);
        ci->SetFillColor(kGray + 1);
        ci->SetNameTitle("ifullvolume", "Interaction Length Over Full Tracker Volume");
        ci->SetXTitle("#eta");
        ci->Draw();
        // Put the total plots to the site
        myImage = new RootWImage(myCanvas, 900, 400);
        myImage->setComment("Material in full volume");
	myImage->setName("matFull");
        myTable = new RootWTable();
        // TODO: put etaMaxAvg correctly in the string :)
        myTable->setContent(1, 1, "Average radiation length in full volume (eta = [0, 2.4])");
        myTable->setContent(2, 1, "Average interaction length in full volume (eta = [0, 2.4])");
        myTable->setContent(1, 2, averageHistogramValues(*cr, etaMaxAvg), 5);
        myTable->setContent(2, 2, averageHistogramValues(*ci, etaMaxAvg), 5);
        myContent->addItem(myTable);
        myContent->addItem(myImage);
        
        // Detailed plots
        myContent = new RootWContent("Tracking volume", false);
        myPage->addContent(myContent);
        // Work area re-init
        myCanvas = new TCanvas("materialInTrackingVolume");
        myCanvas->SetFillColor(color_plot_background);
        myCanvas->Divide(2, 1);
        myPad = myCanvas->GetPad(0);
        myPad->SetFillColor(color_pad_background);
        myPad = myCanvas->GetPad(1);
        myPad->cd();
        // global plots in tracking volume: radiation length
        cr = (TH1D*)a.getHistoGlobalR().Clone();
        cr->SetFillColor(kGray + 2);
        cr->SetNameTitle("rglobal", "Overall Radiation Length");
        cr->SetXTitle("#eta");
        cr->Draw();
        myPad = myCanvas->GetPad(2);
        myPad->cd();
        // global plots in tracking volume: interaction length
        ci = (TH1D*)a.getHistoGlobalI().Clone();
        ci->SetFillColor(kGray + 1);
        ci->SetNameTitle("iglobal", "Overall Interaction Length");
        ci->SetXTitle("#eta");
        ci->Draw();
        // Write global tracking volume plots to web pag
        myImage = new RootWImage(myCanvas, 900, 400);
        myImage->setComment("Material in tracking volume");
	myImage->setName("matTrack");
        myTable = new RootWTable();
        myTable->setContent(1, 1, "Average radiation length in tracking volume (eta = [0, 2.4])");
        myTable->setContent(2, 1, "Average interaction length in tracking volume (eta = [0, 2.4])");
        myTable->setContent(1, 2, averageHistogramValues(*cr, etaMaxAvg), 5);
        myTable->setContent(2, 2, averageHistogramValues(*ci, etaMaxAvg), 5);
        myContent->addItem(myTable);
        myContent->addItem(myImage);
        
        // Detailed plots
        myContent = new RootWContent("Detailed", false);
        myPage->addContent(myContent);
        // Work area re-init
        myCanvas = new TCanvas("detailedMaterial");
        myCanvas->SetFillColor(color_plot_background);
        myCanvas->Divide(2, 1);
        myPad = myCanvas->GetPad(0);
        myPad->SetFillColor(color_pad_background);
        myPad = myCanvas->GetPad(1);
        myPad->cd();
        // radiation length in tracking volume by active, serving or passive
        sur = (TH1D*)a.getHistoSupportsAllR().Clone();
        sur->SetFillColor(kOrange + 4);
        sur->SetXTitle("#eta");
        rcontainer->Add(sur);
        ser = (TH1D*)a.getHistoServicesAllR().Clone();
        ser->SetFillColor(kBlue);
        ser->SetXTitle("#eta");
        rcontainer->Add(ser);
        acr = (TH1D*)a.getHistoModulesAllR().Clone();
        acr->SetFillColor(kRed);
        acr->SetXTitle("#eta");
        rcontainer->Add(acr);
        rcontainer->Draw();
        // interaction length in tracking volume by active, serving or passive
        myPad = myCanvas->GetPad(2);
        myPad->cd();
        sui = (TH1D*)a.getHistoSupportsAllI().Clone();
        sui->SetFillColor(kOrange + 2);
        sui->SetXTitle("#eta");
        icontainer->Add(sui);
        sei = (TH1D*)a.getHistoServicesAllI().Clone();
        sei->SetFillColor(kAzure - 2);
        sei->SetXTitle("#eta");
        icontainer->Add(sei);
        aci = (TH1D*)a.getHistoModulesAllI().Clone();
        aci->SetFillColor(kRed - 3);
        aci->SetXTitle("#eta");
        icontainer->Add(aci);
        icontainer->Draw();
        
        // Write asl category plots to web page
        myImage = new RootWImage(myCanvas, 900, 400);
        myImage->setComment("Detailed");
	myImage->setName("matTrackDet");
        myTable = new RootWTable();
        // Average values by active, service and passive
        myTable->setContent(0, 0, "Average (eta = [0, 2.4])");
        myTable->setContent(1, 0, "modules");
        myTable->setContent(2, 0, "services");
        myTable->setContent(3, 0, "supports");
        myTable->setContent(0, 1, "Radiation length");
        myTable->setContent(0, 2, "Interaction length");
        myTable->setContent(1, 1, averageHistogramValues(*acr, etaMaxAvg), 5);
        myTable->setContent(2, 1, averageHistogramValues(*ser, etaMaxAvg), 5);
        myTable->setContent(3, 1, averageHistogramValues(*sur, etaMaxAvg), 5);
        myTable->setContent(1, 2, averageHistogramValues(*aci, etaMaxAvg), 5);
        myTable->setContent(2, 2, averageHistogramValues(*sei, etaMaxAvg), 5);
        myTable->setContent(3, 2, averageHistogramValues(*sui, etaMaxAvg), 5);
        myContent->addItem(myTable);
        myContent->addItem(myImage);
        
        
        // Work area re-init
        myCanvas = new TCanvas("countourMaterial");
        myCanvas->SetFillColor(color_plot_background);
        myCanvas->Divide(2, 1);
        myPad = myCanvas->GetPad(0);
        myPad->SetFillColor(color_pad_background);
        myPad = myCanvas->GetPad(1);
        myPad->cd();

        // Countour plots
        myContent = new RootWContent("Material distributon", true);
        myPage->addContent(myContent);

#ifdef MATERIAL_SHADOW        
        // radiation length in isolines
        ir = (TH2D*)a.getHistoIsoR().Clone();
        ir->SetNameTitle("isor", "Radiation Length Contours");
        ir->SetContour(temperature_levels, NULL);
        ir->SetXTitle("z");
        ir->SetYTitle("r");
        ir->Draw("COLZ");
        myPad = myCanvas->GetPad(2);
        myPad->cd();
        // interaction length in isolines
        ii = (TH2D*)a.getHistoIsoI().Clone();
        ii->SetNameTitle("isoi", "Interaction Length Contours");
        ii->SetContour(temperature_levels, NULL);
        ii->SetXTitle("z");
        ii->SetYTitle("r");
        ii->Draw("COLZ");
        // Write isoline plots to web page
        myImage = new RootWImage(myCanvas, 900, 400);
        myImage->setComment("Material 2D distributions");
	myImage->setName("matShadow");
        myContent->addItem(myImage);
#endif // MATERIAL_SHADOW

	// Radiation length plot
        myCanvas = new TCanvas("mapMaterialRadiation");
        myCanvas->SetFillColor(color_plot_background);
        myCanvas->cd();
	mapRad = (TH2D*)a.getHistoMapRadiation().Clone();
	mapRad->SetContour(temperature_levels, NULL);
	//myCanvas->SetLogz();
	mapRad->Draw("COLZ");
        myImage = new RootWImage(myCanvas, 900, 400);
        myImage->setComment("Radiation length material map");
	myImage->setName("matMapR");
        myContent->addItem(myImage);

	// Interaction length plot
        myCanvas = new TCanvas("mapMaterialInteraction");
        myCanvas->SetFillColor(color_plot_background);
        myCanvas->cd();
	mapInt = (TH2D*)a.getHistoMapInteraction().Clone();
	mapInt->SetContour(temperature_levels, NULL);
	mapInt->Draw("COLZ");
        myImage = new RootWImage(myCanvas, 900, 400);
        myImage->setComment("Interaction length material map");
	myImage->setName("matMapI");
        myContent->addItem(myImage);

        // Nuclear interactions
        myContent = new RootWContent("Nuclear interactions", true);
        myPage->addContent(myContent);
	
	// Number of hits
	myCanvas = new TCanvas("hadronsHitsNumber");
	myCanvas->SetFillColor(color_plot_background);
	myCanvas->cd();
	TGraph* hadronTotalHitsProfile = new TGraph(a.getHadronTotalHitsProfile());
	TGraph* hadronAverageHitsProfile = new TGraph(a.getHadronAverageHitsProfile());
	hadronTotalHitsProfile->SetMarkerStyle(8);
	hadronTotalHitsProfile->SetMarkerColor(kBlack);
	hadronTotalHitsProfile->SetMinimum(0);
	hadronTotalHitsProfile->Draw("alp");
	hadronAverageHitsProfile->SetMarkerStyle(8);
	hadronAverageHitsProfile->SetMarkerColor(kRed);
	hadronAverageHitsProfile->Draw("same lp");
	myImage = new RootWImage(myCanvas, 600, 600);
        myImage->setComment("Maximum and average number of points (hadrons)");
	myImage->setName("hadHits");
        myContent->addItem(myImage);	

	// Number of hits
	std::vector<TGraph> hadronGoodTracksFraction=a.getHadronGoodTracksFraction();
	std::vector<double> hadronNeededHitsFraction=a.getHadronNeededHitsFraction();
	myCanvas = new TCanvas("hadronsTracksFraction");
	myCanvas->SetFillColor(color_plot_background);
	myCanvas->cd();
	TLegend* myLegend = new TLegend(0.75, 0.16, .95, .40);
	Palette::prepare(hadronGoodTracksFraction.size()); // there was a 120 degree phase here
	TH1D* ranger = new TH1D("hadTrackRanger","", 100, 0, 3);
	ranger->SetMaximum(1.);
	TAxis* myAxis;
	myAxis = ranger->GetXaxis();
	myAxis->SetTitle("#eta");
	myAxis = ranger->GetYaxis();
	myAxis->SetTitle("Tracks fraction");
	ranger->Draw();
	ostringstream tempSS;
	for (unsigned int i=0;
	     i<hadronGoodTracksFraction.size();
	     ++i) {
	  TGraph& myGraph = hadronGoodTracksFraction.at(i);
	  closeGraph(myGraph);
	  myGraph.SetFillColor(Palette::color(i));
	  myGraph.Draw("same F");
	  tempSS.str("");
	  if (hadronNeededHitsFraction.at(i)!=Analyzer::ZeroHitsRequired) {
	    if (hadronNeededHitsFraction.at(i)==Analyzer::OneHitRequired)
	      tempSS << "1 hit required";
	    else
	      tempSS << int(hadronNeededHitsFraction.at(i)*100)
		     << "% hits required";
	    myLegend->AddEntry(&myGraph, tempSS.str().c_str(), "F");
	  }
	}
	ranger->Draw("sameaxis");
	myLegend->Draw();
	myImage = new RootWImage(myCanvas, 600, 600);
        myImage->setComment("Fraction of tracks with a given fraction of good hits (hadrons)");
	myImage->setName("hadTracks");
        myContent->addItem(myImage);
    }
    
#endif
    
    // private
    /**
     * This function bundles the placement of a collection of individual modules in a ROOT geometry tree for
     * visualisation. It loops through the provided module vectors, determining their modules' corners and position
     * in space. Using that information, it adds a ROOT shape and a 3D transformation to a volume assembly
     * note from the geometry tree for each module found in the vectors.
     * @param layers A pointer to the list of layers or discs that is to be displayed
     * @param v A pointer to a template volume that is to be adjusted to the module shape
     * @param t A pointer to a transformation object that will describe the template volume's position in space
     * @param a A pointer to the assembly node that the template volume will be added to
     * @param counter The element counter that keeps track of how many volumes have been added to the geometry tree
     * @return The new value of the element counter
     */
    int Vizard::detailedModules(std::vector<Layer*>* layers,
            TGeoVolume* v, TGeoCombiTrans* t, TGeoVolumeAssembly* a, int counter) {
        Layer* current;
        Module* mod;
        if (!layers->empty()) {
            //  init of volume object for modules
            v = gm->MakeArb8("", medact, 0);
            v->SetLineColor(kRed);
            // layer loop
            for (unsigned int i = 0; i < layers->size(); i++) {
                current = layers->at(i);
                // module loop
                for (unsigned int j = 0; j < current->getModuleVector()->size(); j++) {
                    mod = current->getModuleVector()->at(j);
                    // place volume v according to information in module mod
                    t = modulePlacement(mod, v);
                    // add volume v to scene graph using translation t
                    a->AddNode(v, counter, t);
                    counter++;
                }
            }
        }
        else std::cout << "detailedModules(): layers vector is empty." << std::endl;
        return counter;
    }
    
    /**
     * This geometry function computes the transformation matrix that describes the position of a given module
     * in space. It also sets the shape of the provided template volume to correspond to that of the module.
     * @param m A pointer to the module object that needs to be visualised
     * @param v A pointer to the template volume that will represent the module in the visualisation
     * @return A pointer to the finished transformation matrix object
     */
    TGeoCombiTrans* Vizard::modulePlacement(Module* m, TGeoVolume* v) {
        XYZVector ex, ey, ez, b, c, d, p;
        TGeoArb8* arb;
        TGeoRotation* rot;
        TGeoCombiTrans* tr;
        // copy of module placement parameters in Module class
        b = m->getCorner(1) - m->getCorner(0);
        c = m->getCorner(2) - m->getCorner(0);
        d = m->getCorner(3) - m->getCorner(0);
        ex = b / b.R();
        p = (d.Dot(ex) * ex);
        // unit vectors for module coordinate system
        ey = d - p;
        ey = ey / ey.R();
        ez = ex.Cross(ey);
        // set vertices in volume v according to extracted module measurements
        arb = (TGeoArb8*)(v->GetShape());
        for (int i = 0; i < 5; i = i + 4) {
            arb->SetVertex(i, 0, 0);
            arb->SetVertex(i + 1, b.R(), 0);
            arb->SetVertex(i + 2, c.Dot(ex), c.Dot(ey));
            arb->SetVertex(i + 3, d.Dot(ex), d.Dot(ey));
        }
        // set position of module within the tracker volume
        double matrix[9];
        matrix[0] = ex.X();
        matrix[1] = ey.X();
        matrix[2] = ez.X();
        matrix[3] = ex.Y();
        matrix[4] = ey.Y();
        matrix[5] = ez.Y();
        matrix[6] = ex.Z();
        matrix[7] = ey.Z();
        matrix[8] = ez.Z();
        rot = new TGeoRotation();
        rot->SetMatrix(matrix);
        // save position in transformation object
        tr = new TGeoCombiTrans(m->getCorner(0).X(), m->getCorner(0).Y(), m->getCorner(0).Z(), rot);
        return tr;
    }
    
    /**
     * This function computes the average of a range of histogram bins: from the first to the one that includes a
     * cutoff value along the axis.
     * @param histo A reference to the histogram data
     * @param cutoff The cutoff value
     * @return The average value of the bins within range
     */
    double Vizard::averageHistogramValues(TH1D& histo, double cutoff) {
        double avg = 0.0;
        int cobin = 1;
        // find last relevant bin
        while ((cobin < histo.GetNbinsX()) && (histo.GetBinLowEdge(cobin) < cutoff)) cobin++;
        // calculate average
        if (cobin >= histo.GetNbinsX() - 1) avg = histo.GetMean();
        else {
            for (int i = 1; i <= cobin; i++) avg = avg + histo.GetBinContent(i) / (double)cobin;
        }
        return avg;
    }
    
    
#ifdef USING_ROOTWEB
    /**
     * This function draws the profile of hits obtained by the analysis of the geometry
     * together with the summaries in tables with the rootweb library. It also actually does a couple of
     * calculations to count modules and such, to put the results in the tables.
     * @param analyzer A reference to the analysing class that examined the material budget and filled the histograms
     * @param site the RootWSite object for the output
     */
  bool Vizard::geometrySummary(Analyzer& analyzer, Tracker& tracker, RootWSite& site) {
        
        // A bunch of indexes
        std::map<std::string, Module*> typeMap;
        std::map<std::string, int> typeMapCount;
        std::map<std::string, long> typeMapCountChan;
        std::map<std::string, double> typeMapMaxOccupancy;
        std::map<std::string, double> typeMapAveOccupancy;
        std::map<std::string, double> typeMapAveRphiResolution;
        std::map<std::string, double> typeMapAveYResolution;
        std::map<std::string, Module*>::iterator typeMapIt;
        std::map<int, Module*> ringTypeMap;
        std::string aSensorTag;
        LayerVector::iterator layIt;
        ModuleVector::iterator modIt;
        ModuleVector* aLay;
        double totAreaPts=0;
        double totAreaStrips=0;
        int totCountMod=0;
        int totCountSens=0;
        long totChannelStrips=0;
        long totChannelPts=0;
        
        RootWPage* myPage = new RootWPage("Geometry");
        // TODO: the web site should decide which page to call index.html
	myPage->setAddress("index.html");
        site.addPage(myPage);
        RootWContent* myContent;
        
        // Grab a list of layers from teh tracker object
        LayerVector& layerSet = tracker.getLayers();
        double nMB = tracker.getNMB();
        ModuleVector& endcapSample = tracker.getEndcapSample();
        
        
        //********************************//
        //*                              *//
        //*       Layers and disks       *//
        //*                              *//
        //********************************//
        myContent = new RootWContent("Layers and disks");
        myPage->addContent(myContent);
        RootWTable* layerTable = new RootWTable(); myContent->addItem(layerTable);
        RootWTable* diskTable = new RootWTable(); myContent->addItem(diskTable);
        RootWTable* ringTable = new RootWTable(); myContent->addItem(ringTable);
        
        
        std::vector<std::string> layerNames;
        std::vector<double> layerRho;
        std::vector<std::string> diskNames;
        std::vector<double> diskZ;
        std::vector<std::string> ringNames;
        std::vector<double> ringRho1;
        std::vector<double> ringRho2;
        
        Layer* aLayer;
        BarrelLayer* aBarrelLayer;
        EndcapLayer* anEndcapDisk;
        double aRingRho;
        
        layerTable->setContent(0, 0, "Layer");
        layerTable->setContent(1, 0, "r");
        diskTable->setContent(0, 0, "Disk");
        diskTable->setContent(1, 0, "z");
        ringTable->setContent(0, 0, "Ring");
        ringTable->setContent(1, 0, "r"+subStart+"min"+subEnd);
        ringTable->setContent(2, 0, "r"+subStart+"max"+subEnd);
        
        // Build the module type maps
        // with a pointer to a sample module
        // Build the layer summary BTW
        int nBarrelLayers=0;
        int nDisks=0;
        for (layIt=layerSet.begin(); layIt!=layerSet.end(); layIt++) {
            aLayer = (*layIt);
            if ( (aBarrelLayer=dynamic_cast<BarrelLayer*>(aLayer)) ) {
                if (aBarrelLayer->getMaxZ(+1)>0) {
                    ++nBarrelLayers;
                    //std::cerr << "Layer number " << nBarrelLayers << std::endl;
                    layerTable->setContent(0, nBarrelLayers, aBarrelLayer->getName());
                    layerTable->setContent(1, nBarrelLayers, aBarrelLayer->getAverageRadius(), coordPrecision);
                }
            }
            if ( (anEndcapDisk=dynamic_cast<EndcapLayer*>(aLayer)) ) {
                if (anEndcapDisk->getAverageZ()>0) {
                    ++nDisks;
                    diskTable->setContent(0, nDisks, anEndcapDisk->getName());
                    diskTable->setContent(1, nDisks, anEndcapDisk->getAverageZ(), coordPrecision);
                }
            }
            aLay = (*layIt)->getModuleVector();
            for (modIt=aLay->begin(); modIt!=aLay->end(); modIt++) {
                aSensorTag=(*modIt)->getSensorTag();
                typeMapCount[aSensorTag]++;
                typeMapCountChan[aSensorTag]+=(*modIt)->getNChannels();
                if (((*modIt)->getOccupancyPerEvent()*nMB)>typeMapMaxOccupancy[aSensorTag]) {
                    typeMapMaxOccupancy[aSensorTag]=(*modIt)->getOccupancyPerEvent()*nMB;
                }
                typeMapAveOccupancy[aSensorTag]+=(*modIt)->getOccupancyPerEvent()*nMB;
		typeMapAveRphiResolution[aSensorTag]+=(*modIt)->getResolutionRphi();
		typeMapAveYResolution[aSensorTag]+=(*modIt)->getResolutionY();
                totCountMod++;
                totCountSens+=(*modIt)->getNFaces();
                if ((*modIt)->getReadoutType()==Module::Strip) {
                    totChannelStrips+=(*modIt)->getNChannels();
                    totAreaStrips+=(*modIt)->getArea()*(*modIt)->getNFaces();
                }
                if ((*modIt)->getReadoutType()==Module::Pt) {
                    totChannelPts+=(*modIt)->getNChannels();
                    totAreaPts+=(*modIt)->getArea()*(*modIt)->getNFaces();
                }
                if (typeMap.find(aSensorTag)==typeMap.end()){
                    // We have a new sensor geometry
                    typeMap[aSensorTag]=(*modIt);
                }
            }
        }
        
        EndcapModule* anEC;
        int aRing;
        // Look into the endcap sample in order to indentify and measure rings
        for (ModuleVector::iterator moduleIt=endcapSample.begin(); moduleIt!=endcapSample.end(); moduleIt++) {
            if ( (anEC=dynamic_cast<EndcapModule*>(*moduleIt)) ) {
                aRing=anEC->getRing();
                if (ringTypeMap.find(aRing)==ringTypeMap.end()){
                    // We have a new sensor geometry
                    ringTypeMap[aRing]=(*moduleIt);
                }
            } else {
                std::cout << "ERROR: found a non-Endcap module in the map of ring types" << std::endl;
            }
        }
        
        std::ostringstream myName;
        for (std::map<int, Module*>::iterator typeIt = ringTypeMap.begin();
        typeIt!=ringTypeMap.end(); typeIt++) {
            if ( (anEC=dynamic_cast<EndcapModule*>((*typeIt).second)) ) {
                aRing=(*typeIt).first;
                ringTable->setContent(0, aRing, aRing);
                ringTable->setContent(1, aRing, aRingRho = anEC->getDist(), coordPrecision);
                ringTable->setContent(2, aRing, aRingRho = anEC->getDist()+anEC->getHeight(), coordPrecision);
            } else {
                std::cout << "ERROR: found a non-Endcap module in the map of ring types (twice...)" << std::endl;
            }
        }
        
        
        // A bit of variables
        std::vector<std::string> names;
        std::vector<std::string> tags;
        std::vector<std::string> types;
        std::vector<std::string> areastrips;
        std::vector<std::string> areapts;
        std::vector<std::string> occupancies;
	std::vector<std::string> rphiresolutions;
	std::vector<std::string> yresolutions;
        std::vector<std::string> pitchpairs;
        std::vector<std::string> striplengths;
        std::vector<std::string> segments;
        std::vector<std::string> nstrips;
        std::vector<std::string> numbermods;
        std::vector<std::string> numbersens;
        std::vector<std::string> channelstrips;
        std::vector<std::string> channelpts;
        std::vector<std::string> powers;
        std::vector<std::string> costs;
        
        double totalPower=0;
        double totalCost=0;
        
        std::ostringstream aName;
        std::ostringstream aTag;
        std::ostringstream aType;
        std::ostringstream anArea;
        std::ostringstream anOccupancy;
	std::ostringstream anRphiResolution;
	std::ostringstream aYResolution;
        std::ostringstream aPitchPair;
        std::ostringstream aStripLength;
        std::ostringstream aSegment;
        std::ostringstream anNstrips;
        std::ostringstream aNumberMod;
        std::ostringstream aNumberSens;
        std::ostringstream aChannel;
        std::ostringstream aPower;
        std::ostringstream aCost;
        int barrelCount=0;
        int endcapCount=0;
        Module* aModule;
        
        
        
        //********************************//
        //*                              *//
        //*       Modules                *//
        //*                              *//
        //********************************//
        myContent = new RootWContent("Modules", false);
        myPage->addContent(myContent);
        RootWTable* moduleTable = new RootWTable(); myContent->addItem(moduleTable);
        
        static const int tagRow = 1;
        static const int typeRow = 2;
        static const int areastripRow = 3;
        static const int areaptRow = 4;
        static const int occupancyRow = 5;
	static const int rphiResolutionRow = 6;
	static const int yResolutionRow = 7;
        static const int pitchpairsRow = 8;
        static const int striplengthRow = 9;
        static const int segmentsRow = 10;
        static const int nstripsRow = 11;
        static const int numbermodsRow = 12;
        static const int numbersensRow = 13;
        static const int channelstripRow = 14;
        static const int channelptRow = 15;
        static const int powerRow = 16;
        static const int costRow = 17;
        
        // Row names
        moduleTable->setContent(tagRow, 0, "Tag");
        moduleTable->setContent(typeRow, 0, "Type");
        moduleTable->setContent(areastripRow, 0, "Area (mm"+superStart+"2"+superEnd+")");
        moduleTable->setContent(areaptRow, 0, "Area (mm"+superStart+"2"+superEnd+")");
        moduleTable->setContent(occupancyRow, 0, "Occup (max/av)");
        moduleTable->setContent(rphiResolutionRow, 0, "R/Phi resolution (um)");
        moduleTable->setContent(yResolutionRow, 0, "Y resolution (um)");
        moduleTable->setContent(pitchpairsRow, 0, "Pitch (min/max)");
        moduleTable->setContent(striplengthRow, 0, "Strip length");
        moduleTable->setContent(segmentsRow, 0, "Segments x Chips");
        moduleTable->setContent(nstripsRow, 0, "Chan/Sensor");
        moduleTable->setContent(numbermodsRow, 0, "N. mod");
        moduleTable->setContent(numbersensRow, 0, "N. sens");
        moduleTable->setContent(channelstripRow, 0, "Channels (M)");
        moduleTable->setContent(channelptRow, 0, "Channels (M)");
        moduleTable->setContent(powerRow, 0, "Power (kW)");
        moduleTable->setContent(costRow, 0, "Cost (MCHF)");
        
        int loPitch;
        int hiPitch;
        
        // Summary cycle: prepares the rows cell by cell
        int iType=0;
        for (typeMapIt=typeMap.begin(); typeMapIt!=typeMap.end(); typeMapIt++) {
            ++iType;
            // Name
            aName.str("");
            aModule=(*typeMapIt).second;
            if (dynamic_cast<BarrelModule*>(aModule)) {
                aName << std::dec << "B" << subStart << ++barrelCount << subEnd;
            }
            if (dynamic_cast<EndcapModule*>(aModule)) {
                aName << std::dec << "E" << subStart << ++endcapCount << subEnd;
            }
            // Tag
            aTag.str("");
            aTag << smallStart << aModule->getTag() << smallEnd;
            // Type
            aType.str("");
            aType << (*typeMapIt).second->getType();
            // Area
            anArea.str("");
            anArea << std::dec << std::fixed << std::setprecision(areaPrecision) << (*typeMapIt).second->getArea();
            if ((*typeMapIt).second->getArea()<0) { anArea << "XXX"; }
            // Occupancy
            anOccupancy.str("");
            anOccupancy << std::dec << std::fixed << std::setprecision(occupancyPrecision) <<  typeMapMaxOccupancy[(*typeMapIt).first]*100<< "/" <<typeMapAveOccupancy[(*typeMapIt).first]*100/typeMapCount[(*typeMapIt).first] ; // Percentage
	    // RphiResolution
	    anRphiResolution.str("");
	    anRphiResolution << std::dec << std::fixed << std::setprecision(rphiResolutionPrecision) << typeMapAveRphiResolution[(*typeMapIt).first] / typeMapCount[(*typeMapIt).first] * 1000; // mm -> um
	    // YResolution
	    aYResolution.str("");
	    aYResolution << std::dec << std::fixed << std::setprecision(rphiResolutionPrecision) << typeMapAveYResolution[(*typeMapIt).first] / typeMapCount[(*typeMapIt).first] * 1000; // mm -> um
            // Pitches
	    aPitchPair.str("");
            loPitch=int((*typeMapIt).second->getLowPitch()*1e3);
            hiPitch=int((*typeMapIt).second->getHighPitch()*1e3);
            if (loPitch==hiPitch) {
                aPitchPair << std::dec << std::fixed << std::setprecision(pitchPrecision) << loPitch;
            } else {
                aPitchPair << std::dec << std::fixed << std::setprecision(pitchPrecision)<< loPitch
                << "/" << std::fixed << std::setprecision(pitchPrecision) << hiPitch;
            }
            // Strip Lengths
            aStripLength.str("");
            aStripLength << std::fixed << std::setprecision(stripLengthPrecision)
            << (*typeMapIt).second->getHeight()/(*typeMapIt).second->getNSegments();
            // Segments
            aSegment.str("");
            aSegment << std::dec << (*typeMapIt).second->getNSegments()
            << "x" << int( (*typeMapIt).second->getNStripAcross() / 128. );
            // Nstrips
            anNstrips.str("");
            anNstrips << std::dec << (*typeMapIt).second->getNChannelsPerFace();
            // Number Mod
            aNumberMod.str("");
            aNumberMod << std::dec << typeMapCount[(*typeMapIt).first];
            // Number Sensor
            aNumberSens.str("");
            aNumberSens << std::dec << typeMapCount[(*typeMapIt).first]*((*typeMapIt).second->getNFaces());
            // Channels
            aChannel.str("");
            aChannel << std::fixed << std::setprecision(millionChannelPrecision)
            << typeMapCountChan[(*typeMapIt).first] / 1e6 ;
            // Power and cost
            aPower.str("");
            aCost.str("");
            aPower << std::fixed << std::setprecision(powerPrecision) <<
            typeMapCountChan[(*typeMapIt).first] *           // number of channels in type
            1e-3 *                                           // conversion from W to kW
            tracker.getPower((*typeMapIt).second->getReadoutType()); // power consumption in W/channel
            totalPower += typeMapCountChan[(*typeMapIt).first] * 1e-3 * tracker.getPower((*typeMapIt).second->getReadoutType());
            aCost  << std::fixed << std::setprecision(costPrecision) <<
            (*typeMapIt).second->getArea() * 1e-2 *          // area in cm^2
            (*typeMapIt).second->getNFaces() *               // number of faces
            tracker.getCost((*typeMapIt).second->getReadoutType()) * // price in CHF*cm^-2
            1e-6 *                                           // conversion CHF-> MCHF
            typeMapCount[(*typeMapIt).first];                // Number of modules
            totalCost +=(*typeMapIt).second->getArea() * 1e-2 * (*typeMapIt).second->getNFaces() * tracker.getCost((*typeMapIt).second->getReadoutType()) * 1e-6 * typeMapCount[(*typeMapIt).first];
            
            
            moduleTable->setContent(0, iType, aName.str());
            moduleTable->setContent(tagRow, iType, aTag.str());
            moduleTable->setContent(typeRow, iType, aType.str());
            moduleTable->setContent(occupancyRow, iType, anOccupancy.str());
            moduleTable->setContent(rphiResolutionRow, iType, anRphiResolution.str());
            moduleTable->setContent(yResolutionRow, iType, aYResolution.str());
            moduleTable->setContent(pitchpairsRow, iType, aPitchPair.str());
            moduleTable->setContent(striplengthRow, iType, aStripLength.str());
            moduleTable->setContent(segmentsRow, iType, aSegment.str());
            moduleTable->setContent(nstripsRow, iType, anNstrips.str());
            moduleTable->setContent(numbermodsRow, iType, aNumberMod.str());
            moduleTable->setContent(numbersensRow, iType, aNumberSens.str());
            moduleTable->setContent(powerRow, iType, aPower.str());
            moduleTable->setContent(costRow, iType, aCost.str());
            
            if ((*typeMapIt).second->getReadoutType()==Module::Strip) {
                moduleTable->setContent(channelstripRow, iType, aChannel.str());
                moduleTable->setContent(areastripRow, iType, anArea.str());
                moduleTable->setContent(channelptRow, iType, "--");
                moduleTable->setContent(areaptRow, iType, "--");
            } else {
                moduleTable->setContent(channelstripRow, iType, "--");
                moduleTable->setContent(areastripRow, iType, "--");
                moduleTable->setContent(channelptRow, iType, aChannel.str());
                moduleTable->setContent(areaptRow, iType, anArea.str());
            }
        }
        
        // Score totals
        ++iType;
        moduleTable->setContent(0, iType, "Total");
        moduleTable->setContent(tagRow, iType, "");
        moduleTable->setContent(typeRow, iType, "");
        anArea.str("");
        anArea << emphStart << std::fixed << std::setprecision(areaPrecision) << totAreaStrips/1e6
        << "(m" << superStart << "2" << superEnd << ")" << emphEnd;
        moduleTable->setContent(areastripRow, iType, anArea.str());
        anArea.str("");
        anArea << emphStart << std::fixed << std::setprecision(areaPrecision) << totAreaPts/1e6
        << "(m" << superStart << "2" << superEnd << ")" << emphEnd;
        moduleTable->setContent(areaptRow, iType, anArea.str());
        moduleTable->setContent(occupancyRow, iType, "");
        moduleTable->setContent(rphiResolutionRow, iType, "");
        moduleTable->setContent(yResolutionRow, iType, "");
        moduleTable->setContent(pitchpairsRow, iType, "");
        moduleTable->setContent(striplengthRow, iType, "");
        moduleTable->setContent(segmentsRow, iType, "");
        moduleTable->setContent(nstripsRow, iType, "");
        aNumberMod.str("");
        aNumberMod << emphStart << totCountMod << emphEnd;
        aNumberSens.str("");
        aNumberSens << emphStart << totCountSens << emphEnd;
        moduleTable->setContent(numbermodsRow, iType, aNumberMod.str());
        moduleTable->setContent(numbersensRow, iType, aNumberSens.str());
        aChannel.str("");
        aChannel << emphStart << std::fixed
        << std::setprecision(millionChannelPrecision)
        << totChannelStrips / 1e6 << emphEnd;
        moduleTable->setContent(channelstripRow, iType, aChannel.str());
        aChannel.str("");
        aChannel << emphStart << std::fixed
        << std::setprecision(millionChannelPrecision)
        << totChannelPts / 1e6 << emphEnd;
        moduleTable->setContent(channelptRow, iType, aChannel.str());
        aPower.str("");
        aCost.str("");
        aPower   << std::fixed << std::setprecision(powerPrecision) << totalPower;
        aCost    << std::fixed << std::setprecision(costPrecision) << totalCost;
        moduleTable->setContent(powerRow, iType, aPower.str());
        moduleTable->setContent(costRow, iType, aCost.str());
        
        //********************************//
        //*                              *//
        //*       Plots                  *//
        //*                              *//
        //********************************//
        RootWImage* myImage;
        TCanvas *summaryCanvas = NULL;
        TCanvas *YZCanvas = NULL;
        TCanvas *XYCanvas = NULL;
        TCanvas *XYCanvasEC = NULL;
        TCanvas *myCanvas = NULL;
        //createSummaryCanvas(tracker.getMaxL(), tracker.getMaxR(), analyzer, summaryCanvas, YZCanvas, XYCanvas, XYCanvasEC);
	createSummaryCanvas(tracker.getMaxL(), tracker.getMaxR(), analyzer, YZCanvas, XYCanvas, XYCanvasEC);
	
        
        //TVirtualPad* myPad;
        myContent = new RootWContent("Plots");
        myPage->addContent(myContent);
        
        if (summaryCanvas) {
            myImage = new RootWImage(summaryCanvas, 600, 600);
            myImage->setComment("Tracker summary: modules position in XY (endcap and barrel), YZ and number of hits vs. eta");
            myContent->addItem(myImage);
        }
        
        if (YZCanvas) {
            myImage = new RootWImage(YZCanvas, 600, 600);
            myImage->setComment("YZ Section of the tracker barrel");
            myContent->addItem(myImage);
        }
        if (XYCanvas) {
            myImage = new RootWImage(XYCanvas, 600, 600);
            myImage->setComment("XY Section of the tracker barrel");
            myContent->addItem(myImage);
        }
        if (XYCanvasEC) {
            myImage = new RootWImage(XYCanvasEC, 600, 600);
            myImage->setComment("XY Projection of the tracker endcap(s)");
            myContent->addItem(myImage);
        }

        
        /*
         * myCanvas = new TCanvas("XYViewBarrel", "XYViewBarrel", 600, 600);
         * myCanvas->cd();
         * myPad = summaryCanvas->GetPad(padXY);
         * if (myPad) {
         * myPad->DrawClonePad();
         * myImage = new RootWImage(myCanvas, 600, 600);
         * myImage->setComment("XY Section of the tracker barrel");
         * myContent->addItem(myImage);
         * }
         *
         * myCanvas = new TCanvas("XYViewEndcap", "XYViewEndcap", 600, 600);
         * myCanvas->cd();
         * myPad = summaryCanvas->GetPad(padEC);
         * if (myPad) {
         * myPad->DrawClonePad();
         * myImage = new RootWImage(myCanvas, 600, 600);
         * myImage->setComment("XY View of the tracker endcap");
         * myContent->addItem(myImage);
         * }
	*/

	// Eta profile big plot
        myCanvas = new TCanvas("EtaProfile", "Eta profile", 600, 600);
	drawEtaProfiles(*myCanvas, analyzer);
        myImage = new RootWImage(myCanvas, 600, 600);
        myImage->setComment("Hit coverage in eta");
        myContent->addItem(myImage);
        
        TCanvas* hitMapCanvas = new TCanvas("hitmapcanvas", "Hit Map", 600, 600);
        int prevStat = gStyle->GetOptStat();
        gStyle->SetOptStat(0);
        hitMapCanvas->cd();
        //gStyle->SetPalette(1);
        hitMapCanvas->SetFillColor(color_plot_background);
        hitMapCanvas->SetBorderMode(0);
        hitMapCanvas->SetBorderSize(0);
        analyzer.getMapPhiEta().Draw("colz");
        hitMapCanvas->Modified();
        gStyle->SetOptStat(prevStat);
        myImage = new RootWImage(hitMapCanvas, 600, 600);
        myImage->setComment("Hit coverage in eta, phi");
        myContent->addItem(myImage);
        
        // TODO: make this meaningful!
        return true;
        
    }

  // Draws all the profile plots present in the analyzer into the given TCanvas
  // @param myPad the target TPad
  // @param analyzer the plot data container
  bool Vizard::drawEtaProfiles(TVirtualPad& myPad, Analyzer& analyzer) {
    myPad.cd();
    myPad.SetFillColor(color_plot_background);
    TProfile& totalEtaProfile = analyzer.getTotalEtaProfile();
    std::vector<TProfile>& etaProfiles = analyzer.getTypeEtaProfiles();
    std::vector<TProfile>::iterator etaProfileIterator;

    totalEtaProfile.Draw();
    for (etaProfileIterator=etaProfiles.begin();
	 etaProfileIterator!=etaProfiles.end();
	 ++etaProfileIterator) {
      (*etaProfileIterator).Draw("same");
    }
    return true; // TODO: make this meaningful
  }
  // Draws all the profile plots present in the analyzer into the given TCanvas
  // @param myCanvas the target TCanvas
  // @param analyzer the plot data container
  bool Vizard::drawEtaProfiles(TCanvas& myCanvas, Analyzer& analyzer) {
    TVirtualPad* myVirtualPad = myCanvas.GetPad(0);
    if (!myVirtualPad) return false;
    return drawEtaProfiles(*myVirtualPad, analyzer);
  }
    
  bool Vizard::additionalInfoSite(std::string& geomfile, std::string& settingsfile,
				  std::string& matfile, std::string& pixmatfile,
				  Analyzer& analyzer, Tracker& tracker, RootWSite& site) {
        RootWPage* myPage = new RootWPage("Info");
        myPage->setAddress("info.html");
        site.addPage(myPage);
        RootWContent *simulationContent, *filesContent;
        RootWBinaryFile* myBinaryFile;
        std::string trackerName = tracker.getName();
        
        int materialTracksUsed = analyzer.getMaterialTracksUsed();
        int geometryTracksUsed = analyzer.getGeometryTracksUsed();
        
        //********************************//
        //*                              *//
        //*  Simulation and files        *//
        //*                              *//
        //********************************//
        // (also todo: handle this properly: with a not-hardcoded model)
        simulationContent = new RootWContent("Simulation parameters");
        myPage->addContent(simulationContent);
        filesContent = new RootWContent("Geometry files");
        myPage->addContent(filesContent);
        
        if (geomfile!="") {
            std::string destinationFilename = trackerName + ".cfg";
            myBinaryFile = new RootWBinaryFile(destinationFilename, "Geometry configuration file", geomfile);
            simulationContent->addItem(myBinaryFile);
        }
        if (settingsfile!="") {
            std::string destinationFilename = trackerName + "_Types.cfg";
            myBinaryFile = new RootWBinaryFile(destinationFilename, "Module types configuration file", settingsfile);
            simulationContent->addItem(myBinaryFile);
        }
        if (matfile!="") {
            std::string destinationFilename = trackerName + "_Materials.cfg";
            myBinaryFile = new RootWBinaryFile(destinationFilename, "Material configuration file (outer)", matfile);
            simulationContent->addItem(myBinaryFile);
        }
	if (pixmatfile!="") {
            std::string destinationFilename = trackerName + "_Materials.cfg.pix";
            myBinaryFile = new RootWBinaryFile(destinationFilename, "Material configuration file (pixel)", pixmatfile);
            simulationContent->addItem(myBinaryFile);
	}
        
        RootWInfo* myInfo;
        myInfo = new RootWInfo("Minimum bias per bunch crossing");
        myInfo->setValue(tracker.getNMB(), minimumBiasPrecision);
        simulationContent->addItem(myInfo);
        myInfo = new RootWInfo("Number of tracks used for material");
        myInfo->setValue(materialTracksUsed);
        simulationContent->addItem(myInfo);
        myInfo = new RootWInfo("Number of tracks used for geometry");
        myInfo->setValue(geometryTracksUsed);
        simulationContent->addItem(myInfo);
        
        ostringstream barrelModuleCoordinates, endcapModuleCoordinates;
        RootWTextFile* myTextFile;
        tracker.printBarrelModuleZ(barrelModuleCoordinates);
        tracker.printEndcapModuleRPhiZ(endcapModuleCoordinates);
        // Barrel coordinates
        myTextFile = new RootWTextFile("barrelCoordinates.csv", "Barrel modules coordinate file");
        myTextFile->addText(barrelModuleCoordinates.str());
        filesContent->addItem(myTextFile);
        // Endcap coordinates
        myTextFile = new RootWTextFile("endcapCoordinates.csv", "Endcap modules coordinate file");
        myTextFile->addText(endcapModuleCoordinates.str());
        filesContent->addItem(myTextFile);
        
        // TODO: make an object that handles this properly:
        RootWTable* myTable = new RootWTable();
        myTable->setContent(1, 0, "CHF/cm"+superStart+"2"+superEnd);
        myTable->setContent(2, 0, "mW/channel");
        myTable->setContent(0, 1, "Pt modules");
        myTable->setContent(0, 2, "Strip modules");
        myTable->setContent(1, 1, tracker.getCost(Module::Pt), costPerUnitPrecision);
        myTable->setContent(1, 2, tracker.getCost(Module::Strip), costPerUnitPrecision);
        myTable->setContent(2, 1, tracker.getPower(Module::Pt)*1e3, powerPerUnitPrecision);
        myTable->setContent(2, 2, tracker.getPower(Module::Strip)*1e3, powerPerUnitPrecision);
        simulationContent->addItem(myTable);
        
        return true; // TODO: make this meaningful
    }
    
    
    bool Vizard::bandwidthSummary(Analyzer& analyzer, Tracker& tracker, RootWSite& site) {
        RootWPage* myPage = new RootWPage("Band width");
        myPage->setAddress("bandwidth.html");
        site.addPage(myPage);
        RootWContent* myContent;
        
        //********************************//
        //*                              *//
        //*       Bandwidth              *//
        //*                              *//
        //********************************//
        // (also todo: handle this properly: with a not-hardcoded model)
        myContent = new RootWContent("Distributions and models");
        myPage->addContent(myContent);
        TCanvas* bandWidthCanvas = new TCanvas("ModuleBandwidthC", "Modules needed bandwidthC", 600, 400); // TODO: put all these numbers somewhere
        TCanvas* moduleHitCanvas = new TCanvas("ModuleHitC", "Module hit countC", 600, 400);
        bandWidthCanvas->SetLogy(1);
        moduleHitCanvas->SetLogy(1);
        
        bandWidthCanvas->cd();
        TH1D& bandwidthDistribution = analyzer.getBandwidthDistribution();
        TH1D& bandwidthDistributionSparsified = analyzer.getBandwidthDistributionSparsified();
        bandwidthDistribution.Draw();
        bandwidthDistributionSparsified.Draw("same");
        TLegend* myLegend = new TLegend(0.75, 0.5, 1, .75);
        myLegend->AddEntry(&bandwidthDistribution, "Unsparsified", "l");
        myLegend->AddEntry(&bandwidthDistributionSparsified, "Sparsified", "l");
        myLegend->Draw();
        RootWImage* myImage = new RootWImage(bandWidthCanvas, 600, 600);
        myImage->setComment("Module bandwidth distribution in the sparsified and unsparsified model");
        myContent->addItem(myImage);
        
        moduleHitCanvas->cd();
        TH1D& chanHitDistribution = analyzer.getChanHitDistribution();
        chanHitDistribution.Draw();
        myImage = new RootWImage(moduleHitCanvas, 600, 600);
        myImage->setComment("Number of hits per module per BX distribution");
        myContent->addItem(myImage);
        
        RootWText* myDescription = new RootWText();
        myContent->addItem(myDescription);
        myDescription->addText( "Bandwidth useage estimate:<br/>");
        myDescription->addText( "(Pt modules: ignored)<br/>");
        myDescription->addText( "Sparsified (binary) bits/event: 23 bits/chip + 9 bit/hit<br/>");
        myDescription->addText( "Unsparsified (binary) bits/event: 16 bits/chip + 1 bit/channel<br/>");
        ostringstream aStringStream; aStringStream.str("100 kHz trigger, "); aStringStream << tracker.getNMB();
        aStringStream <<" minimum bias events assumed</br>";
        myDescription->addText( aStringStream.str() );
                
        return true;
    }
    
    bool Vizard::errorSummary(Analyzer& a, RootWSite& site) {
        //********************************//
        //*                              *//
        //*    Resolution estimate       *//
        //*                              *//
        //********************************//
        
        // Here you should check if the TGraph
        // list is empty => maybe not?
        if (!(a.getRhoProfiles(false).empty() && a.getDProfiles(false).empty() && a.getPhiProfiles(false).empty())) {
            // Create a page for the errors
            RootWPage& myPage = site.addPage("Resolution");
            myPage.setAddress("errors.html");
            
            // Create the contents
            RootWContent& resolutionContent = myPage.addContent("Track resolution");
            RootWContent& idealResolutionContent = myPage.addContent("Track resolution (without material)");
	    
            for (int scenario=0; scenario<2; ++scenario) {
              bool idealMaterial;
              RootWContent* myContent;
              if (scenario==0) {
                idealMaterial=false;
                myContent = &resolutionContent;
              } else {
                idealMaterial=true;
                myContent = &idealResolutionContent;
              }
	      
	      TCanvas momentumCanvas;
	      TCanvas distanceCanvas;
	      TCanvas angleCanvas;
	      TCanvas ctgThetaCanvas;
	      TCanvas z0Canvas;
	      momentumCanvas.SetGrid(1,1);
	      distanceCanvas.SetGrid(1,1);
	      angleCanvas.SetGrid(1,1);
	      ctgThetaCanvas.SetGrid(1,1);
	      z0Canvas.SetGrid(1,1);
	      std::string plotOption = "Ap";
	      std::map<double, TGraph>::iterator g_iter, g_guard;
	      // momentum canvas loop
	      int myColor=0;
	      g_guard = a.getRhoProfiles(idealMaterial).end();
	      gStyle->SetGridStyle(style_grid);
	      gStyle->SetGridColor(color_hard_grid);
	      for (g_iter = a.getRhoProfiles(idealMaterial).begin(); g_iter != g_guard; g_iter++) {
                TGraph& momentumGraph = g_iter->second;
		if (idealMaterial) {
		  momentumGraph.SetMinimum(1E-5*100);
		  momentumGraph.SetMaximum(.11*100);		  
		} else {
		  momentumGraph.SetMinimum(4E-3*100);
		  momentumGraph.SetMaximum(.11*100);
		}
                momentumGraph.GetXaxis()->SetLimits(0, 2.4);
		momentumCanvas.SetLogy();
		momentumGraph.SetLineColor(momentumColor(myColor));
		momentumGraph.SetMarkerColor(momentumColor(myColor));
                myColor++;
		momentumGraph.SetMarkerStyle(8);
                momentumCanvas.cd();
                momentumCanvas.SetFillColor(color_plot_background);
                momentumGraph.Draw(plotOption.c_str());
		plotOption = "p same";
	      }
	      plotOption = "Ap";
	      myColor=0;
	      // distance canvas loop
	      g_guard = a.getDProfiles(idealMaterial).end();
	      for (g_iter = a.getDProfiles(idealMaterial).begin(); g_iter != g_guard; g_iter++) {
                TGraph& distanceGraph = g_iter->second;
		if (idealMaterial) {
		  distanceGraph.SetMinimum(4*1e-4);
		  distanceGraph.SetMaximum(4E2*1e-4);
		} else {
		  distanceGraph.SetMinimum(4*1e-4);
		  distanceGraph.SetMaximum(4E2*1e-4);
		}
                distanceGraph.GetXaxis()->SetLimits(0, 2.4);
		distanceCanvas.SetLogy();
		distanceGraph.SetLineColor(momentumColor(myColor));
		distanceGraph.SetMarkerColor(momentumColor(myColor));
                myColor++;
		distanceGraph.SetMarkerStyle(8);
                distanceCanvas.cd();
                distanceCanvas.SetFillColor(color_plot_background);
                distanceGraph.Draw(plotOption.c_str());
		plotOption = "p same";
	      }
	      plotOption = "Ap";
	      myColor=0;
	      // angle canvas loop
	      g_guard = a.getPhiProfiles(idealMaterial).end();
	      for (g_iter = a.getPhiProfiles(idealMaterial).begin(); g_iter != g_guard; g_iter++) {
                TGraph& angleGraph = g_iter->second;
		if (idealMaterial) {
		  angleGraph.SetMinimum(1E-5);
		  angleGraph.SetMaximum(0.01);
		} else {
		  angleGraph.SetMinimum(1E-5);
		  angleGraph.SetMaximum(0.01);
		}
                angleGraph.GetXaxis()->SetLimits(0, 2.4);
                angleCanvas.SetLogy();
		angleGraph.SetLineColor(momentumColor(myColor));
		angleGraph.SetMarkerColor(momentumColor(myColor));
                myColor++;
		angleGraph.SetMarkerStyle(8);
                angleCanvas.cd();
                angleCanvas.SetFillColor(color_plot_background);
                angleGraph.Draw(plotOption.c_str());
		plotOption = "p same";
	      }
	      plotOption = "Ap";
	      myColor=0;
	      // ctgTheta canvas loop
	      g_guard = a.getCtgThetaProfiles(idealMaterial).end();
	      for (g_iter = a.getCtgThetaProfiles(idealMaterial).begin(); g_iter != g_guard; g_iter++) {
                TGraph& ctgThetaGraph = g_iter->second;
		ctgThetaGraph.SetMinimum(1E-5);
		ctgThetaGraph.SetMaximum(0.1);
                ctgThetaGraph.GetXaxis()->SetLimits(0, 2.4);
                ctgThetaCanvas.SetLogy();
		ctgThetaGraph.SetLineColor(momentumColor(myColor));
		ctgThetaGraph.SetMarkerColor(momentumColor(myColor));
                myColor++;
		ctgThetaGraph.SetMarkerStyle(8);
                ctgThetaCanvas.cd();
                ctgThetaCanvas.SetFillColor(color_plot_background);
                ctgThetaGraph.Draw(plotOption.c_str());
		plotOption = "p same";
	      }
	      plotOption = "Ap";
	      myColor=0;
	      // z0 canvas loop
	      g_guard = a.getZ0Profiles(idealMaterial).end();
	      for (g_iter = a.getZ0Profiles(idealMaterial).begin(); g_iter != g_guard; g_iter++) {
                TGraph& z0Graph = g_iter->second;
		z0Graph.SetMinimum(1E-5);
		z0Graph.SetMaximum(1);
                z0Graph.GetXaxis()->SetLimits(0, 2.4);
                z0Canvas.SetLogy();
		z0Graph.SetLineColor(momentumColor(myColor));
		z0Graph.SetMarkerColor(momentumColor(myColor));
                myColor++;
		z0Graph.SetMarkerStyle(8);
                z0Canvas.cd();
                z0Canvas.SetFillColor(color_plot_background);
                z0Graph.Draw(plotOption.c_str());
		plotOption = "p same";
	      }
	      RootWImage& momentumImage = myContent->addImage(momentumCanvas, 600, 600);
	      momentumImage.setComment("Momentum resolution vs. eta");
	      momentumImage.setName("ptres");
	      RootWImage& distanceImage = myContent->addImage(distanceCanvas, 600, 600);
	      distanceImage.setComment("Distance of closest approach resolution vs. eta");
	      distanceImage.setName("dxyres");
	      RootWImage& angleImage = myContent->addImage(angleCanvas, 600, 600);
	      angleImage.setComment("Angle resolution vs. eta");
	      angleImage.setName("phires");
	      RootWImage& ctgThetaImage = myContent->addImage(ctgThetaCanvas, 600, 600);
	      ctgThetaImage.setComment("CtgTheta resolution vs. eta");
	      ctgThetaImage.setName("cotThetares");
	      RootWImage& z0Image = myContent->addImage(z0Canvas, 600, 600);
	      z0Image.setComment("z0 resolution vs. eta");
	      z0Image.setName("dzres");
            }

	    // Check that the ideal and real have the same pts
	    // Otherwise the table cannot be prepared

            RootWContent& summaryContent = myPage.addContent("Summary", false);
	    RootWTable& cutsTable = summaryContent.addTable();
	    std::vector<std::string> plotNames;
	    std::map<std::string, RootWTable*> tableMap;
	    std::map<std::string, RootWTable*>::iterator tableMapIt;
	    plotNames.push_back("pt");
	    plotNames.push_back("d");
	    plotNames.push_back("phi");
	    plotNames.push_back("ctg(theta)");
	    plotNames.push_back("z0");
	    for (std::vector<std::string>::iterator it=plotNames.begin();
		 it!=plotNames.end(); ++it) {
	      tableMap[(*it)] = &(summaryContent.addTable());
	      tableMap[(*it)]->setContent(0,0,(*it));
	    }

	    // Prepare the cuts for the averages
	    std::vector<std::string> cutNames;
	    std::vector<double> cuts;
	    ostringstream label;
	    std::string name;	   
	    RootWTable* myTable;

	    cuts.push_back(0.05);
	    cuts.push_back(1);
	    cuts.push_back(1.5);
	    cuts.push_back(2.4);
	    cutNames.push_back("C");
	    cutNames.push_back("I");
	    cutNames.push_back("F");
	    unsigned int nCuts = cutNames.size();
	    
	    // Table explaining the cuts
	    cutsTable.setContent(0,0,"Region");
	    cutsTable.setContent(1,0,"etaMin");
	    cutsTable.setContent(2,0,"etaMax");
	    myTable = &cutsTable;
	    for (unsigned int iBorder=0; iBorder<cuts.size()-1; ++iBorder) {
	      myTable->setContent(0,iBorder+1,cutNames[iBorder]);
	      label.str(""); label << cuts[iBorder];
	      myTable->setContent(1,iBorder+1,label.str());
	      label.str(""); label << cuts[iBorder+1];
	      myTable->setContent(2,iBorder+1,label.str());
	    }
	    
	    std::map<graphIndex, TGraph*> myPlotMap;
	    graphIndex myIndex;

	    fillPlotMap(plotNames[0], myPlotMap, &a, &Analyzer::getRhoProfiles);
	    fillPlotMap(plotNames[1], myPlotMap, &a, &Analyzer::getDProfiles);
	    fillPlotMap(plotNames[2], myPlotMap, &a, &Analyzer::getPhiProfiles);
	    fillPlotMap(plotNames[3], myPlotMap, &a, &Analyzer::getCtgThetaProfiles);
	    fillPlotMap(plotNames[4], myPlotMap, &a, &Analyzer::getZ0Profiles);

	    // TODO: remove this useless cycle
	    /*
	    for (std::map<graphIndex, TGraph*>::iterator it = myPlotMap.begin();
		 it!=myPlotMap.end(); ++it) {
	      myIndex =  (*it).first;
	      std::cerr << "Check: myIndex.name = " << myIndex.name << std::endl; // debug
	    }
	    for (std::map<graphIndex, TGraph*>::iterator myPlotMapIt = myPlotMap.begin();
		 myPlotMapIt!=myPlotMap.end(); ++myPlotMapIt) {
	      myIndex =  (*myPlotMapIt).first;
	      std::cerr << "Check2: myIndex.name = " << myIndex.name << std::endl; // debug
	      }*/


	    // Cycle over the different measurements
	    for (std::vector<std::string>::iterator plotNameIt = plotNames.begin();
		 plotNameIt!=plotNames.end(); ++plotNameIt) {
	      
	      //std::cerr << "tableMap[\""<< *plotNameIt <<"\"] = " << tableMap[*plotNameIt] << std::endl; // debug
	      myTable = tableMap[*plotNameIt];
	      if (!myTable) continue;

	      // Count the realistic plots' momenta
	      std::vector<double> momentum;
	      std::vector<double>::iterator momentumIt;

	      for (std::map<graphIndex, TGraph*>::iterator myPlotMapIt = myPlotMap.begin();
		   myPlotMapIt!=myPlotMap.end(); ++myPlotMapIt) {
		myIndex =  (*myPlotMapIt).first;
		//std::cerr << "Check3: myIndex.name = " << myIndex.name << std::endl; // debug
		if (myIndex.name==(*plotNameIt)) {
		  //std::cerr << "found momentum " << myIndex.p <<std::endl; // debug
		  momentumIt = std::find(momentum.begin(), momentum.end(), myIndex.p);
		  if (momentumIt == momentum.end()) momentum.push_back(myIndex.p);
		}
	      }
	      
	      std::sort(momentum.begin(), momentum.end());
	      //std::cerr << "momentum.size() = " << momentum.size() <<std::endl; // debug

	      // Fill the table with the values
	      // First the heading of momentum
	      int baseColumn;
	      std::vector<double> averagesReal;
	      std::vector<double> averagesIdeal;
	      TGraph* myGraph;
	      int myColor;
	      myIndex.name=(*plotNameIt);
	      for (unsigned int i=0; i<momentum.size(); ++i) {
		baseColumn = nCuts*i+1;
		myTable->setContent(0, baseColumn, momentum[i],0);
		myIndex.p=momentum[i];
		myIndex.ideal = false;
		myGraph = myPlotMap[myIndex];
		myTable->setContent(2, 0, "Real");
		myTable->setContent(3, 0, "Ideal");
		myTable->setContent(4, 0, "Loss");
		if (myGraph) {
		  averagesReal=Analyzer::average(*myGraph, cuts);
		  myColor = myGraph->GetMarkerColor();
		  myTable->setColor(0, baseColumn, myColor);
		}
		myIndex.ideal = true;
		myGraph = myPlotMap[myIndex];
		if (myGraph) averagesIdeal=Analyzer::average(*myGraph, cuts);
		for (unsigned int j=0; j<nCuts; ++j) {
		  myTable->setContent(1, baseColumn+j, cutNames[j]);
		  myTable->setColor(1, baseColumn+j, myColor);
		  if (averagesReal.size() > j) {
		    myTable->setContent(2, baseColumn+j,averagesReal[j],5);
		    myTable->setColor(2, baseColumn+j, myColor);
		  }
		  if (averagesIdeal.size() > j) {
		    myTable->setContent(3, baseColumn+j,averagesIdeal[j],5);
		    myTable->setColor(3, baseColumn+j, myColor);
		  }
		  if ((averagesReal.size() > j)&&(averagesIdeal.size() > j)) {
		    myTable->setContent(4, baseColumn+j,averagesReal[j]/averagesIdeal[j],1);
		    myTable->setColor(4, baseColumn+j, myColor);
		  }
		}
	      }
	    }
            return true;
        }
	return false;
    }

  // TODO: describe this here, if it ever worked
  void Vizard::fillPlotMap(std::string& plotName, 
			   std::map<graphIndex, TGraph*>& myPlotMap,
			   Analyzer *a,
			   std::map<double, TGraph>& (Analyzer::*retriveFunction)(bool)) {
    graphIndex myIndex;
    double p;
    TGraph* myGraph;
    std::vector<std::string> plotNames;

    myIndex.name=plotName;
    //std::cerr << "myIndex.name=" << myIndex.name << std::endl; // debug
    for (int i=0; i<2; ++i) {
      if (i==0) myIndex.ideal=false;
      else myIndex.ideal=true;
      std::map<double, TGraph>& ptProfilesIdeal = (a->*retriveFunction)(myIndex.ideal);
      std::map<double, TGraph>::iterator profilesIterator;
      for (profilesIterator=ptProfilesIdeal.begin();
	   profilesIterator!=ptProfilesIdeal.end();
	   ++profilesIterator) {
	myGraph = &(*profilesIterator).second;
	p = (*profilesIterator).first;
	myIndex.p = p;
	myPlotMap[myIndex] = myGraph;
	//std::cerr << "myIndex.name=" << myIndex.name << std::endl; // debug
      }
    }
    
  }

    
    // public
    // creates a page with all the logs taken from the messagelogger objects
    // @param site a reference to the site we want to work onto
    // @param loggerVector a vector of references to some messageLogger objects
    // @return true if any log was written
    bool Vizard::makeLogPage(RootWSite& site) {
        bool anythingFound=false;
        RootWPage& myPage = site.addPage("Log page");
        for (int iLevel=0; iLevel < MessageLogger::NumberOfLevels; ++iLevel) {
            if (!MessageLogger::hasEmptyLog(iLevel)) {
                bool defaultOpen=false;
                if (iLevel<=MessageLogger::WARNING) defaultOpen=true;
                anythingFound=true;
                RootWContent& newContent = myPage.addContent(MessageLogger::getLevelName(iLevel), defaultOpen);
                newContent.addText("<pre>"+MessageLogger::getLatestLog(iLevel)+"</pre>");
                MessageLogger::getLatestLog(iLevel);
            }
        }
        return anythingFound;
    }
    
    
#endif
    
    // private
    // Draws tickmarks on 3d canvases
    // @param myView the TView where to draw ticks
    // @param maxL maximum tracker length in z
    // @param maxR maximum tracker radius in rho
    // @param noAxis number of the axis: Enumerate sections by axis
    //        index normal to draw plane (if x=1, y=2, z=3)
    // @param spacing grid tick spacing
    // @param option the options to pass to the Draw() method
    void Vizard::drawTicks(TView* myView, double maxL, double maxR, int noAxis/*=1*/, double spacing /*= 100.*/, Option_t* option /*= "same"*/) {
        TPolyLine3D* aLine;
        Color_t gridColor_hard = color_hard_grid;
        int gridStyle_solid = 1;
        std::string theOption(option);
        
        int i;
        int j;
        int k;
        
        double topMax = (maxL > maxR) ? maxL : maxR;
        topMax = ceil(topMax/spacing)*spacing;
        
        double aValue[3];
        double minValue[3];
        double maxValue[3];
        
        i=(noAxis)%3;
        j=(noAxis+1)%3;
        k=(noAxis+2)%3;
        
        maxL *= 1.1;
        maxR *= 1.1;
        
        if (noAxis==1) {
            minValue[0]=0;
            maxValue[0]=+maxR;
            minValue[1]=0;
            maxValue[1]=+maxR;
            minValue[2]=0;
            maxValue[2]=+maxL;
        } else {
            minValue[0]=-maxR;
            maxValue[0]=+maxR;
            minValue[1]=-maxR;
            maxValue[1]=+maxR;
            minValue[2]=0;
            maxValue[2]=+maxL;
        }
        
        aValue[k]=-topMax;
        
        if (noAxis==1) {
            double etaStep=.2;
            double etaMax = 2.1;
            // Add the eta ticks
            double theta;
            double tickLength = 2 * spacing;
            double tickDistance = spacing;
            double startR = maxR + tickDistance;
            double startL = maxL + tickDistance;
            double endR = maxR + tickDistance + tickLength;
            double endL = maxL + tickDistance + tickLength;
            XYZVector startTick;
            XYZVector endTick;
            Double_t pw[3];
            Double_t pn[3];
            TText* aLabel;
            char labelChar[10];
	    double eta;
            for (eta=0; eta<etaMax+etaStep; eta+=etaStep) {
                aLine = new TPolyLine3D(2);
                theta = 2 * atan(exp(-eta));
                startTick = XYZVector(0, sin(theta), cos(theta));
                startTick *= startR/startTick.Rho();
                endTick = startTick / startTick.Rho() * endR;
                if (startTick.Z()>startL) {
                    startTick *= startL/startTick.Z();
                    endTick *=  endL/endTick.Z();
                }
                pw[0]=0.;
                pw[1]=endTick.Y();
                pw[2]=endTick.Z();
                myView->WCtoNDC(pw, pn);
                sprintf(labelChar, "%.01f", eta);
                aLabel = new TText(pn[0], pn[1], labelChar);
                aLabel->SetTextSize(aLabel->GetTextSize()*.6);
                aLabel->SetTextAlign(21);
                aLabel->Draw(theOption.c_str());
                theOption="same";
                endTick = (endTick+startTick)/2.;
                aLine->SetPoint(0, 0., startTick.Y(), startTick.Z());
                aLine->SetPoint(1, 0., endTick.Y(), endTick.Z());
                aLine->SetLineStyle(gridStyle_solid);
                aLine->SetLineColor(gridColor_hard);
                aLine->Draw("same");
            }

            aLine = new TPolyLine3D(2);
            theta = 2 * atan(exp(-2.5));
            startTick = XYZVector(0, sin(theta), cos(theta));
            startTick *= startR/startTick.Rho();
            endTick = startTick / startTick.Rho() * endR;
            if (startTick.Z()>startL) {
                startTick *= startL/startTick.Z();
                endTick *=  endL/endTick.Z();
            }
            pw[0]=0.;
            pw[1]=endTick.Y();
            pw[2]=endTick.Z();
            myView->WCtoNDC(pw, pn);
            sprintf(labelChar, "%.01f", 2.5);
            aLabel = new TText(pn[0], pn[1], labelChar);
            aLabel->SetTextSize(aLabel->GetTextSize()*.8);
            aLabel->SetTextAlign(21);
            aLabel->Draw("same");
            endTick = (endTick+startTick)/2.;
            aLine->SetPoint(0, 0., 0., 0.);
            aLine->SetPoint(1, 0., endTick.Y(), endTick.Z());
            aLine->SetLineStyle(gridStyle_solid);
            aLine->SetLineColor(gridColor_hard);
            aLine->Draw("same");

            for (double z=0; z<=maxL ; z+=(4*spacing)) {
                aLine = new TPolyLine3D(2);
                startTick = XYZVector(0, 0, z);
                endTick = XYZVector(0, -(tickLength/2), z);
                aLine->SetPoint(0, 0., startTick.Y(), startTick.Z());
                aLine->SetPoint(1, 0., endTick.Y(), endTick.Z());
                pw[0]=0.;
                pw[1]=-tickLength;
                pw[2]=endTick.Z();
                myView->WCtoNDC(pw, pn);
                sprintf(labelChar, "%.0f", z);
                aLabel = new TText(pn[0], pn[1], labelChar);
                aLabel->SetTextSize(aLabel->GetTextSize()*.6);
                aLabel->SetTextAlign(23);
                aLabel->Draw(theOption.c_str());
                theOption="same";
                aLine->SetLineStyle(gridStyle_solid);
                aLine->SetLineColor(gridColor_hard);
                aLine->Draw("same");
            }
            
            for (double y=0; y<=maxR ; y+=(2*spacing)) {
                aLine = new TPolyLine3D(2);
                startTick = XYZVector(0, y, 0);
                endTick = XYZVector(0, y, -(tickLength/2));
                aLine->SetPoint(0, 0., startTick.Y(), startTick.Z());
                aLine->SetPoint(1, 0., endTick.Y(), endTick.Z());
                pw[0]=0.;
                pw[1]=endTick.Y();
                pw[2]=-tickLength;
                myView->WCtoNDC(pw, pn);
                sprintf(labelChar, "%.0f", y);
                aLabel = new TText(pn[0], pn[1], labelChar);
                aLabel->SetTextSize(aLabel->GetTextSize()*.6);
                aLabel->SetTextAlign(32);
                aLabel->Draw(theOption.c_str());
                theOption="same";
                aLine->SetLineStyle(gridStyle_solid);
                aLine->SetLineColor(gridColor_hard);
                aLine->Draw("same");
            }
        }
    }
    
    // private
    // Draws a grid on the current canvas
    // @param maxL maximum tracker length in z
    // @param maxR maximum tracker radius in rho
    // @param noAxis number of the axis: Enumerate sections by axis
    //        index normal to draw plane (if x=1, y=2, z=3)
    // @param spacing grid tick spacing
    // @param option the options to pass to the Draw() method
    void Vizard::drawGrid(double maxL, double maxR, int noAxis/*=1*/, double spacing /*= 100.*/, Option_t* option /*= "same"*/) {
        TPolyLine3D* aLine;
        Color_t gridColor = color_grid;
        Color_t gridColor_hard = color_hard_grid;
        Color_t thisLineColor;
        
        std::string theOption(option);
        
        int i;
        int j;
        int k;
        
        double topMax = (maxL > maxR) ? maxL : maxR;
        topMax = ceil(topMax/spacing)*spacing;
        
        double aValue[3];
        double minValue[3];
        double maxValue[3];
        double runValue;
        int thisLineStyle;
        
        i=(noAxis)%3;
        j=(noAxis+1)%3;
        k=(noAxis+2)%3;
        
        maxL *= 1.1;
        maxR *= 1.1;
        
        if (noAxis==1) {
            minValue[0]=0;
            maxValue[0]=+maxR;
            minValue[1]=0;
            maxValue[1]=+maxR;
            minValue[2]=0;
            maxValue[2]=+maxL;
        } else {
            minValue[0]=-maxR;
            maxValue[0]=+maxR;
            minValue[1]=-maxR;
            maxValue[1]=+maxR;
            minValue[2]=0;
            maxValue[2]=+maxL;
        }
        
        aValue[k]=-topMax;
        for(runValue = -topMax; runValue<=topMax; runValue+=spacing) {
            
            // Special line for axis
            if (runValue==0) {
                thisLineStyle=1;
                thisLineColor=gridColor_hard;
            } else {
                thisLineStyle=2;
                thisLineColor=gridColor;
            }
            
            // Parallel to j
            if ((runValue<=maxValue[i])&&(runValue>=minValue[i])) {
                aValue[i] = runValue;
                aLine = new TPolyLine3D(2);
                aValue[j] = minValue[j];
                aLine->SetPoint(0, aValue[0], aValue[1], aValue[2]);
                aValue[j] = maxValue[j];
                aLine->SetPoint(1, aValue[0], aValue[1], aValue[2]);
                aLine->SetLineStyle(thisLineStyle);
                aLine->SetLineColor(thisLineColor);
                aLine->Draw(theOption.c_str());
                theOption="same";
            };
            
            // Parallel to i
            if ((runValue<=maxValue[j])&&(runValue>=minValue[j])) {
                aValue[j] = runValue;
                aLine = new TPolyLine3D(2);
                aValue[i] = minValue[i];
                aLine->SetPoint(0, aValue[0], aValue[1], aValue[2]);
                aValue[i] = maxValue[i];
                aLine->SetPoint(1, aValue[0], aValue[1], aValue[2]);
                aLine->SetLineStyle(thisLineStyle);
                aLine->SetLineColor(thisLineColor);
                aLine->Draw(theOption.c_str());
                theOption="same";
            };
            
        }
    }
    
    // private
    // DEPRECATED
    // Creates a new 4-pad canvas with XY and YZ views with all the useful details, like the axis ticks
    // and the eta reference. The fourth pad contains a miniature of the eta profile coverage
    // if you need any of these you can get them with GetPad()
    // @param maxZ maximum tracker's Z coordinate to be shown
    // @param maxRho maximum tracker's Rho coordinate to be shown
    // @param analyzer A reference to the analysing class that examined the material budget and filled the histograms
    // @return a pointer to the new TCanvas
    void Vizard::createSummaryCanvas(double maxZ, double maxRho, Analyzer& analyzer, TCanvas *&summaryCanvas,
				     TCanvas *&YZCanvas, TCanvas *&XYCanvas, TCanvas *&XYCanvasEC) {
        Int_t irep;
        TVirtualPad* myPad;

        YZCanvas = new TCanvas("YZCanvas", "YZView Canvas", 600, 600 );
        XYCanvas = new TCanvas("XYCanvas", "XYView Canvas", 600, 600 );
        XYCanvasEC = new TCanvas("XYCanvasEC", "XYView Canvas (Endcap)", 600, 600 );
        summaryCanvas = new TCanvas("summaryCanvas", "Summary Canvas", 600, 600);
        summaryCanvas->SetFillColor(color_pad_background);
        summaryCanvas->Divide(2, 2);
        
        for (int i=1; i<=4; i++) { myPad=summaryCanvas->GetPad(i); myPad->SetFillColor(color_plot_background);  }
        
        // First pad
        // YZView
        myPad = summaryCanvas->GetPad(padYZ);
        myPad->SetFillColor(color_plot_background);
        myPad->cd();
        if (analyzer.getGeomLiteYZ()) {
            drawGrid(maxZ, maxRho, ViewSectionYZ);
            analyzer.getGeomLiteYZ()->DrawClonePad();
            myPad->SetFillColor(color_plot_background);
            myPad->GetView()->SetParallel();
            myPad->GetView()->SetRange(0, 0, 0, maxZ, maxZ, maxZ);
            myPad->GetView()->SetView(0 /*long*/, 270/*lat*/, 270/*psi*/, irep);
            drawTicks(myPad->GetView(), maxZ, maxRho, ViewSectionYZ);
            
            YZCanvas->cd();
            myPad = YZCanvas->GetPad(0);
            drawGrid(maxZ, maxRho, ViewSectionYZ);
            analyzer.getGeomLiteYZ()->DrawClonePad();
	    myPad->SetBorderMode(0);
	    myPad->SetFillColor(color_plot_background);
            myPad->GetView()->SetParallel();
            myPad->GetView()->SetRange(0, 0, 0, maxZ, maxZ, maxZ);
            myPad->GetView()->SetView(0 /*long*/, 270/*lat*/, 270/*psi*/, irep);
            drawTicks(myPad->GetView(), maxZ, maxRho, ViewSectionYZ);
        }
        
        // Second pad
        // XYView (barrel)
        myPad = summaryCanvas->GetPad(padXY);
        myPad->cd();
        myPad->SetFillColor(color_plot_background);
        if (analyzer.getGeomLiteXY()) {
            drawGrid(maxZ, maxRho, ViewSectionXY);
            analyzer.getGeomLiteXY()->DrawClonePad();
            myPad->SetFillColor(color_plot_background);
            myPad->GetView()->SetParallel();
            myPad->GetView()->SetRange(-maxRho, -maxRho, -maxRho, maxRho, maxRho, maxRho);
            myPad->GetView()->SetView(0 /*long*/, 0/*lat*/, 270/*psi*/, irep);

	    XYCanvas->cd();
	    myPad = XYCanvas->GetPad(0);
            drawGrid(maxZ, maxRho, ViewSectionXY);
            analyzer.getGeomLiteXY()->DrawClonePad();
            myPad->SetFillColor(color_plot_background);
            myPad->GetView()->SetParallel();
            myPad->GetView()->SetRange(-maxRho, -maxRho, -maxRho, maxRho, maxRho, maxRho);
            myPad->GetView()->SetView(0 /*long*/, 0/*lat*/, 270/*psi*/, irep);
        }
        
        // Third pad
        // Plots
        myPad = summaryCanvas->GetPad(padProfile);
	drawEtaProfiles(*myPad, analyzer);
        
        // Fourth pad
        // XYView (EndCap)
        myPad = summaryCanvas->GetPad(padEC);
        myPad->cd();
        myPad->SetFillColor(color_plot_background);
        if (analyzer.getGeomLiteEC()) {
            drawGrid(maxZ, maxRho, ViewSectionXY);
            analyzer.getGeomLiteEC()->DrawClonePad();
            myPad->SetFillColor(color_plot_background);
            myPad->GetView()->SetParallel();
            myPad->GetView()->SetRange(-maxRho, -maxRho, -maxRho, maxRho, maxRho, maxRho);
            myPad->GetView()->SetView(0 /*long*/, 0/*lat*/, 270/*psi*/, irep);

	    XYCanvasEC->cd();
	    myPad = XYCanvasEC->GetPad(0);
            drawGrid(maxZ, maxRho, ViewSectionXY);
            analyzer.getGeomLiteEC()->DrawClonePad();
            myPad->SetFillColor(color_plot_background);
            myPad->GetView()->SetParallel();
            myPad->GetView()->SetRange(-maxRho, -maxRho, -maxRho, maxRho, maxRho, maxRho);
            myPad->GetView()->SetView(0 /*long*/, 0/*lat*/, 270/*psi*/, irep);
        }
        
        for (int i=1; i<=4; i++) { myPad=summaryCanvas->GetPad(i); myPad->SetBorderMode(0); }
        summaryCanvas->Modified();
        //return summaryCanvas;
    }


    // private
    // Creates 4 new canvas with XY and YZ views with all the useful details, like the axis ticks
    // and the eta reference.
    // @param maxZ maximum tracker's Z coordinate to be shown
    // @param maxRho maximum tracker's Rho coordinate to be shown
    // @param analyzer A reference to the analysing class that examined the material budget and filled the histograms
    // @return a pointer to the new TCanvas
  void Vizard::createSummaryCanvas(double maxZ, double maxRho, Analyzer& analyzer,
				   TCanvas *&YZCanvas, TCanvas *&XYCanvas,
				   TCanvas *&XYCanvasEC) {
        Int_t irep;
        TVirtualPad* myPad;

        YZCanvas = new TCanvas("YZCanvas", "YZView Canvas", 600, 600 );
        XYCanvas = new TCanvas("XYCanvas", "XYView Canvas", 600, 600 );
        XYCanvasEC = new TCanvas("XYCanvasEC", "XYView Canvas (Endcap)", 600, 600 );
        
        // YZView
        if (analyzer.getGeomLiteYZ()) {
            YZCanvas->cd();
            myPad = YZCanvas->GetPad(0);
            drawGrid(maxZ, maxRho, ViewSectionYZ);
            analyzer.getGeomLiteYZ()->DrawClonePad();
	    myPad->SetBorderMode(0);
	    myPad->SetFillColor(color_plot_background);
            myPad->GetView()->SetParallel();
            myPad->GetView()->SetRange(0, 0, 0, maxZ, maxZ, maxZ);
            myPad->GetView()->SetView(0 /*long*/, 270/*lat*/, 270/*psi*/, irep);
            drawTicks(myPad->GetView(), maxZ, maxRho, ViewSectionYZ);
        }
        
        // XYView (barrel)
        if (analyzer.getGeomLiteXY()) {
	    XYCanvas->cd();
	    myPad = XYCanvas->GetPad(0);
            drawGrid(maxZ, maxRho, ViewSectionXY);
            analyzer.getGeomLiteXY()->DrawClonePad();
            myPad->SetFillColor(color_plot_background);
            myPad->GetView()->SetParallel();
            myPad->GetView()->SetRange(-maxRho, -maxRho, -maxRho, maxRho, maxRho, maxRho);
            myPad->GetView()->SetView(0 /*long*/, 0/*lat*/, 270/*psi*/, irep);
        }
        
        // XYView (EndCap)
        if (analyzer.getGeomLiteEC()) {
	    XYCanvasEC->cd();
	    myPad = XYCanvasEC->GetPad(0);
            drawGrid(maxZ, maxRho, ViewSectionXY);
            analyzer.getGeomLiteEC()->DrawClonePad();
            myPad->SetFillColor(color_plot_background);
            myPad->GetView()->SetParallel();
            myPad->GetView()->SetRange(-maxRho, -maxRho, -maxRho, maxRho, maxRho, maxRho);
            myPad->GetView()->SetView(0 /*long*/, 0/*lat*/, 270/*psi*/, irep);
        }
        
        //return summaryCanvas;
  }    
  
  /*
   * Returns always the same color for a given momentum index
   * @param iMomentum index of the momentum
   * @return the color index in ROOT
   */
  int Vizard::momentumColor(int iMomentum) {
    if (iMomentum==0) return kBlack;
    if (iMomentum==1) return kBlue;
    if (iMomentum==2) return kRed;   
    if (iMomentum==3) return kGreen;   
    return iMomentum+1;
  }
  
  /*
   * Modifies a TGraph, so that it looks like a
   * histogram (can be filled)
   * @param myGraph a reference to the TGraph to be modified
   */  
  void Vizard::closeGraph(TGraph& myGraph) {
    double x, y, x0, y0;
    myGraph.GetPoint(myGraph.GetN()-1, x, y);
    myGraph.GetPoint(0, x0, y0);
    myGraph.SetPoint(myGraph.GetN(), x,0);
    myGraph.SetPoint(myGraph.GetN(), x0,0);
  }


}

