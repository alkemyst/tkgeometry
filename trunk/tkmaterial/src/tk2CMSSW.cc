/**
 * @file tk2CMSSW.cc
 * @brief
 */

#include <tk2CMSSW.h>
namespace insur {
    // public
    void tk2CMSSW::translate(MaterialTable& mt, MaterialBudget& mb, std::string outsubdir) {
        std::string outpath = default_xmlpath;
        if (outsubdir.empty()) outpath = outpath + "/" + default_xml;
        else {
            if (outsubdir.at(0) == '/') outpath = outpath + outsubdir;
            else outpath = outpath + "/" + outsubdir;
        }
        if(outpath.at(outpath.size() - 1) != '/') outpath = outpath + "/";
        // analyse tracker system and build up collection of elements, composites, hierarchy, shapes and position
        analyse(mt, mb, elements, composites, logic, shapes, positions);
        std::ostringstream buffer;
        buffer << xml_preamble;
        materialSection(xml_trackerfile, elements, composites, buffer);
        logicalPartSection(logic, xml_trackerfile, buffer);
        solidSection(shapes, xml_trackerfile, buffer);
        posPartSection(positions, xml_trackerfile, buffer);
        //TODO: find out if algorithm() calls happen here or within posPartSection()
        buffer << xml_defclose;
        // write results to top-level file
        bfs::remove_all(outpath.c_str());
        bfs::create_directory(outpath);
        std::ofstream outstream((outpath + xml_trackerfile).c_str());
        outstream << buffer.str() << std::endl;
        outstream.close();
        std::cout << "CMSSW XML output has been written to " << outpath << xml_trackerfile << std::endl;
    }
    
    // protected
    void tk2CMSSW::materialSection(std::string name , std::vector<Element>& e, std::vector<Composite>& c, std::ostringstream& stream) {
        stream << xml_material_section_open << name << xml_material_section_inter;
        for (unsigned int i = 0; i < e.size(); i++) elementaryMaterial(e.at(i).tag, e.at(i).density, e.at(i).atomic_number, e.at(i).atomic_weight, stream);
        for (unsigned int i = 0; i < c.size(); i++) compositeMaterial(c.at(i).name, c.at(i).density, c.at(i).method, c.at(i).elements, stream);
        stream << xml_material_section_close;
    }
    
    void tk2CMSSW::logicalPartSection(std::vector<LogicalInfo>& l, std::string label, std::ostringstream& stream) {
        std::vector<LogicalInfo>::const_iterator iter, guard = l.end();
        stream << xml_logical_part_section_open << label << xml_logical_part_section_inter;
        for (iter = l.begin(); iter != guard; iter++) logicalPart(iter->name_tag, iter->shape_tag, iter->material_tag, stream);
        stream << xml_logical_part_section_close;
    }
    
    void tk2CMSSW::solidSection(std::vector<ShapeInfo>& s, std::string label, std::ostringstream& stream) {
        std::vector<ShapeInfo>::const_iterator iter, guard = s.end();
        stream << xml_solid_section_open << label << xml_solid_section_inter;
        for (iter = s.begin(); iter != guard; iter++) {
            switch (iter->type) {
                case bx : box(iter->name_tag, iter->dx, iter->dy, iter->dz, stream);
                break;
                case tp : trapezoid(iter->name_tag, iter->dx, iter->dxx, iter->dy, iter->dz, stream);
                break;
                case tb : tubs(iter->name_tag, iter->rmin, iter->rmax, iter->dz, stream);
                break;
                default: std::cerr << "solidSection(): unknown shape type found. Using box." << std::endl;
                box(iter->name_tag, iter->dx, iter->dy, iter->dz, stream);
            }
        }
        stream << xml_solid_section_close;
    }
    
