// 
// File:   Extractor.h
// Author: ndemaio
//
// Created on January 31, 2010, 3:33 AM
//

/**
 * @file Extractor.h
 * @brief This is the header file for the class that analyses a full tracker and material budget for export
 */

#ifndef _EXTRACTOR_H
#define	_EXTRACTOR_H

#include <tk2CMSSW_datatypes.h>
#include <tk2CMSSW_strings.h>
#include <set>
#include <cmath>
#include <sstream>
#include <Tracker.h>
#include <MaterialTable.h>
#include <MaterialBudget.h>
#include <MaterialObject.h>

namespace insur {
  /**
   * @class Extractor
   * @brief This class bundles the analysis functions that prepare an existing material budget and table for output to CMSSW XML.
   *
   * The only public function of the class receives the material budget and table that make up the input. The output goes into an instance
   * of a struct - provided as a reference - that bundles a series of vectors of internal datatypes. ons of internal data types. The analysis
   * results will be stored in those, ready to be formatted and written to file.
   */
  class Extractor {
    class LayerAggregator : public GeometryVisitor { // CUIDADO quick'n'dirty visitor-based adaptor to interface with legacy spaghetti code
      std::vector<Layer*> barrelLayers_;
      std::vector<Disk*> endcapLayers_;
    public:
      void visit(Layer& l) { barrelLayers_.push_back(&l); }
      void visit(Disk& d) { endcapLayers_.push_back(&d); }
      std::vector<Layer*>* getBarrelLayers() { return &barrelLayers_; }
      std::vector<Disk*>* getEndcapLayers() { return &endcapLayers_; }
    };
  public:
    void analyse(MaterialTable& mt, MaterialBudget& mb, CMSSWBundle& d, bool wt = false);
  protected:
    void analyseElements(MaterialTable&mattab, std::vector<Element>& elems);
    void analyseBarrelContainer(Tracker& t, std::vector<std::pair<double, double> >& up,
                                std::vector<std::pair<double, double> >& down);
    void analyseEndcapContainer(Tracker& t, std::vector<std::pair<double, double> >& up,
                                std::vector<std::pair<double, double> >& down);
    void analyseLayers(MaterialTable& mt, std::vector<std::vector<ModuleCap> >& bc, Tracker& tr, std::vector<Composite>& c,
                       std::vector<LogicalInfo>& l, std::vector<ShapeInfo>& s, std::vector<PosInfo>& p, std::vector<AlgoInfo>& a,
                       std::vector<Rotation>& r, std::vector<SpecParInfo>& t, std::vector<RILengthInfo>& ri, bool wt = false);
    void analyseDiscs(MaterialTable& mt, std::vector<std::vector<ModuleCap> >& ec, Tracker& tr, std::vector<Composite>& c,
                      std::vector<LogicalInfo>& l, std::vector<ShapeInfo>& s, std::vector<PosInfo>& p, std::vector<AlgoInfo>& a,
                      std::vector<Rotation>& r, std::vector<SpecParInfo>& t, std::vector<RILengthInfo>& ri, bool wt = false);
    void analyseBarrelServices(InactiveSurfaces& is, std::vector<Composite>& c, std::vector<LogicalInfo>& l, std::vector<ShapeInfo>& s,
                               std::vector<PosInfo>& p, std::vector<SpecParInfo>& t, bool wt = false);
    void analyseEndcapServices(InactiveSurfaces& is, std::vector<Composite>& c, std::vector<LogicalInfo>& l, std::vector<ShapeInfo>& s,
                               std::vector<PosInfo>& p, std::vector<SpecParInfo>& t, bool wt = false);
    void analyseSupports(InactiveSurfaces& is, std::vector<Composite>& c, std::vector<LogicalInfo>& l, std::vector<ShapeInfo>& s,
                         std::vector<PosInfo>& p, std::vector<SpecParInfo>& t, bool wt = false);
  private:
    Composite createComposite(std::string name, double density, MaterialProperties& mp, bool nosensors = false);
    std::vector<ModuleCap>::iterator findPartnerModule(std::vector<ModuleCap>::iterator i,
                                                       std::vector<ModuleCap>::iterator g, int ponrod, bool find_first = false);
    double findDeltaR(std::vector<Module*>::iterator start, std::vector<Module*>::iterator stop, double middle);
    double findDeltaZ(std::vector<Module*>::iterator start, std::vector<Module*>::iterator stop, double middle);
    int findSpecParIndex(std::vector<SpecParInfo>& specs, std::string label);
    double calculateSensorThickness(ModuleCap& mc, MaterialTable& mt);
    std::string stringParam(std::string name, std::string value);
    std::string numericParam(std::string name, std::string value);
    std::string vectorParam(double x, double y, double z);
    double compositeDensity(ModuleCap& mc, bool nosensors = false);
    double compositeDensity(InactiveElement& ie);
    double fromRim(double r, double w);
    int Z(double x0, double A);
  };

#if 1

  // For flipping one of sensors
  // This should be moved to tk2CMSSW_strings.h at some point.
  static const std::string rot_sensor_tag = "SensorFlip";

  class HybridVolumes {
    public :
     HybridVolumes(std::string moduleName, ModuleCap& modcap);
     ~HybridVolumes();
     void buildVolumes();
     void addShapeInfo   (std::vector<ShapeInfo>&   vec);
     void addLogicInfo   (std::vector<LogicalInfo>& vec);
     void addPositionInfo(std::vector<PosInfo>&     vec);
     void addMaterialInfo(std::vector<Composite>&   vec);
     void print() const;

     const double getHybridWidth() const { return hybridWidth; }

    private :

      class Volume {
        public :
          Volume(std::string name, std::string pname, 
                 double dx,   double dy,   double dz,
                 double posx, double posy, double posz) : fname(name),
                                                          fparentname(pname),
                                                          fdx(dx),
                                                          fdy(dy),
                                                          fdz(dz),
                                                          fx(posx),
                                                          fy(posy),
                                                          fz(posz),
                                                          fdxyz(dx*dy*dz),
                                                          fdensity(-1.){}

          const double      getVolume()    const { return fdxyz; }
          const double      getDx()        const { return fdx;   }
          const double      getDy()        const { return fdy;   }
          const double      getDz()        const { return fdz;   }
          const double      getX()         const { return fx;    }
          const double      getY()         const { return fy;    }
          const double      getZ()         const { return fz;    }
          const std::string getName()      const { return fname; }
          const std::string getParentName()const { return fparentname; }

          double getDensity() const { return fdensity; }
          double setDensity( double density ) { fdensity = density; }
          void print() const { if (fdensity<0.) std::cout << "!!! Error : please call assignMassToVolumes beforehand to set the density." << std:: endl; 
                               else             std::cout << "   " << fname << " volume=" << fdxyz << " mm3, density=" << fdensity << " g/cm3" <<std::endl; }
        private :
          std::string  fname;
          std::string  fparentname;
          const double fdx,fdy,fdz;
          const double fx,fy,fz;
          const double fdxyz;
          double       fdensity;
      };

      ModuleCap&           modulecap;
      Module&              module;
      std::vector<Volume*> volumes;
      std::string          moduleId;
      const double         modThickness;
      const double         modWidth;  // Sensor width
      const double         modLength; // Sensor length
      const double         hybridWidth;
      const double         hybridThickness;
            double         hybridMass;
      const std::string    prefix_xmlfile;
      const std::string    prefix_material;
  };
#endif
}
#endif	/* _EXTRACTOR_H */

