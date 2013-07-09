#ifndef _LAYER_HH_
#define _LAYER_HH_

// Standard stuff
#include <vector>
#include <string>

// ROOT stuff
#include <Math/Vector3D.h>
#include <TGeoManager.h>

// Our stuff
#include <module.hh>
#include <messageLogger.h>

using namespace ROOT::Math;

typedef std::vector<Module* > ModuleVector;
typedef std::pair<int,double> LayerOption;

typedef std::vector<TiltedModuleSpecs> TiltedRodSpecs;

struct TiltedLayerSpecs {
  TiltedRodSpecs innerRod, outerRod;
  int numRods;
  bool valid() const { return numRods > 0 && innerRod.size() > 0 && outerRod.size() > 0; }
};



class Layer {

protected:
  // If you add another member variable, please update the copy
  // constructor accordingly
  ModuleVector moduleSet_;
  std::string layerName_;
  std::string containerName_;
  int containerId_;
  int layerIndex_;
  std::ostringstream tempString;

private:
  virtual void setDefaultParameters();

public:
  virtual ~Layer();
  Layer();
  void translate(XYZVector Delta);
  void rotatePhi(double phi) {/*TODO*/};
  void shapeVolume(TGeoVolume* container, TGeoMedium* medium, TGeoManager* geom);
  void shapeModuleVolumes(TGeoVolume* container, TGeoMedium* medium, TGeoManager* geom);
  ModuleVector* getModuleVector() { return &moduleSet_; }
  virtual Module* getSampleModule() { return NULL; }

  std::string getName() {return layerName_; };
  int getIndex() {return layerIndex_; };
  std::string getContainerName() {return containerName_; };
  int getContainerId() const { return containerId_; }
  void setName(const std::string& newName, const int& newIndex) { layerName_ = newName; layerIndex_ = newIndex; };
  void setContainerName(const std::string& newName ) { containerName_ = newName; };
  void setContainerId(int containerId) { containerId_ = containerId; }

  double getMaxZ();
  double getMinZ();
  double getMaxRho();
  double getMinRho();
  virtual double getTilt() { return 0.0; }
  virtual double getStartAngle() { return 0.0; }
  virtual double getMaxModuleThickness() { return 0.0; }

  int getNModules() const;

  virtual int cutOverEta(double etaCut) { return 0; }

  virtual void decreaseModCount(int ring) {}

  virtual int getRings() = 0;

  enum {NoSection = 0x0,
    XYSection = 0x1,
    YZSection = 0x2,
    Forward   = 0x4};

  // Directives
  enum {SHRINK  = -1,
    FIXED   = -2,
    ENLARGE = -3,
    AUTO    = -4};

  // Options
  enum {Stacked = 1};

  // Other constants
  enum {InvalidRadius = -1 }; // It needs to be negative

protected:
  int round(const double& x, const bool& odd);

};


class BarrelLayer : public Layer {
private:
  // If you add another member variable, please update the copy
  // constructor accordingly
  int nOfRods_, nModsOnString_, nModsOnStringZPlus_, nModsOnStringZMinus_;
  BarrelModule* sampleModule_;
  BarrelModule* getSampleModule() { return sampleModule_; }
  double averageRadius_;
  void setDefaultParameters(){ averageRadius_= InvalidRadius ;};

  // CUIDADO: DEPRECATED, TBR
  std::pair<double, int> computeRadius(const double& x,    // radius of the inner module
                                       const double& g,    // Gap between inner and outer
                                       const double& o,    // Needed overlap in mm
                                       const double& b,    // Module's half width
                                       const int& optimal, // wether to shrink or enlarge, fix or auto
                                       const int& base );
  // CUIDADO: NEW VERSION
  std::pair<double, int> computeRadius(double avgR,
                                       double bigD,
                                       double smallD,
                                       double dsDist,
                                       double modW,
                                       double overlap,
                                       int optimal,
                                       int sliceMods);

public: // placement strategy must be in a public section because it is decided by the caller of the method
  struct ModulePlacementStrategy {
    virtual double operator()(std::vector<double>& listZ,
                              double startZ,
                              std::pair<double,double> worstCaseRadii,
                              double bigDelta,
                              double smallDelta,
                              const vector<double>& dsDistances,
                              double modLengthZ,
                              double originDeltaZ,
                              double baseOverlapZ,
                              int parity,
                              int direction,
                              bool looseStartZ = false) const = 0; 
  };