    void tk2CMSSW::posPartSection(std::vector<PosInfo>& p, std::string label, std::ostringstream& stream) {
        std::vector<PosInfo>::iterator iter, guard = p.end();
        stream << xml_pos_part_section_open << label << xml_pos_part_section_inter;
        for (iter = p.begin(); iter != guard; iter++) posPart(iter->parent_tag, iter->child_tag, iter->rot, iter->trans, 1, stream);
        //TODO: position every single volume relative to its parent using a series of posPart() calls
        //            IMPORTANT: find out the role of algorithm() in reducing the number of posPart() calls
        //                                    assemble parameters in String, Numeric and Vector blocks here and collect them
        //                                    in a vector using an ostringstream os for conversions (declared within the loop)
        //                                    String: os << xml_algorithm_string << name << xml_algorithm_value << value << xml_algorithm_endl;
        //                                    Numeric: os << xml_algorithm_numeric << name << xml_algorithm_value << value << xml_algorithm_endl;
        //                                    Vector: os << xml_algorithm_vector_open << name << xml_algorithm_vector_first_inter << type;
        //                                                os << xml_algorithm_vector_second_inter << entries << xml_algorithm_vector_third_inter;
        //                                                if (!vector.empty()) os << vector.at(0);
        //                                                for (unsigned int i = 1; i < vector.size(); i++) os << "," << vector.at(i);
        //                                                os << xml_algorithm_vector_close; (see xmltest.cpp for code)
        stream << xml_pos_part_section_close;
    }
    
    void tk2CMSSW::algorithm(std::string name, std::string parent,
            std::vector<std::string>& params, std::ostringstream& stream) {
        stream << xml_algorithm_open << name << xml_algorithm_parent << parent << xml_algorithm_endline;
        for (unsigned int i = 0; i < params.size(); i++) stream << params.at(i);
        stream << xml_algorithm_close;
    }
    
    void tk2CMSSW::elementaryMaterial(std::string tag, double density, int a_number,
            double a_weight, std::ostringstream& stream) {
        stream << xml_elementary_material_open << tag << xml_elementary_material_first_inter << tag;
        stream << xml_elementary_material_second_inter << a_number << xml_elementary_material_third_inter;
        stream << a_weight << xml_elementary_material_fourth_inter << density;
        stream << xml_elementary_material_close;
    }
    
    void tk2CMSSW::compositeMaterial(std::string name, double density, CompType method,
            std::vector<std::pair<std::string, double> >& es, std::ostringstream& stream) {
        stream << xml_composite_material_open << name << xml_composite_material_first_inter;
        stream << density << xml_composite_material_second_inter ;
        switch (method) {
            case wt : stream << "mixture by weight";
            break;
            case vl : stream << "mixture by volume";
            break;
            case ap : stream << "mixture by atomic proportion";
            break;
            default: std::cerr << "tk2CMSSW::compositeMaterial(): unknown method identifier for composite material. Using mixture by weight." << std::endl;
            stream << "mixture by weight";
        }
        stream << xml_composite_material_third_inter;
        for (unsigned int i = 0; i < es.size(); i++) {
            stream << xml_material_fraction_open << es.at(i).second << xml_material_fraction_inter;
            stream << xml_fileident << ":" << es.at(i).first << xml_material_fraction_close;
        }
        stream << xml_composite_material_close;
    }
    
    void tk2CMSSW::logicalPart(std::string name, std::string solid, std::string material, std::ostringstream& stream) {
        stream << xml_logical_part_open << name << xml_logical_part_first_inter << solid;
        stream << xml_logical_part_second_inter << material << xml_logical_part_close;
    }
    
    void tk2CMSSW::box(std::string name, double dx, double dy, double dz, std::ostringstream& stream) {
        stream << xml_box_open << name << xml_box_first_inter << dx << xml_box_second_inter << dy;
        stream << xml_box_third_inter << dz << xml_box_close;
    }
    
    void tk2CMSSW::trapezoid(std::string name, double dx, double dxx, double dy, double dz, std::ostringstream& stream) {
        stream << xml_trapezoid_open << name << xml_trapezoid_first_inter << dx;
        stream << xml_trapezoid_second_inter << dxx << xml_trapezoid_third_inter << dy;
        stream << xml_trapezoid_fourth_inter << dy << xml_trapezoid_fifth_inter << dz;
        stream << xml_trapezoid_close;
    }
    
