/**
 * @file XMLWriter.cc
 * @brief This class implements the output functions that turn a set of previously collected tracker information into a series of CMSSW XML files
 */

#include <XMLWriter.h>
#include <stdlib.h> // Because atoi() is used


namespace insur {
    //public
    /**
     * This creates a custom file <i>pixbar.xml</i> from a skeleton file using the list of previously collected shapes.
     * It identifies, by name tag, the vector of <i>(r, z)</i>-points that describe the volume addition to the pixel barrel.
     * It then writes those new coordinates into the skeleton file at the appropriate position.
     * @param s The vector containing the list of individual shapes that make up the tracker
     * @param in A reference to a file stream that is bound to the skeleton file
     * @param out A reference to a file stream that is bound to the output file
     */
    void XMLWriter::pixbar(std::vector<ShapeInfo>& s, std::ifstream& in, std::ofstream& out) {
        unsigned int pos = 0;
        std::string line;
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        if (s.size() > 0) {
            while ((pos < s.size()) && (s.at(pos).name_tag.find(xml_tob) == std::string::npos)) pos++;
            if ((pos < s.size()) && (s.at(pos).rzup.size() > 0)) {
                out << xml_rzpoint_open << s.at(pos).rzup.at(0).first << xml_rzpoint_inter;
                out << "-" << xml_zv3 << xml_general_endline;
                for (unsigned int i = 0; i < s.at(pos).rzup.size(); i++) {
                    out << xml_rzpoint_open << s.at(pos).rzup.at(i).first << xml_rzpoint_inter;
                    out << s.at(pos).rzup.at(i).second << xml_rzpoint_close;
                }
                for (unsigned int i = s.at(pos).rzdown.size(); i > 0; i--) {
                    out << xml_rzpoint_open << s.at(pos).rzdown.at(i - 1).first << xml_rzpoint_inter;
                    out << s.at(pos).rzdown.at(i - 1).second << xml_rzpoint_close;
                }
                out << xml_rzpoint_open << s.at(pos).rzup.at(0).first << xml_rzpoint_inter;
                out << xml_zv3 << xml_general_endline;
            }
        }
        while (std::getline(in, line)) out << line << std::endl;
    }
    
    /**
     * This creates a custom file <i>pixfwd.xml</i> from a skeleton file using the list of previously collected shapes.
     * It identifies, by name tag, the vector of <i>(r, z)</i>-points that describe the volume addition to the pixel endcap.
     * If such an entry exists, it writes those new coordinates into the skeleton file at the appropriate position. If the tracker
     * above the pixel detector has no endcaps, the skeleton file remains unchanged but is nevertheless copied to a new
     * output file (minus the comment that serves as a position marker in the skeleton file).
     * @param s The vector containing the list of individual shapes that make up the tracker
     * @param in A reference to a file stream that is bound to the skeleton file
     * @param out A reference to a file stream that is bound to the output file
     */
    void XMLWriter::pixfwd(std::vector<ShapeInfo>& s, std::ifstream& in, std::ofstream& out) {
        unsigned pos = 0;
        std::string line;
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        if (s.size() > 0) {
            while ((pos < s.size()) && (s.at(pos).name_tag.find(xml_tid) == std::string::npos)) pos++;
            if ((pos < s.size()) && (s.at(pos).rzup.size() > 0) && (s.at(pos).rzdown.size() > 0)) {
                out << xml_rzpoint_open << xml_root_radius << xml_rzpoint_inter;
                out << s.at(pos).rzup.at(0).second << xml_rzpoint_close;
                for (unsigned int i = 0; i < s.at(pos).rzup.size(); i++) {
                    out << xml_rzpoint_open << s.at(pos).rzup.at(i).first << xml_rzpoint_inter;
                    out << s.at(pos).rzup.at(i).second << xml_rzpoint_close;
                }
                for (unsigned int i = s.at(pos).rzdown.size(); i > 0; i--) {
                    out << xml_rzpoint_open << s.at(pos).rzdown.at(i - 1).first << xml_rzpoint_inter;
                    out << s.at(pos).rzdown.at(i - 1).second << xml_rzpoint_close;
                }
                out << xml_rzpoint_open << xml_track_beam_r2 << xml_rzpoint_inter;
                out << s.at(pos).rzdown.at(0).second << xml_rzpoint_close;
            }
        }
        while (std::getline(in, line)) out << line << std::endl;
    }
    
