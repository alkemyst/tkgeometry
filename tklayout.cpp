//
// File:   tklayout.cpp
// Author: ndemaio
//
// Created on June 8, 2009, 10:51 AM
//

/**
 * @file tklayout.cpp
 * @brief This is <i>tkgeometry</i>'s main program.
 * It collects switches and parameter files and decides what to do with them based on what it found.
 * To find out the available options, running <i>bin/tklayout</i> without parameters will print a message that shows them.
 */

#include <stdlib.h>
#include <Squid.h>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    std::string usage("Usage: ");
    usage += argv[0];
    usage += " <config basename> [options]";
    usage += "\n\n<config basename> is the user-specified part of the config file names.\nFull names are automatically inferred from it by appending appropriate suffixes\n";
    po::options_description shown("Allowed options");
    int geomtracks, mattracks;

    std::string basename, xmlname;

    shown.add_options()
        ("help", "Display this help message.")
        ("geometry-tracks,n", po::value<int>(&geomtracks)->default_value(50), "N. of tracks for geometry calculations.")
        ("material-tracks,N", po::value<int>(&mattracks)->default_value(2000), "N. of tracks for material calculations.")
        ("power,p", "Report irradiated power analysis.")
        ("bandwidth,b", "Report base bandwidth analysis.")
        ("bandwidth-cpu,B", "Report multi-cpu bandwidth analysis.\n\t(implies 'b')")
        ("material,m", "Report materials and weights analyses.")
        ("resolution,r", "Report resolution analysis.")
        ("trigger,t", "Report base trigger analysis.")
        ("trigger-ext,T", "Report extended trigger analysis.\n\t(implies 't')")
        ("all,a", "Report all analyses, except extended\ntrigger. (implies all other relevant\nreport options)")
        ("graph,g", "Build and report neighbour graph.")
        ("xml", po::value<std::string>(&xmlname)->implicit_value(""), "Produce XML output files for materials.\nOptional arg specifies the subdirectory\nof the output directory (chosen via inst\nscript) where to create XML files.\nIf not supplied, basename will be used\nas subdir.")
    ;

    po::options_description hidden;
    hidden.add_options()("base-name", po::value<std::string>(&basename));

    po::positional_options_description posopt;
    posopt.add("base-name", 1); 

    po::options_description allopt;
    allopt.add(shown).add(hidden);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(allopt).positional(posopt).run(), vm);

        po::notify(vm);

        if (geomtracks < 1) throw po::invalid_option_value("geometry-tracks");
        if (mattracks < 1) throw po::invalid_option_value("material-tracks");
        if (!vm.count("base-name")) throw po::error("Missing required config basename argument"); 

    } catch(po::error e) {
        std::cerr << e.what() << std::endl << std::endl;
        std::cout << usage << std::endl << shown << std::endl;
        return EXIT_FAILURE;
    }
    
    // argument processing
    i++;
    // argument can be option switch or geomfile
    if (argv[i][0] == '-') {
        std::string tmp = argv[i];
        tmp = tmp.substr(1);
        balgo::trim(tmp);
        for (std::string::iterator iter = tmp.begin(); iter != tmp.end(); iter++) {
            switch (*iter) {
                case 'u': u = true;
                break;
                case 'm': m = true;
                break;
                case 'd': d = true;
                break;
                default: std::cerr << "Error: unknown parameter " << *iter << ". Aborting tklayout." << std::endl;
                return (EXIT_FAILURE);
            }
        }
        switch_processed = true;
    }

            
    insur::Squid squid;
    bool verboseMaterial = false;

    squid.setBasename(basename);

    // The tracker (and possibly pixel) must be build in any case
    if (!squid.buildTracker()) return EXIT_FAILURE;

    // The tracker should pick the types here but in case it does not,
    // we can still write something
    if (squid.dressTracker()) {
      if (!squid.pureAnalyzeGeometry(geomtracks)) return EXIT_FAILURE;

      if ((vm.count("all") || vm.count("bandwidth") || vm.count("bandwidth-cpu")) && !squid.reportBandwidthSite()) return EXIT_FAILURE;
      if ((vm.count("all") || vm.count("bandwidth-cpu")) && (!squid.reportTriggerProcessorsSite()) ) return EXIT_FAILURE;
      if ((vm.count("all") || vm.count("power")) && (!squid.irradiateTracker() || !squid.reportPowerSite()) ) return EXIT_FAILURE;

      // If we need to have the material model, then we build it
      if ( vm.count("all") || vm.count("material") || vm.count("resolution") || vm.count("graph") || vm.count("xml") ) {
	if (squid.buildInactiveSurfaces(verboseMaterial) && squid.createMaterialBudget(verboseMaterial)) {
	  if ( vm.count("all") || vm.count("material") || vm.count("resolution") ) {
	    // TODO: the following call should know whether to compute resolution or material (or both)
	    if (!squid.pureAnalyzeMaterialBudget(mattracks, true)) return EXIT_FAILURE;
	    if ((vm.count("all") || vm.count("material"))  && !squid.reportMaterialBudgetSite()) return EXIT_FAILURE;
	    if ((vm.count("all") || vm.count("resolution"))  && !squid.reportResolutionSite()) return EXIT_FAILURE;	  
	  }
	  if (vm.count("graph") && !squid.reportNeighbourGraphSite()) return EXIT_FAILURE;
	  if (vm.count("xml") && !squid.translateFullSystemToXML(xmlname.empty() ? basename : xmlname, false)) return (EXIT_FAILURE); //TODO: take care of flag in a more intelligent way...
	}
      }

      if ((vm.count("all") || vm.count("trigger") || vm.count("trigger-ext")) &&
	  ( !squid.analyzeTriggerEfficiency(mattracks, vm.count("trigger-ext")) || !squid.reportTriggerPerformanceSite(vm.count("trigger-ext"))) ) return EXIT_FAILURE;
    } else if (!squid.pureAnalyzeGeometry(geomtracks)) return EXIT_FAILURE;


    if (!squid.reportGeometrySite()) return EXIT_FAILURE;
    if (!squid.additionalInfoSite()) return EXIT_FAILURE;
    if (!squid.makeSite()) return EXIT_FAILURE;
    
    //DEBUG: print internal status
    /*std::cout << std::endl << "Internal status after parsing " << argc << " arguments (i = " << i << "):" << std::endl;
    std::cout << "switch_processed = " << (switch_processed ? "true" : "false") << ", files_processed = ";
    std::cout << (files_processed ? "true" : "false") << ", cfiles = " << cfiles << std::endl;
    std::cout << "usher_verbose = " << (u ? "true" : "false") << ", mat_verbose = " << (m ? "true" : "false");
    std::cout << ", detailed = " << (d ? "true" : "false") << std::endl;
    std::cout << "geomfile = " << geomfile << ", settingsfile = " << settingsfile << ", matfile = " << matfile << "." <<std::endl;
    std::cout << "htmlflag = " << (h ? "true" : "false") << ", htmlout = " << htmlout << "." << std::endl;
    std::cout << "rootflag = " << (r ? "true" : "false") << ", rootout = " << rootout << "." << std::endl;
    std::cout << "graphflag = " << (g ? "true" : "false") << ", graphout = " << graphout << "." << std::endl;
    std::cout << "xmlflag = " << (x ? "true" : "false") << ", xmlout = " << xmlout << "." << std::endl;
    std::cout << "trackflag = " << (t ? "true" : "false") << ", tracks = " << tracks << std::endl << std::endl;*/
    
    // here comes the heavy lifting...
    insur::Squid s;
    // TODO: review completely the argument parsing !!
    if (h || x) {
        if (matfile!="") pixmatfile = matfile+".pix";
        if (!s.buildFullSystem(geomfile, settingsfile, matfile, pixmatfile, u, m)) return (EXIT_FAILURE);
        if (h) {
	  if (tracks_geom==0) tracks_geom = 2000;
	  if (tracks==0) tracks = 50;
	  s.pureAnalyzeGeometry(tracks_geom);
	  std::cout << "Calling analyzer with " << tracks << " tracks." << std::endl;
	  if (!s.pureAnalyzeMaterialBudget(tracks)) return (EXIT_FAILURE);
	  if (!(s.reportGeometrySite())) return (EXIT_FAILURE);
	  if (!(s.reportMaterialBudgetSite())) return (EXIT_FAILURE);
	   if (!(s.reportTriggerPerformanceSite())) return (EXIT_FAILURE); // TODO: put this back
	  if (!(s.additionalInfoSite(geomfile, settingsfile, matfile, pixmatfile))) return (EXIT_FAILURE);
	  if (!(s.makeSite())) return (EXIT_FAILURE);
        }
        if (r) {
            if (rootout.empty()) {
                rootout = geomfile;
                pos = rootout.find_last_of('/');
                if (pos != (int)rootout.npos) {
                    pos++;
                    rootout = rootout.substr(pos);
                }
                pos = rootout.find('.');
                if (pos != (int)rootout.npos) {
                    if (pos > (int)rootout.npos - 1) rootout.erase(pos + 1);
                }
                else rootout.push_back('.');
                rootout = rootout + "root";
                std::cout << "ROOT file will be written to " << rootout << std::endl;
            }
            if (!s.analyzeGeometry(rootout, !d)) return (EXIT_FAILURE);
        }
        if (g) {
            if (graphout.empty()) {
                graphout = geomfile;
                pos = graphout.find_last_of('/');
                if (pos != (int)graphout.npos) {
                    pos++;
                    graphout = graphout.substr(pos);
                }
                pos = graphout.find('.');
                if (pos != (int)graphout.npos) {
                    if (pos > (int)graphout.npos - 1) graphout.erase(pos + 1);
                }
                else graphout.push_back('.');
                graphout = graphout + "graph";
                std::cout << "Graph file will be written to " << graphout << std::endl;
            }
            if (!s.analyzeNeighbours(graphout)) return (EXIT_FAILURE);
        }
        if (x) {
            if (xmlout.empty()) {
                xmlout = geomfile;
                pos = xmlout.find_last_of('/');
                if (pos != (int)xmlout.npos) {
                    pos++;
                    xmlout = xmlout.substr(pos);
                }
                pos = xmlout.find('.');
                if (pos != (int)xmlout.npos) {
                    if (pos > (int)xmlout.npos - 1) xmlout.erase(pos);
                }
                std::cout << "XML files will be written to subdirectory " << xmlout << std::endl;
            }
            if (!s.translateFullSystemToXML(xmlout, false)) return (EXIT_FAILURE); //TODO: take care of flag in a more intelligent way...
            // false: a la nico, true: a la Harry
        }
    }
    else if (r || g) {
        if (!s.buildTracker(geomfile)) return (EXIT_FAILURE);
        if (settingsfile.empty()) std::cout << "Warning: using tracker geometry without a settings file to dress it from." << std::endl;
        else {
            if (!s.dressTracker(settingsfile)) return (EXIT_FAILURE);
            if (!s.irradiateTracker()) return (EXIT_FAILURE);
        }
        if (!s.buildInactiveSurfaces(u)) return (EXIT_FAILURE);
        if (m && !matfile.empty()) s.createMaterialBudget(matfile, m);
        if (r) {
            if (rootout.empty()) {
                rootout = geomfile;
                pos = rootout.find_last_of('/');
                if (pos != (int)rootout.npos) {
                    pos++;
                    rootout = rootout.substr(pos);
                }
                pos = rootout.find('.');
                if (pos != (int)rootout.npos) {
                    if (pos > (int)rootout.npos - 1) rootout.erase(pos + 1);
                }
                else rootout.push_back('.');
                rootout = rootout + "root";
                std::cout << "ROOT file will be written to " << rootout << std::endl;
            }
            if (!s.analyzeGeometry(rootout, !d)) return (EXIT_FAILURE);
        }
        if (g) {
            if (graphout.empty()) {
                graphout = geomfile;
                pos = graphout.find_last_of('/');
                if (pos != (int)graphout.npos) {
                    pos++;
                    graphout = graphout.substr(pos);
                }
                pos = graphout.find('.');
                if (pos != (int)graphout.npos) {
                    if (pos > (int)graphout.npos - 1) graphout.erase(pos + 1);
                }
                else graphout.push_back('.');
                graphout = graphout + "graph";
                std::cout << "Graph file will be written to " << graphout << std::endl;
            }
            if (!s.analyzeNeighbours(graphout)) return (EXIT_FAILURE);
        }
    }
    else {
        //no output files required
        switch (cfiles) {
            case 1 : if (!s.buildInactiveSurfaces(geomfile, u)) return (EXIT_FAILURE);
            break;
            case 2 : if (!s.buildInactiveSurfaces(geomfile, settingsfile, u)) return (EXIT_FAILURE);
            break;
            case 3 : if (!s.buildFullSystem(geomfile, settingsfile, matfile, u, m)) return (EXIT_FAILURE);
            break;
            default: std::cerr << "Something truly strange happened during processing: the command line passed the parsing stage ";
            std::cerr << "but the number of input files is not between 1 and 3. Aborting tklayout." << std::endl;
            return (EXIT_FAILURE);
        }
    }
    std::cout << "Done." << std::endl;
    return (EXIT_SUCCESS);
}