    void tk2CMSSW::tubs(std::string name, double rmin, double rmax, double dz, std::ostringstream& stream) {
        stream << xml_tubs_open << name << xml_tubs_first_inter << rmin << xml_tubs_second_inter << rmax;
        stream << xml_tubs_third_inter << dz << xml_tubs_close;
    }
    
    void tk2CMSSW::posPart(std::string parent, std::string child, Rotation& rot, Translation& trans, int copy, std::ostringstream& stream) {
        stream << xml_pos_part_open << copy << xml_pos_part_first_inter << parent;
        stream << xml_pos_part_second_inter << child << xml_pos_part_third_inter;
        if (!rot.name.empty()) rotation(rot.name, rot.phix, rot.phiy, rot.phiz, rot.thetax, rot.thetay, rot.thetaz, stream);
        if (!(trans.dx == 0.0 && trans.dy == 0.0 && trans.dz == 0.0)) translation(trans.dx, trans.dy, trans.dz, stream);
        stream << xml_pos_part_close;
    }
    
    void tk2CMSSW::rotation(std::string name, double phix, double phiy, double phiz,
            double thetax, double thetay, double thetaz, std::ostringstream& stream) {
        stream << xml_rotation_open << name << xml_rotation_first_inter << phix << xml_rotation_second_inter << phiy;
        stream << xml_rotation_third_inter << phiz << xml_rotation_fourth_inter << thetax << xml_rotation_fifth_inter;
        stream << thetay << xml_rotation_sixth_inter << thetaz << xml_rotation_close;
    }
    
    void tk2CMSSW::translation(double x, double y, double z, std::ostringstream& stream) {
        stream << xml_translation_open << x << xml_translation_first_inter << y << xml_translation_second_inter << z;
        stream << xml_translation_close;
    }
    