  class PlaceWithMaxZ : public ModulePlacementStrategy {
    const double maxZ_;
  public:
    PlaceWithMaxZ(double maxZ) : maxZ_(maxZ) {}
    double operator()(std::vector<double>& listZ,
                      double startZ,
                      std::pair<double,double> worstCaseRadii,
                      double bigDelta,
                      double smallDelta,
                      const vector<double>& dsDistances,
                      double modLengthZ,
                      double originDeltaZ,
                      double baseOverlapZ,
                      int parity,
                      int direction,
                      bool looseStartZ = false) const;
  };

  class PlaceWithNumModules : public ModulePlacementStrategy {
    const int numModules_;
  public:
    PlaceWithNumModules(int numModules) : numModules_(numModules) {}
    double operator()(std::vector<double>& listZ,
                      double startZ,
                      std::pair<double,double> worstCaseRadii,
                      double bigDelta,
                      double smallDelta,
                      const vector<double>& dsDistances,
                      double modLengthZ,
                      double originDeltaZ,
                      double baseOverlapZ,
                      int parity,
                      int direction,
                      bool looseStartZ = false) const;
  };
private:

  std::pair<int, int> buildStringPair(ModuleVector& thisModuleSet,
                                      double averageRadius,
                                      std::pair<double,double> worstCaseRadii,
                                      double bigDelta,
                                      double smallDelta,
                                      const vector<double>& dsDistances,
                                      double baseOverlap,
                                      double zDelta,
                                      const ModulePlacementStrategy& computeListZ,
                                      int smallParity,
                                      BarrelModule* sampleModule);

  std::pair<int, int> buildStringPairRecursion(ModuleVector& thisModuleSet,
                                               double averageRadius,
                                               std::pair<double,double> worstCaseRadii,
                                               double bigDelta,
                                               double smallDelta,
                                               const vector<double>& dsDistances,
                                               double baseOverlap,
                                               double zDelta,
                                               double startZ,
                                               const ModulePlacementStrategy& computeListZ,
                                               int smallParity,
                                               int recursionCounter,
                                               BarrelModule* sampleModule);

  std::pair<int, int> buildMezzanineStringPair(ModuleVector& thisModuleSet,
                                               double averageRadius,
                                               std::pair<double,double> worstCaseRadii,
                                               double bigDelta,
                                               double smallDelta,
                                               const vector<double>& dsDistances,
                                               double baseOverlap,
                                               double zDelta,
                                               double startZ,
                                               const ModulePlacementStrategy& computeListZ,
                                               int smallParity,
                                               BarrelModule* sampleModule);

  ModuleVector buildTiltedString(const TiltedRodSpecs::const_iterator& begin, const TiltedRodSpecs::const_iterator& end, const BarrelModule* sampleModule, int side);

  // CUIDADO: DEPRECATED, TBR
  double layerRadius(const double& nMod,  // number of strings in a layer
                     const double& g,     // Gap between inner and outer
                     const double& o,     // Needed overlap in mm
                     const double& b);    // Module's half width
  // CUIDADO: NEW VERSION 
  double layerRadius(int numMods,
                     double bigD,
                     double smallD,
                     double dsDist,
                     double modW,
                     double overlap);
  // This function was tuned "by hand" (see code for comments)
  std::pair<double, int> layerPhi(double, double, double, double, double, double, int, int, bool);


public:
  ~BarrelLayer();
  BarrelLayer();
  BarrelLayer(BarrelLayer& sampleLayer);
  BarrelLayer(double waferDiameter, double heightOverWidth);
  BarrelLayer(double heightOverWidth);
  BarrelLayer(const BarrelModule& mySample);
  BarrelLayer(BarrelModule* mySample);

  // An optimization function to run prior of settling the geometry
  void neededModulesPlot(double smallDelta, // Half distance between modules in the same string
                         double bigDelta,   // String half gap
                         double o,          // overlap
                         double b,          // Module half width
                         int base);

