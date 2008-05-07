#ifndef _MODULE_HH_
#define _MODULE_HH_

#include <vector>
#include <time.h>
#include "Math/Vector3D.h"
#include <string>
#include "TGeoManager.h"
#include "TPolyLine3D.h"

/*****************************/
/*                           */
/* Measurements are  in mm   */
/* It's simple to remember:  */
/* it's an  IS measurement!  */
/*                           */
/*****************************/

using namespace ROOT::Math;

typedef std::pair<double, int> edge;

class Module {

 protected:
  double waferDiameter_;
  
  // Shape-specific paramters
  double height_;
  double thickness_;
  double area_;
  double stripArea_;
  int nChannels_;
  int nSegments_;
  int nStripAcross_;

  int inSection_;
  
  //  point corner_[4];
  XYZVector corner_[4];

  double phi_; // A che serve?
  double r_;
  int nHits_;

  // Default variables
  static const double  defaultWaferDiameter_ = 131.; // Wafer diameter 131 mm
  static const double  defaultThickness_ = 0.3; // Wafer thickness: 300 um
  static const Color_t defaultColor_ = kRed;
  static const int     defaultNHits_ = 0;
  
  std::string id_;   // Ids of the module
  std::string tag_;  // Tags the module
  std::string type_; // Specifies the module type
  Color_t color_;
  
  // Returns the gamma parameter of the line if hits (complete line)
  double triangleCross(const XYZVector& P1, // Triangle points
		       const XYZVector& P2,
		       const XYZVector& P3,
		       const XYZVector& PL, // Base line point
		       const XYZVector& PU);

  TGeoVolume* thisVolume_;

  edge getEdgeRho(int direction);

  void computeStripArea();

 private:
  void setDefaultParameters();
    
 public:
  ~Module();
  Module();
  Module(double waferDiameter);
  
  void translate(XYZVector Delta);
  void rotatePhi(double phi);
  void rotateY_PI();

  XYZVector* marginBorder(double widthMargin, double lengthMargin, int corner );
  XYZVector* marginBorderSide(double margin, int side);

  void print();

  int projectSideZ(int side, double destZ, double displaceZ = 0);
  int projectSideRho(int side, double destRho, double displaceZ = 0);
  
  // Returns the distance if hits or -1
  double trackCross (const XYZVector& PL,  // Base line point
		     const XYZVector& PU); // Base direction

  void setId(const std::string newId) {id_=newId;}
  std::string getId() {return id_;}

  void setTag(const std::string newTag) {tag_=newTag;}
  std::string getTag() {return tag_;}

  void setType(const std::string newType) {type_=newType;}
  std::string getType() {return type_;}

  void setColor(const Color_t newColor) {color_ = newColor;}
  Color_t getColor() {return color_;}

  void resetNHits() { nHits_ = defaultNHits_ ; };
  int getNHits() { return nHits_ ;};

  void shapeVolume(TGeoVolume* container,
		   TGeoMedium* medium,
		   TGeoManager* geom);

  TPolyLine3D* getContour();

  double getHeight() {return height_;};
  double getArea() {return area_;};
  double getDiameter() {return waferDiameter_; };

  edge getEdgeRhoSide(int direction);
  int setEdgeRho(double destRho, int direction);
  int setEdgeRhoSide(double destRho, int direction);

  double getMinTheta();
  double getMaxTheta();
  double getMeanTheta();
  XYZVector getMeanPoint();

  virtual std::string getSensorTag() {};

  int getSection() { return inSection_ ;};
  void setSection(const int newSection) {inSection_ = newSection; };

  int getNChannels()    { return nChannels_ ;};
  int getNStripAcross() { return nStripAcross_ ;};
  int getNSegments()    { return nSegments_ ;};

  //  void setNChannels(const int& newN) { nChannels_ = newN;};
  void setNStripAcross(const int& newN) { nStripAcross_=newN; nChannels_=nStripAcross_*nSegments_;  };
  void setNSegments(const int& newN) { nSegments_=newN; nChannels_=nStripAcross_*nSegments_;  };


  double getLowPitch();
  double getHighPitch();

  double getOccupancyPerEvent();
};


// Barrel Module

class BarrelModule : public Module {

 private:
  void setSensorGeometry(double heightOverWidth);
  edge getEdgeZ(int direction, double margin = 0);
  edge getEdgePhi(int direction, double margin = 0);
  double width_;
  
 public:
  ~BarrelModule();
  BarrelModule(double waferDiameter, double heightOverWidth);
  BarrelModule(double heightOverWidth=1);

  edge getEdgeZSide(int direction, double margin = 0);
  int setEdgeZ(double newZ, int direction);

  edge getEdgePhiSide(int direction, double margin = 0);
  int setEdgePhi(double newPhi, int direction);

  double getMaxPhi();
  double getMinPhi();
  double getWidth() {return width_;};

  std::string getSensorTag();
};


class EndcapModule : public Module {

private:
  double phiWidth_;
  double widthLo_;
  double widthHi_;
  double dist_;
  bool cut_;
  double lost_; // lost millimeters in height because of cut
  void setSensorGeometry(double alpha, double d, double maxRho = -1);
  

public:
  ~EndcapModule();
  // The constructor needs 
  // alpha: the angle covered by the module
  // d: the dstance of the module's base from the z axis
  EndcapModule(double alpha, double d, double maxRho = -1);
  EndcapModule(const Module& sampleModule, double alpha, double d, double maxRho = -1);

  std::string getSensorTag();

  bool wasCut() {return cut_ ; };
  double getLost(){if (cut_) return lost_; return 0;};
  double getDist(){return dist_;};
};


#endif