    // private
    void tk2CMSSW::analyse(MaterialTable& mt, MaterialBudget& mb, std::vector<Element>& e, std::vector<Composite>& c,
            std::vector<LogicalInfo>& l, std::vector<ShapeInfo>& s, std::vector<PosInfo>& p) {
        Tracker& tr = mb.getTracker();
        InactiveSurfaces& is = mb.getInactiveSurfaces();
        std::vector<std::vector<ModuleCap> >& bc = mb.getBarrelModuleCaps();
        std::vector<std::vector<ModuleCap> >& ec = mb.getEndcapModuleCaps();
        std::vector<std::vector<ModuleCap> >::iterator oiter, oguard;
        std::vector<ModuleCap>::iterator iiter, iguard;
        std::vector<InactiveElement>::iterator iter, guard;
        int counter, layer;
        e.clear();
        c.clear();
        l.clear();
        s.clear();
        p.clear();
        // inits
        ShapeInfo shape;
        LogicalInfo logic;
        PosInfo pos;
        pos.trans.dx = 0.0;
        pos.trans.dy = 0.0;
        pos.trans.dz = 0.0;
        pos.rot.name = "";
        pos.rot.phix = 0.0;
        pos.rot.phiy = 0.0;
        pos.rot.phiz = 0.0;
        pos.rot.thetax = 0.0;
        pos.rot.thetay = 0.0;
        pos.rot.thetaz = 0.0;
        // define top-level volume and hierarchy root
        shape.name_tag = xml_tracker;
        shape.type = tb;
        shape.dx = 0.0;
        shape.dxx = 0.0;
        shape.dy = 0.0;
        shape.dz = max_length;
        shape.rmin = 0.0;
        shape.rmax = outer_radius + volume_width;
        s.push_back(shape);
        logic.name_tag = xml_tracker;
        logic.shape_tag = xml_fileident + ":" + xml_tracker;
        logic.material_tag = xml_material_air;
        l.push_back(logic);
        // translate entries in mt to elementary materials
        for (unsigned int i = 0; i < mt.rowCount(); i++) {
            Element elem;
            MaterialRow& r = mt.getMaterial(i);
            elem.tag = r.tag;
            elem.density = r.density;
            elem.atomic_weight = pow((r.ilength / 35.), 3); // magic!
            elem.atomic_number = Z(r.rlength, elem.atomic_weight);
            e.push_back(elem);
        }
        // b_mod: one composite for every module position on rod
        layer = 1;
        oguard = bc.end();
        for (oiter = bc.begin(); oiter != oguard; oiter++) {
            counter = 0;
            iguard = oiter->end();
            for (iiter = oiter->begin(); iiter != iguard; iiter++) {
                if (iiter->getModule().getRing() > counter) {
                    std::ostringstream matname;
                    matname << xml_base_actcomp << "L" << layer << "P" << iiter->getModule().getRing();
                    c.push_back(createComposite(matname.str(), compositeDensity(*iiter), *iiter));
                    counter++;
                }
            }
            layer++;
        }
        // e_mod: one composite for every ring
        layer = 1;
        oguard = ec.end();
        for (oiter = ec.begin(); oiter != oguard; oiter++) {
            counter = 0;
            iguard = oiter->end();
            for (iiter = oiter->begin(); iiter != iguard; iiter++) {
                if (iiter->getModule().getRing() > counter) {
                    std::ostringstream matname;
                    matname << xml_base_actcomp << "D" << layer << "R" << iiter->getModule().getRing();
                    c.push_back(createComposite(matname.str(), compositeDensity(*iiter), *iiter));
                    counter++;
                }
            }
            layer++;
        }
        // all tubes from now on, only translations in z
        shape.type = tb;
        shape.dx = 0.0;
        shape.dxx = 0.0;
        shape.dy = 0.0;
        pos.trans.dx = 0.0;
        pos.trans.dz = 0.0;
        // b_ser, e_ser: one composite for every service volume on the z+ side
        // s, l and p: one entry per service volume
        std::vector<InactiveElement>& bs = is.getBarrelServices();
        std::vector<InactiveElement>& es = is.getEndcapServices();
        guard = bs.end();
        for (iter = bs.begin(); iter != guard; iter++) {
            std::ostringstream matname, shapename;
            matname << xml_base_serfcomp << iter->getCategory() << "R" << (int)(iter->getInnerRadius());
            shapename << xml_base_serf << "R" << (int)(iter->getInnerRadius()) << "Z" << (int)(fabs(iter->getZOffset()));
            if ((iter->getZOffset() + iter->getZLength()) > 0) c.push_back(createComposite(matname.str(), compositeDensity(*iter), *iter));
            shape.name_tag = shapename.str();
            shape.dz = iter->getZLength() / 2.0;
            shape.rmin = iter->getInnerRadius();
            shape.rmax = shape.rmin + iter->getRWidth();
            s.push_back(shape);
            logic.name_tag = shapename.str();
            logic.shape_tag = xml_fileident + ":" + shapename.str();
            logic.material_tag = xml_fileident + ":" + matname.str();
            l.push_back(logic);
            pos.parent_tag = xml_fileident + ":" + xml_tracker;
            pos.child_tag = logic.shape_tag;
            pos.trans.dz = iter->getZOffset() + shape.dz;
            p.push_back(pos);
        }
        guard = es.end();
        for (iter = es.begin(); iter != guard; iter++) {
            std::ostringstream matname, shapename;
            matname << xml_base_serfcomp << iter->getCategory() << "Z" << (int)(fabs(iter->getZOffset() + iter->getZLength() / 2.0));
            shapename << xml_base_serf << "R" << (int)(iter->getInnerRadius()) << "Z" << (int)(fabs(iter->getZOffset() + iter->getZLength() / 2.0));
            if ((iter->getZOffset() + iter->getZLength()) > 0) c.push_back(createComposite(matname.str(), compositeDensity(*iter), *iter));
            shape.name_tag = shapename.str();
            shape.dz = iter->getZLength() / 2.0;
            shape.rmin = iter->getInnerRadius();
            shape.rmax = shape.rmin + iter->getRWidth();
            s.push_back(shape);
            logic.name_tag = shapename.str();
            logic.shape_tag = xml_fileident + ":" + shapename.str();
            logic.material_tag = xml_fileident + ":" + matname.str();
            l.push_back(logic);
            pos.parent_tag = xml_fileident + ":" + xml_tracker;
            pos.child_tag = logic.shape_tag;
            pos.trans.dz = iter->getZOffset() + shape.dz;
            p.push_back(pos);
        }
        // b_sup, e_sup, o_sup, t_sup, u_sup: one composite per category
        // l, s and p: one entry per support part
        std::set<MaterialProperties::Category> found;
        std::set<MaterialProperties::Category>::iterator fres;
        std::vector<InactiveElement>& sp = is.getSupports();
        guard = sp.end();
        for (iter = sp.begin(); iter != guard; iter++) {
            std::ostringstream matname, shapename;
            matname << xml_base_lazycomp << iter->getCategory();
            shapename << xml_base_lazy << "R" << (int)(iter->getInnerRadius()) << "Z" << (int)(fabs(iter->getZOffset()));
            fres = found.find(iter->getCategory());
            if (fres == found.end()) {
                c.push_back(createComposite(matname.str(), compositeDensity(*iter), *iter));
                found.insert(iter->getCategory());
            }
            shape.name_tag = shapename.str();
            shape.dz = iter->getZLength() / 2.0;
            shape.rmin = iter->getInnerRadius();
            shape.rmax = shape.rmin + iter->getRWidth();
            s.push_back(shape);
            logic.name_tag = shapename.str();
            logic.shape_tag = xml_fileident + ":" + shapename.str();
            logic.material_tag = xml_fileident + ":" + matname.str();
            l.push_back(logic);
            pos.parent_tag = xml_fileident + ":" + xml_tracker;
            pos.child_tag = logic.shape_tag;
            if ((iter->getCategory() == MaterialProperties::o_sup) ||
                    (iter->getCategory() == MaterialProperties::t_sup)) pos.trans.dz = 0.0;
            else pos.trans.dz = iter->getZOffset() + shape.dz;
            p.push_back(pos);
        }
        //TODO: traverse collections in mb and fill up l, s and p
        //            find out how to handle <algorithm> blocks
        //FOR NOW: barrel only
        //            find out about DOWN configuration (inner barrel longer than outer one)
        //            => not necessarily a problem if going from tracker straight to layers
        //            find out about short layers and stacked layers
        //            => should be straightforward: treat stacked layers as two individual ones,
        //                  short layers have two copies instead of one and translations in +z and -z
        //loop through layers
        //        see notes for rest of hierarchy
        //loop through discs
        //        see XML files for hierarchy
        //DEBUG
        //print();
    }
    