    /**
     * This function creates the file <i>tracker.xml</i> from scratch from the information that is available in struct
     * <i>d</i>. It does this by iterating over the contents of the various vectors in the struct, formatting the information
     * within according to what CMSSW will expect to find, and writing the result to the output file. The tracker file also
     * includes the material descriptions for the various modules and inactive surfaces since most of those will be non-
     * standard mixtures not available in CMSSW elsewhere. Formatting of the data in <i>d</i> is actually delegated to
     * a number of protected functions depending on the type of information that needs to be processed. This function
     * makes sure they are called in the right order and at the right time. Additionally, it writes the opening and closing
     * tags of the file itself.
     * @param d A reference to a struct containing a number of vectors for the previously extracted tracker information
     * @param out A reference to a file stream that is bound to the output file
     */
    void XMLWriter::tracker(CMSSWBundle& d, std::ofstream& out, bool wt) {
        std::vector<Element>& e = d.elements;
        std::vector<Composite>& c = d.composites;
        std::vector<LogicalInfo>& l = d.logic;
        std::vector<ShapeInfo>& s = d.shapes;
        std::vector<PosInfo>& p = d.positions;
        std::vector<AlgoInfo>& a = d.algos;
        std::vector<Rotation>& r = d.rots;
        std::ostringstream buffer;
        buffer << xml_preamble;
        if (wt) {
            buffer << xml_new_const_section;
            materialSection(xml_newtrackerfile, e, c, buffer);
            rotationSection(r, xml_newtrackerfile, buffer);
            logicalPartSection(l, xml_newtrackerfile, buffer, true);
            solidSection(s, xml_newtrackerfile, buffer, true, true);
            posPartSection(p, a, xml_newtrackerfile, buffer);
        }
        else {
            buffer << xml_const_section;
            materialSection(xml_trackerfile, e, c, buffer);
            rotationSection(r, xml_trackerfile, buffer);
            logicalPartSection(l, xml_trackerfile, buffer);
            solidSection(s, xml_trackerfile, buffer, true);
            posPartSection(p, a, xml_trackerfile, buffer);
        }
        buffer << xml_defclose;
        out << buffer.str();
    }
    