  void buildLayer(double averageRadius,  // average radius for PHI ring construction
                  std::pair<double,double> worstCaseRadii, // radii for hermetic Z rod construction (if 0.0, 0.0 they're taken equal to the radius found by the ring construction routine, which is close to averageRadius)
                  double bigDelta, 
                  double smallDelta, 
                  const vector<double>& dsDistances,
                  double overlap, 
                  double safetyOrigin, 
                  const ModulePlacementStrategy& moduleStrategy, 
                  int pushDirection, 
                  int base,
                  bool stringSameParity,
                  BarrelModule* sampleModule,
                  int sectioned = NoSection,
                  double minZ = 0.);

  void buildTiltedLayer(const TiltedLayerSpecs& tiltlay, const BarrelModule* sampleModule);

  int cutOverEta(double etaCut);

  int getRods() { return nOfRods_; }
  int getModulesOnRod() { return nModsOnString_; }
  int getModulesOnRodSide(int zSide) { return zSide > 0 ? nModsOnStringZPlus_ : nModsOnStringZMinus_; }
  int getRings() { return nModsOnString_; }
  int getRingsSide(int zSide) { return zSide > 0 ? nModsOnStringZPlus_ : nModsOnStringZMinus_; }

  double getMaxZ(int direction);
  void compressToZ(double newMaxZ);
  void compressExceeding(double newMaxZ, double newMinZ);

  double computeAverageRadius();
  double getAverageRadius();
  void rotateY_PI();
  void reflectZ();
  void shiftRho(double Delta);

  double getMaxModuleThickness(); // CUIDADO WAS { if (sampleModule_ != NULL) return sampleModule_->getModuleThickness(); return 0.0; }
};


class EndcapLayer : public Layer {
private:
  int nOfRings_;
  std::vector<int> nModsOnRing_;
  EndcapModule* sampleModule_;
  EndcapModule* getSampleModule() { return sampleModule_; }
  double averageZ_;
  void setDefaultParameters(){averageZ_=0;};

  double solvex(double y);
  double gamma1(double x, double y, double r);
  double gamma2(double x, double y, double r);
  double Area(double x, double y, double r);
  double compute_l(double x, double y, double d);
  double compute_d(double x, double y, double l);

public:
  ~EndcapLayer();
  EndcapLayer();
  EndcapLayer(EndcapLayer& sampleLayer);
  EndcapLayer(const Module& mySample, double alpha, double d);
  EndcapLayer(double alpha, double d);
  EndcapLayer(const EndcapModule& mySample);
  EndcapLayer(const EndcapModule& mySample, double alpha, double d);

  void translateZ(const double& zShift);

  void buildSingleDisk(double minRadius,
                       double maxRadius,
                       double smallDelta, 
                       double bigDelta,
                       double diskZ, 
                       double overlap, 
                       double zError,
                       const std::vector<double>& dsDistances,
                       int phiSegments, 
                       bool oddSegments, bool alignEdges,
                       std::map<int, EndcapModule*> sampleModule, 
                       std::map<int, int> ringDirectives, 
                       std::map<int, double> ringGaps, 
                       int diskParity,
                       int sectioned = NoSection);

  void buildSingleDisk(int nRings,
                       double maxRadius,
                       double smallDelta, 
                       double bigDelta,
                       double diskZ, 
                       double overlap, 
                       double zError,
                       const std::vector<double>& dsDistances,
                       int phiSegments, 
                       bool oddSegments, bool alignEdges,
                       std::map<int, EndcapModule*> sampleModule, 
                       std::map<int, int> ringDirectives, 
                       std::map<int, double> ringGaps, 
                       int diskParity,
                       int sectioned = NoSection);

  double buildRing(double minRadius,
                   double smallDelta, 
                   double bigDelta, 
                   double diskZ, 
                   double overlap, 
                   int phiSegments,
                   bool oddSegments, bool alignEdges,
                   int nearDirection, 
                   EndcapModule* sampleModule,
                   int& numPlacedModules, // set to the number of modules placed in the ring
                   double maxRadius = -1,
                   int addModules = 0,
                   int sectioned = NoSection);

  int cutOverEta(double etaCut);

  int getRings() { return nOfRings_; }
  std::vector<int>& getModulesOnRing() { return nModsOnRing_; }
  virtual void decreaseModCount(int ring) { nModsOnRing_.at(ring)--; }

  void rotateY_PI();
  void reflectZ();
  double getAverageZ() {return averageZ_;};

  double getMaxModuleThickness();


};


#endif