    tk2CMSSW::Composite tk2CMSSW::createComposite(std::string name, double density, MaterialProperties& mp) {
        Composite comp;
        comp.name = name;
        comp.density = density;
        comp.method = wt;
        for (unsigned int i = 0; i < mp.localMassCount(); i++) {
            std::pair<std::string, double> p;
            p.first = mp.getLocalTag(i);
            p.second = mp.getLocalMass(i);
            comp.elements.push_back(p);
        }
        for (unsigned int i = 0; i < mp.exitingMassCount(); i++) {
            std::pair<std::string, double> p;
            p.first = mp.getExitingTag(i);
            p.second = mp.getExitingMass(i);
            bool found = false;
            std::vector<std::pair<std::string, double> >::iterator iter, guard = comp.elements.end();
            for (iter = comp.elements.begin(); iter != guard; iter++) {
                if (iter->first == p.first) {
                    found = true;
                    break;
                }
            }
            if (found) iter->second = iter->second + p.second;
            else comp.elements.push_back(p);
        }
        for (unsigned int i = 0; i < comp.elements.size(); i++) comp.elements.at(i).second = comp.elements.at(i).second / mp.getTotalMass();
        return comp;
    }
    
    double tk2CMSSW::compositeDensity(InactiveElement& ie) {
        double d = ie.getRWidth() + ie.getInnerRadius();
        d = d * d - ie.getInnerRadius() * ie.getInnerRadius();
        d = 1000 * ie.getTotalMass() / (PI * ie.getZLength() * d);
        return d;
    }
    