    /**
     * The modified topology file <i>trackerStructureTopology.xml</i> is created from a skeleton file in this
     * function. The additions are spread out over many places within the file, and stand for a number of different
     * things. First, hierarchical information is added to those <i>SpecPar</i> blocks listing layers, rods (ladders)
     * and active surfaces of the barrel. The same is done for the endcaps in the blocks listing discs, rings (panels)
     * and, again, active surfaces. Then, all active modules, from the barrels as well as from any endcaps, are
     * listed in blocks that specify 128 channels per row and one channel per colums for a ROC. Last, a <i>SpecPar</i>
     * block is added for each multiple of those channels (row and column) that occur on the modules.
     * @t A reference to the collection of tracker topology information
     * @in A reference to a file stream that is bound to the input file
     * @out A reference to a file stream that is bound to the output file
     */
    void XMLWriter::topology(std::vector<SpecParInfo>& t, std::ifstream& in, std::ofstream& out) {
        std::ostringstream strm;
        std::string line;
        unsigned int i;
        int pos;
        //find PixelBarrelLayerPar
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        //      add all TOB layers
        pos = findEntry(t, xml_subdet_layer + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        //find PixelBarrelLadderPar
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        //      add all rods
        pos = findEntry(t, xml_subdet_rod + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        //find PixelBarrelModulePar
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        //      add all BModule...active
        pos = findEntry(t, xml_subdet_tobdet + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        //find PixelEndcapDiskPar
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        //      add all z+ discs ONCE (for now)
        pos = findEntry(t, xml_subdet_wheel + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        //find PixelEndcapPanelPar
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        //      add all z+ rings ONCE
        pos = findEntry(t, xml_subdet_ring + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        //find PixelEndcapDetPar
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        //      add all z+ EModule...active
        pos = findEntry(t, xml_subdet_tiddet + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        //find PixelROCRowsTrackerPar
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        //      add ALL active modules (barrel and endcap)
        pos = findEntry(t, xml_subdet_tobdet + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        pos = findEntry(t, xml_subdet_tiddet + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        //find PixelROCColsTrackerPar
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        //      add ALL active modules (barrel and endcap)
        pos = findEntry(t, xml_subdet_tobdet + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        pos = findEntry(t, xml_subdet_tiddet + xml_par_tail);
        if (pos != -1) {
            for (i = 0; i < t.at(pos).partselectors.size(); i++)
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
        }
        //find final marker
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        //      add one SpecPar block for every entry in t where parameter.first == PixelROC_X
        for (i = 0; i < t.size(); i++) {
            if (t.at(i).parameter.first.compare(xml_roc_x) == 0) {
                specPar(t.at(i).name, t.at(i).parameter, t.at(i).partselectors, strm);
            }
        }
        //      add one SpecPar block for every entry in t where parameter.first == PixelROC_Y
        for (i = 0; i < t.size(); i++) {
            if (t.at(i).parameter.first.compare(xml_roc_y) == 0)
                specPar(t.at(i).name, t.at(i).parameter, t.at(i).partselectors, strm);
        }
        out << strm.str();
        //copy rest of skeleton file unchanged
        while (std::getline(in, line)) out << line << std::endl;
    }
    
    /**
     * This function modifies the skeleton file <i>trackerProdCuts.xml</i> and writes the result to a new file
     * of the same name. Effectively, it simply adds to a long list of entries for active surfaces.
     * @param t A reference to the collection of tracker topology information
     * @param in A reference to a file stream that is bound to the input file
     * @param out A reference to a file stream that is bound to the output file
     */
    void XMLWriter::prodcuts(std::vector<SpecParInfo>& t, std::ifstream& in, std::ofstream& out) {
        unsigned int pos = 0;
        std::string line;
        // head of file
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        // TOB
        while ((pos < t.size()) && (t.at(pos).name.find(xml_subdet_tobdet) == std::string::npos)) pos++;
        if (pos < t.size()) {
            for (unsigned int i = 0; i < t.at(pos).partselectors.size(); i++) {
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
            }
        }
        pos = 0;
        // TID
        while ((pos < t.size()) && (t.at(pos).name.find(xml_subdet_tiddet) == std::string::npos)) pos++;
        if (pos < t.size()) {
            for (unsigned int i = 0; i < t.at(pos).partselectors.size(); i++) {
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
            }
        }
        // tail of file
        while (std::getline(in, line)) out << line << std::endl;
    }
    
    /**
     * This function modifies the skeleton file <i>trackersens.xml</i> and writes the result to a new file of the
     * same name. Effectively, it simply adds to two long lists of entries for active surfaces, one for those in the
     * barrel and one for those in endcap if there is one.
     * @param t A reference to the collection of tracker topology information
     * @param in A reference to a file stream that is bound to the input file
     * @param out A reference to a file stream that is bound to the output file
     */
    void XMLWriter::trackersens(std::vector<SpecParInfo>& t, std::ifstream& in, std::ofstream& out) {
        unsigned int pos = 0;
        std::string line;
        // TOB
        while ((pos < t.size()) && (t.at(pos).name.find(xml_subdet_tobdet) == std::string::npos)) pos++;
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        if (pos < t.size()) {
            for (unsigned int i = 0; i < t.at(pos).partselectors.size(); i++) {
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
            }
        }
        pos = 0;
        // TID
        while ((pos < t.size()) && (t.at(pos).name.find(xml_subdet_tiddet) == std::string::npos)) pos++;
        while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
        if (pos < t.size()) {
            for (unsigned int i = 0; i < t.at(pos).partselectors.size(); i++) {
                out << xml_spec_par_selector << t.at(pos).partselectors.at(i) << xml_general_endline;
            }
        }
        // tail of file
        while (std::getline(in, line)) out << line << std::endl;
    }
    
    /**
     * This function modifies the skeleton file <i>trackerRecoMaterial.xml</i> and writes the result to a new file of the
     * same name. Unlike the other files that use the topology information, this one lists the full paths from the top of the
     * hierarchy (using either the pixel barrel or the pixel endcap as the root) to the active surfaces. The assembly of these
     * path strings from the available topology information is delegated to a private function. Once that returns, formatting
     * and output to file are taken care of in here.
     * @param t A reference to the collection of tracker topology information
     * @param in A reference to a file stream that is bound to the input file
     * @param out A reference to a file stream that is bound to the output file
     */
    void XMLWriter::recomaterial(std::vector<SpecParInfo>& t,
            std::vector<RILengthInfo>& ri, std::ifstream& in, std::ofstream& out, bool wt) {
        std::vector<PathInfo> b;
        b = buildPaths(t, b, wt);
        if (!b.empty()) {
            std::string line;
            while (std::getline(in, line) && (line.find(xml_insert_marker) == std::string::npos)) out << line << std::endl;
            std::vector<PathInfo>::iterator iter, guard = b.end();
            for (iter = b.begin(); iter != guard; iter++) {
                unsigned int id;
                for (id = 0; id < ri.size(); id++) {
                    if ((ri.at(id).index == iter->layer) && (ri.at(id).barrel == iter->barrel)) break;
                }
                if ((!iter->block_name.empty()) && (!iter->paths.empty())) {
                    std::vector<std::string>::iterator iiter, iguard = iter->paths.end();
                    out << xml_spec_par_open << iter->block_name << xml_eval_true;
                    for (iiter = iter->paths.begin(); iiter != iguard; iiter++) {
                        out << xml_spec_par_selector << *iiter << xml_general_endline;
                    }
                    if (id < ri.size()) {
                        out << xml_spec_par_parameter_first << xml_recomat_radlength << xml_spec_par_parameter_second;
                        out << ri.at(id).rlength << xml_general_endline << xml_spec_par_parameter_first << xml_recomat_xi;
                        out << xml_spec_par_parameter_second << ri.at(id).ilength;
                    }
                    else {
                        std::cerr << "WARNING: no RILengthInfo entry found for SpecPar block " << iter->block_name;
                        std::cerr << " in XMLWriter::recomaterial(). Using default dummy values." << std::endl;
                        out << xml_recomat_parameters;
                    }
                    out << xml_spec_par_close << std::endl;
                }
            }
            while (std::getline(in, line)) out << line << std::endl;
        }
    }
    
    //protected
    /**
     * This function writes the opening and closing tags for a material section in a CMSSW XML file. It also loops through
     * the list of elementary materials and that of the composites to generate one entry each for the material section. Actual
     * XML formatting of those list elements is left to two other functions, though. All generated output is sent to an
     * <i>ostringstream</i> that serves as a buffer for the output file contents.
     * @param name The label of the material section, typically the name of the output file
     * @param e A reference to the vector containing a series of elementary material definitions
     * @param c A reference to the vector containing a series of composite material definitions
     * @param stream A reference to the output buffer
     */
    void XMLWriter::materialSection(std::string name , std::vector<Element>& e, std::vector<Composite>& c, std::ostringstream& stream) {
        stream << xml_material_section_open << name << xml_general_inter;
        for (unsigned int i = 0; i < e.size(); i++) elementaryMaterial(e.at(i).tag, e.at(i).density, e.at(i).atomic_number, e.at(i).atomic_weight, stream);
        for (unsigned int i = 0; i < c.size(); i++) compositeMaterial(c.at(i).name, c.at(i).density, c.at(i).method, c.at(i).elements, stream);
        stream << xml_material_section_close;
    }
    
    /**
     * This function writes the opening and closing tags for a rotation section in a CMSSW XML file, if such a block is
     * necessary. It also loops through the list of rotations, but leaves XML formatting of the individual entries to another
     * function. All generated output is sent to an <i>ostringstream</i> that serves as a buffer for the output file contents.
     * @param r A reference to the vector containing a series of rotation definitions
     * @param label The label of the rotation section, typically the name of the output file
     * @param stream A reference to the output buffer
     */
    void XMLWriter::rotationSection(std::vector<Rotation>& r, std::string label, std::ostringstream& stream) {
        if (!r.empty()) {
            stream << xml_rotation_section_open << label << xml_general_inter;
            for (unsigned int i = 0; i < r.size(); i++)
                rotation(r.at(i).name, r.at(i).thetax, r.at(i).phix, r.at(i).thetay, r.at(i).phiy, r.at(i).thetaz, r.at(i).phiz, stream);
            stream << xml_rotation_section_close;
        }
    }
    
    /**
     * This function writes the opening and closing tags for the logical part section in a CMSSW XML file that describes
     * a volume hierachy. It writes an entry for the root volume <i>Tracker</i> before looping through the list of logical
     * volumes within it. XML formatting of the those entries is left to another function, though. All generated output is sent
     * to an <i>ostringstream</i> that serves as a buffer for the output file contents.
     * @param l A reference to the vector containing a series of logical volume definitions
     * @param label The label of the logical part section, typically the name of the output file
     * @param stream A reference to the output buffer
     */
    void XMLWriter::logicalPartSection(std::vector<LogicalInfo>& l, std::string label, std::ostringstream& stream, bool wt) {
        std::vector<LogicalInfo>::const_iterator iter, guard = l.end();
        stream << xml_logical_part_section_open << label << xml_general_inter;
        if (!wt) logicalPart(xml_tracker, xml_fileident + ":" + xml_tracker, xml_material_air, stream);
        for (iter = l.begin(); iter != guard; iter++) logicalPart(iter->name_tag, iter->shape_tag, iter->material_tag, stream);
        stream << xml_logical_part_section_close;
    }
    
    /**
     * This function writes the opening and closing tags for the solid section in a CMSSW XML file. It writes an entry for the
     * root volume <i>Tracker</i> before looping through the list of physical shapes within it. XML formatting of all entries
     * is left to another function, though. All generated output is sent to an <i>ostringstream</i> that serves as a buffer for
     * the output file contents.
     * @param s A reference to the vector containing a series of physical volume definitions
     * @param label The label of the solid section, typically the name of the output file
     * @param stream A reference to the output buffer
     */
    void XMLWriter::solidSection(std::vector<ShapeInfo>& s, std::string label, std::ostringstream& stream, bool notobtid, bool wt) {
        stream << xml_solid_section_open << label << xml_general_inter;
        if (!wt) tubs(xml_tracker, pixel_radius, outer_radius, max_length, stream);
        for (unsigned int i = 0; i < s.size(); i++) {
            if ((notobtid) &&
                    ((s.at(i).name_tag.compare(xml_tob) == 0) || (s.at(i).name_tag.compare(xml_tid) == 0))) continue;
            else {
                switch (s.at(i).type) {
                    case bx : box(s.at(i).name_tag, s.at(i).dx, s.at(i).dy, s.at(i).dz, stream);
                    break;
                    case tp : trapezoid(s.at(i).name_tag, s.at(i).dx, s.at(i).dy, s.at(i).dyy, s.at(i).dz, stream);
                    break;
                    case tb : tubs(s.at(i).name_tag, s.at(i).rmin, s.at(i).rmax, s.at(i).dz, stream);
                    break;
                    case pc : polycone(s.at(i).name_tag, s.at(i).rzup, s.at(i).rzdown, stream);
                    break;
                    default: std::cerr << "solidSection(): unknown shape type found. Using box." << std::endl;
                    box(s.at(i).name_tag, s.at(i).dx, s.at(i).dy, s.at(i).dz, stream);
                }
            }
        }
        stream << xml_solid_section_close;
    }
    
    /**
     * This function writes the opening and closing tags for the positioning section in a CMSSW XML file. It loops first through the
     * collection of explicit volume placements and then through those of the required placement algorithms, while leaving XML
     * formatting of the individual entries to two other functions. All generated output is sent to an <i>ostringstream</i> that serves
     * as a buffer for the output file contents.
     * @param p A reference to the vector containing a series of placement definitions
     * @param a A reference to the vector containing a series of algorithm names and parameters
     * @param label The label of the position section, typically the name of the output file
     * @param stream A reference to the output buffer
     */
    void XMLWriter::posPartSection(std::vector<PosInfo>& p, std::vector<AlgoInfo>& a, std::string label, std::ostringstream& stream) {
        std::vector<PosInfo>::iterator piter, pguard = p.end();
        std::vector<AlgoInfo>::iterator aiter, aguard = a.end();
        stream << xml_pos_part_section_open << label << xml_general_inter;
        for (piter = p.begin(); piter != pguard; piter++) posPart(piter->parent_tag, piter->child_tag, piter->rotref, piter->trans, piter->copy, stream);
        for (aiter = a.begin(); aiter != aguard; aiter++) algorithm(aiter->name, aiter->parent, aiter->parameters, stream);
        stream << xml_pos_part_section_close;
    }
    
    /**
     * This function writes the opening and closing tags for a section specifying additional parameters for various detector parts in a
     * CMSSW XML file. It also loops through the collection of parameter information but leaves formatting of the individual entries
     * to another function. All generated output is sent to an <i>ostringstream</i> that serves as a buffer for the output file contents.
     * @param t A reference to the collection of tracker topology information
     * @param label The label of the <i>SpecPar</i> section, typically the name of the output file
     * @param stream A reference to the output buffer
     */
    void XMLWriter::specParSection(std::vector<SpecParInfo>& t, std::string label, std::ostringstream& stream) {
        std::vector<SpecParInfo>::iterator titer, tguard = t.end();
        stream << xml_spec_par_section_open << label << xml_general_inter;
        for (titer = t.begin(); titer != tguard; titer++) specPar(titer->name, titer->parameter, titer->partselectors, stream);
        stream << xml_spec_par_section_close;
    }
    
    /**
     * This formatter writes an XML entry describing a call to a volume placement algorithm to the stream that serves as a
     * buffer for the output file contents.
     * @param name The name of the chosen algorithm as defined elsewhere in CMSSW
     * @param parent The name of the parent volume in which the duplicated volumes will be placed
     * @param params A pre-formatted list of arguments for the algorithm
     * @param stream A reference to the output buffer
     */
    void XMLWriter::algorithm(std::string name, std::string parent, std::vector<std::string>& params, std::ostringstream& stream) {
        stream << xml_algorithm_open << name << xml_algorithm_parent << parent << xml_general_endline;
        for (unsigned int i = 0; i < params.size(); i++) stream << params.at(i);
        stream << xml_algorithm_close;
    }
    
    /**
     * This formatter writes an XML entry describing an elementary material to the stream that serves as a buffer for the
     * output file contents.
     * @param tag The material name; must be unique
     * @param density The density of the element, in g/cm3
     * @param a_number The atomic number of the element
     * @param a_weight The atomic weight of the element, in g/mole
     * @param stream A reference to the output buffer
     */
    void XMLWriter::elementaryMaterial(std::string tag, double density, int a_number, double a_weight, std::ostringstream& stream) {
        stream << xml_elementary_material_open << tag << xml_elementary_material_first_inter << tag;
        stream << xml_elementary_material_second_inter << a_number << xml_elementary_material_third_inter;
        stream << a_weight << xml_elementary_material_fourth_inter << density;
        stream << xml_elementary_material_close;
    }
    
    /**
     * This formatter writes an XML entry describing a composite material to the stream that serves as a buffer for the
     * output file contents.
     * @param name The name of the composite material; must be unique
     * @param density The overall density of the composite material, in g/cm3
     * @param method An enumeration value denoting the material mixing method
     * @param es A reference to a list of elementary material names and their fractions in the composite mixture, stored in instances of <i>std::pair</i>
     * @param stream A reference to the output buffer
     */
    void XMLWriter::compositeMaterial(std::string name,
            double density, CompType method, std::vector<std::pair<std::string, double> >& es, std::ostringstream& stream) {
        stream << xml_composite_material_open << name << xml_composite_material_first_inter;
        stream << density << xml_composite_material_second_inter ;
        switch (method) {
            case wt : stream << "mixture by weight";
            break;
            case vl : stream << "mixture by volume";
            break;
            case ap : stream << "compound by atomic proportion";
            break;
            default: std::cerr << "tk2CMSSW::compositeMaterial(): unknown method identifier for composite material. Using mixture by weight." << std::endl;
            stream << "mixture by weight";
        }
        stream << xml_general_inter;
        for (unsigned int i = 0; i < es.size(); i++) {
            stream << xml_material_fraction_open << es.at(i).second << xml_material_fraction_inter;
            stream << xml_fileident << ":" << es.at(i).first << xml_material_fraction_close;
        }
        stream << xml_composite_material_close;
    }
    
    /**
     * This formatter writes an XML entry describing a logical volume to the stream that serves as a buffer for the
     * output file contents.
     * @param name The name of the logical volume; must be unique
     * @param solid The name of the physical shape entry that this logical volume describes further
     * @param material The name of the material that this volume is made of
     * @param stream A reference to the output buffer
     */
    void XMLWriter::logicalPart(std::string name, std::string solid, std::string material, std::ostringstream& stream) {
        stream << xml_logical_part_open << name << xml_logical_part_first_inter << solid;
        stream << xml_logical_part_second_inter << material << xml_logical_part_close;
    }
    
    /**
     * This formatter writes an XML entry describing a box shape to the stream that serves as a buffer for the output
     * file contents.
     * @param name The name of the box shape; must be unique
     * @param dx Half the volume length along x
     * @param dy Half the volume length along y
     * @param dz Half the volume length along z
     * @param stream A reference to the output buffer
     */
    void XMLWriter::box(std::string name, double dx, double dy, double dz, std::ostringstream& stream) {
        stream << xml_box_open << name << xml_box_first_inter << dx << xml_box_second_inter << dy;
        stream << xml_box_third_inter << dz << xml_box_close;
    }
    
    /**
     * This formatter writes an XML entry describing an isosceles trapezium shape to the stream that serves as a buffer
     * for the output file contents.
     * @param name The name of the trapezium shape; must be unique
     * @param dx Half the volume length along x
     * @param dy Half the volume length along the lower y
     * @param dyy Half the volume length along the upper y
     * @param dz Half the volume length along z
     * @param stream A reference to the output buffer
     */
    void XMLWriter::trapezoid(std::string name, double dx, double dy, double dyy, double dz, std::ostringstream& stream) {
        stream << xml_trapezoid_open << name << xml_trapezoid_first_inter << dx;
        stream << xml_trapezoid_second_inter << dx << xml_trapezoid_third_inter << dy;
        stream << xml_trapezoid_fourth_inter << dyy << xml_trapezoid_fifth_inter << dz;
        stream << xml_trapezoid_close;
    }
    
    /**
     * This formatter writes an XML entry describing a tube shape to the stream that serves as a buffer for the output
     * file contents.
     * @param name The name of the tube shape; must be unique
     * @param rmin The inner radius of the tube
     * @param rmax The outer radius of the tube
     * @param dz Half the length of the tube
     * @param stream A reference to the output buffer
     */
    void XMLWriter::tubs(std::string name, double rmin, double rmax, double dz, std::ostringstream& stream) {
        stream << xml_tubs_open << name << xml_tubs_first_inter << rmin << xml_tubs_second_inter << rmax;
        stream << xml_tubs_third_inter << dz << xml_tubs_close;
    }
    
    /**
     * This formatter writes an XML entry describing a polycone to the stream that serves as a buffer for the output
     * file contents. Since the list of points describing the polycone must be in the order in which they will be connected,
     * it is provided in two halves: one from the lowest possible starting point on the left side to the topmost point on the
     * same side, the other from the lowest possible starting point on the right side to the topmost point on the same side.
     * The two halved are then combined by looping through the two lists, in ascending order for the first and in descending
     * order for the second.
     * @param name The name of the polycone; must be unique
     * @param rzu A reference to the list of ascending points in <i>r, z</i> coordinates
     * @param rzd A reference to the list of descending points in <i>r, z</i> coordinates
     * @param stream A reference to the output buffer
     */
    void XMLWriter::polycone(std::string name, std::vector<std::pair<double, double> >& rzu,
            std::vector<std::pair<double, double> >& rzd, std::ostringstream& stream) {
        stream << xml_polycone_open << name << xml_polycone_inter;
        for (unsigned int i = 0; i < rzu.size(); i++) {
            stream << xml_rzpoint_open << rzu.at(i).first << xml_rzpoint_inter << rzu.at(i).second << xml_rzpoint_close;
        }
        for (unsigned int i = rzd.size(); i > 0; i--) {
            stream << xml_rzpoint_open << rzd.at(i - 1).first << xml_rzpoint_inter << rzd.at(i - 1).second << xml_rzpoint_close;
        }
        stream << xml_polycone_close;
    }
    
    /**
     * This formatter writes an XML entry describing a volume placement in space to the stream that serves as a buffer for the
     * output file contents. Namely, a child volume is placed at its appropriate position within a parent volume. The coordinate
     * system used is that of the parent volume, so rotations and translations have to be given in this context.
     * @param parent The name of the logical part describing the parent volume
     * @param child The name of the logical part describing the child volume
     * @param rotref The name of a rotation that will be applied to the child volume; an empty string (the default) means none
     * @param trans A reference to a struct describing a translation that will be applied to the child volume
     * @param copy The number of the child volume copy allowing different copies of the same child to be identified; <i>starts at 1</i>
     * @param stream A reference to the output buffer
     */
    void XMLWriter::posPart(std::string parent, std::string child, std::string rotref, Translation& trans, int copy, std::ostringstream& stream) {
        stream << xml_pos_part_open << copy << xml_pos_part_first_inter << parent;
        stream << xml_pos_part_second_inter << child << xml_general_endline;
        if (!rotref.empty()) stream << xml_pos_part_third_inter << rotref << xml_general_endline;
        if (!(trans.dx == 0.0 && trans.dy == 0.0 && trans.dz == 0.0)) translation(trans.dx, trans.dy, trans.dz, stream);
        stream << xml_pos_part_close;
    }
    
    /**
     * This formatter writes an XML entry describing a rotation in 3D to the stream that servers as a buffer for the output file contents.
     * @param name The name of the rotation definition; must be unique
     * @param thetax The angle theta with respect to the x-axis
     * @param phix The angle phi with respect to the x-axis
     * @param thetay The angle theta with respect to the y-axis
     * @param phiy The angle phi with respect to the y-axis
     * @param thetaz The angle theta with respect to the z-axis
     * @param phiz The angle phi with respect to the z-axis
     * @param stream A reference to the output buffer
     */
    void XMLWriter::rotation(std::string name, double thetax, double phix,
            double thetay, double phiy, double thetaz, double phiz, std::ostringstream& stream) {
        stream << xml_rotation_open << name << xml_rotation_first_inter << thetax << xml_rotation_second_inter << phix;
        stream << xml_rotation_third_inter << thetay << xml_rotation_fourth_inter << phiy << xml_rotation_fifth_inter;
        stream << thetaz << xml_rotation_sixth_inter << phiz << xml_rotation_close;
    }
    
    /**
     * This formatter writes an XML entry describing a translation in three dimensions to the stream that serves as a buffer for
     * the output file contents.
     * @param x The displacement along the x axis
     * @param y The displacement along the y axis
     * @param z The displacement along the z axis
     * @param stream A reference to the output buffer
     */
    void XMLWriter::translation(double x, double y, double z, std::ostringstream& stream) {
        stream << xml_translation_open << x << xml_translation_first_inter << y << xml_translation_second_inter << z;
        stream << xml_translation_close;
    }
    
    /**
     * This formatter writes an XML entry describing an additional parameter and the detector parts it is relevant for to the stream
     * that serves as a buffer for the output file contents.
     * @param name The name of the <i>SpecPar</i> block; must be unique
     * @param param The name and value of the additional parameter, given as instances of <i>std::string</i> and packaged into a <i>std::pair</i>
     * @param partsel A list of logical volume names that the additional parameter applies to
     * @param stream A reference to the output buffer
     */
    void XMLWriter::specPar(std::string name, std::pair<std::string, std::string> param, std::vector<std::string>& partsel, std::ostringstream& stream) {
        stream << xml_spec_par_open << name << xml_general_inter;
        for (unsigned i = 0; i < partsel.size(); i++) {
            stream << xml_spec_par_selector << partsel.at(i) << xml_general_endline;
        }
        stream << xml_spec_par_parameter_first << param.first << xml_spec_par_parameter_second;
        stream << param.second << xml_spec_par_close;
    }
    
    //private
    /**
     * This function builds the topological path for every active surface from a collection of <i>SpecParInfo</i> instances.
     * @param specs The collection of topology information bundles
     * @param blocks A container for a string representation of <i>SpecPar</i> blocks and their <i>PartSelector</i> path entries
     * @return The completed collection of blocks in string representation
     */
    std::vector<PathInfo>& XMLWriter::buildPaths(std::vector<SpecParInfo>& specs, std::vector<PathInfo>& blocks, bool wt) {
        std::vector<PathInfo>::iterator existing;
        std::string prefix, postfix, spname;
        std::vector<std::string> paths, tpaths;
        int dindex, rindex, mindex, layer = 0;
        std::vector<PathInfo> tblocks;
        blocks.clear();
        //TOB
        rindex = findEntry(specs, xml_subdet_rod + xml_par_tail);
        mindex = findEntry(specs, xml_subdet_tobdet + xml_par_tail);
        if ((rindex >= 0) && (mindex >= 0)) {
            // rod loop
            for (unsigned int i = 0; i < specs.at(rindex).partselectors.size(); i++) {
                std::string rnumber, mnumber, plusminus;
                std::string& rcurrent = specs.at(rindex).partselectors.at(i);
                if ((rcurrent.size() > xml_plus.size())
                        && (rcurrent.substr(rcurrent.size() - xml_plus.size()).compare(xml_plus) == 0))
                    plusminus = rcurrent.substr(rcurrent.size() - xml_plus.size());
                if ((rcurrent.size() > xml_minus.size())
                        && (rcurrent.substr(rcurrent.size() - xml_minus.size()).compare(xml_minus) == 0))
                    plusminus = rcurrent.substr(rcurrent.size() - xml_minus.size());
                rnumber = rcurrent.substr(xml_rod.size());
                rnumber = rnumber.substr(0, rnumber.size() - plusminus.size());
                spname = xml_tob_prefix + xml_pixbar + xml_layer + rnumber;
                layer = atoi(rnumber.c_str());
                prefix = xml_pixbar + "/" + xml_layer + rnumber + "/";
                if (wt && (plusminus.length() > 0)) {
                    if ((plusminus.compare(xml_plus) == 0) || (plusminus.compare(xml_minus) == 0))
                        prefix = prefix + xml_layer + rnumber + plusminus + "/";
                }
                prefix = prefix + rcurrent;
                // module loop
                for (unsigned int j = 0; j < specs.at(mindex).partselectors.size(); j++) {
                    mnumber = specs.at(mindex).partselectors.at(j).substr(xml_barrel_module.size());
                    mnumber = mnumber.substr(0, mnumber.size() - xml_base_act.size());
                    mnumber = mnumber.substr(findNumericPrefixSize(mnumber) + xml_layer.size());
                    // matching layers
                    if (mnumber.compare(rnumber) == 0) {
                        postfix = specs.at(mindex).partselectors.at(j);
                        postfix = postfix.substr(0, postfix.size() - xml_base_act.size());
                        postfix = postfix + "/" + postfix + xml_base_waf + "/" + specs.at(mindex).partselectors.at(j);
                        paths.push_back(prefix + "/" + postfix);
                    }
                }
                existing = findEntry(spname, blocks);
                if (existing != blocks.end()) existing->paths.insert(existing->paths.end(), paths.begin(), paths.end());
                else {
                    PathInfo pi;
                    pi.block_name = spname;
                    pi.layer = layer;
                    pi.barrel = true;
                    pi.paths = paths;
                    blocks.push_back(pi);
                }
                paths.clear();
            }
        }
        //TID
        dindex = findEntry(specs, xml_subdet_wheel + xml_par_tail);
        rindex = findEntry(specs, xml_subdet_ring + xml_par_tail);
        if ((dindex >= 0) && (rindex >= 0)) {
            // disc loop
            for (unsigned int i = 0; i < specs.at(dindex).partselectors.size(); i++) {
                std::string& dcurrent = specs.at(dindex).partselectors.at(i);
                bool plus = specs.at(dindex).partextras.at(i) == xml_plus; // CUIDADO was : (dcurrent.size() >= xml_plus.size() && (dcurrent.substr(dcurrent.size() - xml_plus.size()).compare(xml_plus) == 0);
                std::string dnumber, rnumber;
                dnumber = dcurrent.substr(xml_disc.size()); 
                //CUIDADO if (plus) dnumber = dnumber.substr(0, dnumber.size() - xml_plus.size());
                //else dnumber = dnumber.substr(0, dnumber.size() - xml_minus.size());
                std::ostringstream index;
                index << (xml_reco_material_disc_offset + i / 2);
                layer = atoi(dnumber.c_str());
                spname = xml_tid_prefix + index.str();
                if (plus) spname = spname + xml_forward;
                else spname = spname + xml_backward;
                prefix = xml_pixfwd;
                //if (plus) prefix = xml_pixfwd_plus;
                //else prefix = xml_pixfwd_minus;
                prefix = prefix + "/" + dcurrent; // CUIDADO was: prefix + "/" + dcurrent  + "[" + index.str() +"]";
                // ring loop
                for (unsigned int j = 0; j < specs.at(rindex).partselectors.size(); j++) {
                    std::string compstr = specs.at(rindex).partselectors.at(j);
                    compstr = compstr.substr(compstr.size() - dnumber.size());
                    // matching discs
                   if (dnumber.compare(compstr) == 0) {
                        rnumber = specs.at(rindex).partselectors.at(j).substr(xml_ring.size());
                        rnumber = rnumber.substr(0, findNumericPrefixSize(rnumber));
                        postfix = xml_endcap_module + rnumber + xml_disc + dnumber;
                        postfix = postfix + "/" + postfix + xml_base_waf + "/" + postfix + xml_base_act;
                        postfix = specs.at(rindex).partselectors.at(j) + "/" + postfix;
                        if (plus) paths.push_back(prefix + "/" + postfix);
                        else tpaths.push_back(prefix + "/" + postfix);
                    }
                }
                if (plus) {
                    existing = findEntry(spname, blocks);
                }
                else {
                    existing = findEntry(spname, tblocks);
                }
                if (plus && (existing != blocks.end())) {
                    existing->paths.insert(existing->paths.end(), paths.begin(), paths.end());
                }
                else if (!plus && (existing != tblocks.end())) {
                    existing->paths.insert(existing->paths.end(), tpaths.begin(), tpaths.end());
                }
                else {
                    PathInfo pi;
                    pi.block_name = spname;
                    pi.layer = layer;
                    pi.barrel = false;
                    if (plus) {
                        pi.paths = paths;
                        blocks.push_back(pi);
                    }
                    else {
                        pi.paths = tpaths;
                        tblocks.push_back(pi);
                    }
                }
                paths.clear();
                tpaths.clear();
            }
        }
        blocks.insert(blocks.end(), tblocks.begin(), tblocks.end());
        return blocks;
    }
    
    /**
     * This function looks for the presence of endcaps in a topology.
     * @param specs The collection of topology information bundles
     * @return True if the provided topology representation has blocks indicating endcaps, false otherwise
     */
    bool XMLWriter::endcapsInTopology(std::vector<SpecParInfo>& specs) {
        for (unsigned int i = 0; i < specs.size(); i++) {
            if (specs.at(i).name.compare(xml_subdet_tiddet + xml_par_tail) == 0) return true;
        }
        return false;
    }
    
    /**
     * Given a string, this function finds the number of digits from the beginning of the string until the first non-numeric character.
     *
     * WARNING: due to the intricacies of atoi(), the function will return the wrong result for strings starting with the digit '0'
     *                     and for those starting with a '+' or '-' sign!!!
     *
     * @param s An arbitrary string
     * @return The number of numeric characters that make up the start of the string
     */
    int XMLWriter::findNumericPrefixSize(std::string s) {
        std::ostringstream st;
        int number;
        if (!s.empty()) {
            number = atoi(s.c_str());
            if (number == 0) return number;
            else {
                st << number;
                return st.str().size();
            }
        }
        return 0;
    }
    
    /**
     * This is a custom function to find an entry in a collection of <i>SpecParInfo</i> structs
     * @param specs The collection of available <i>SpecParInfo</i> instances
     * @param name The requested block name
     * @return The index of the matching entry in the collection; -1 if no such entry exists
     */
    int XMLWriter::findEntry(std::vector<SpecParInfo>& specs, std::string name) {
        int index = 0;
        while (index < (int)(specs.size())) {
            if (specs.at(index).name.compare(name) == 0) return index;
            index++;
        }
        return -1;
    }
    
    /**
     * This is a custom function to find the name of a <i>SpecPar</i> block in a nested string representation of a series of such blocks.
     * @param name The name of the requested <i>SpecPar</i> block
     * @param data The collection of available blocks
     * @return An iterator pointing to the matching entry, or to <i>data.end()</i> if no such entry exists
     */
    std::vector<PathInfo>::iterator XMLWriter::findEntry(std::string name, std::vector<PathInfo>& data) {
        std::vector<PathInfo>::iterator result = data.begin();
        std::vector<PathInfo>::iterator guard = data.end();
        while (result != guard) {
            if ((result->block_name).compare(name) == 0) return result;
            result++;
        }
        return result;
    }
}