    double tk2CMSSW::compositeDensity(ModuleCap& mc) {
        double d = mc.getSurface() * mc.getModule().getThickness();
        d = 1000 * mc.getTotalMass() / d;
        return d;
    }
    
    int tk2CMSSW::Z(double x0, double A) {
        double d = 4 - 4 * (1.0 - 181.0 * A / x0);
        if (d > 0) return floor((sqrt(d) - 2.0) / 2.0 + 0.5);
        else return -1;
    }
    
    void tk2CMSSW::print() {
        std::cout << "tm2CMSSW internal status:" << std::endl;
        std::cout << "elements: " << elements.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < elements.size(); i++) {
            std::cout << "entry " << i << ": tag = " << elements.at(i).tag << ", density = " << elements.at(i).density << ", atomic number = ";
            std::cout << elements.at(i).atomic_number << ", atomic weight = " << elements.at(i).atomic_weight << std::endl;
        }
        std::cout << "composites: " << composites.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < composites.size(); i++) {
            std::cout << "entry " << i << ": name = " << composites.at(i).name << ", density = " << composites.at(i).density << ", method = ";
            switch (composites.at(i).method) {
                case wt: std::cout << "fraction by weight";
                break;
                case vl: std::cout << "fraction by volume";
                break;
                case ap: std::cout << "fraction by atomic proportion";
                break;
                default: std::cout << "unknown method";
            }
            std::cout << std::endl << "elements: ";
            std::vector<std::pair<std::string, double> >& elems = composites.at(i).elements;
            for (unsigned int j = 0; j < elems.size(); j++) std::cout << "(" << elems.at(j).first << ", " << elems.at(j).second << ") ";
            std::cout << std::endl;
        }
        std::cout << "logic: " << logic.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < logic.size(); i++) {
            std::cout << "name_tag = " << logic.at(i).name_tag << ", shape_tag = " << logic.at(i).shape_tag;
            std::cout << ", material_tag = " << logic.at(i).material_tag << std::endl;
        }
        std::cout << "shapes: " << shapes.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < shapes.size(); i++) {
            std::cout << "name_tag = " << shapes.at(i).name_tag << ", type = ";
            switch (shapes.at(i).type) {
                case bx: std::cout << "box, dx = " << shapes.at(i).dx << ", dy = " << shapes.at(i).dy << ", dz = ";
                std::cout << shapes.at(i).dz;
                break;
                case tb: std::cout << "tube, rmin = " << shapes.at(i).rmin << ", rmax = " << shapes.at(i).rmax;
                std::cout << ", dz = " << shapes.at(i).dz;
                break;
                case tp: std::cout << "trapezoid, dx = " << shapes.at(i).dx << ", dxx = " << shapes.at(i).dxx;
                std::cout << ", dy = " << shapes.at(i).dy << ", dz = " << shapes.at(i).dz;
                break;
                default: std::cout << "unknown shape";
            }
            std::cout << std::endl;
        }
        std::cout << "positions: " << positions.size() << " entries." << std::endl;
        for (unsigned int i = 0; i < positions.size(); i++) {
            std::cout << "parent_tag = " << positions.at(i).parent_tag << ", child_tag = " << positions.at(i).child_tag;
            std::cout << ", rotation = (" << (positions.at(i).rot.name.empty() ? "[no name]": positions.at(i).rot.name) << ", ";
            std::cout  << positions.at(i).rot.phix << ", " << positions.at(i).rot.phiy << ", " << positions.at(i).rot.phiz << ", ";
            std::cout << positions.at(i).rot.thetax << ", " << positions.at(i).rot.thetay << ", " << positions.at(i).rot.thetaz;
            std::cout << "), translation = (" << positions.at(i).trans.dx << ", " << positions.at(i).trans.dy << ", ";
            std::cout << positions.at(i).trans.dz << ")" << std::endl;
        }
    }
}
