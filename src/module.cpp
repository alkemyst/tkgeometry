
#include <iostream>
#include <cmath>
#include "module.hh"
#include "global_constants.h"

#include "Math/RotationZ.h"
#include "Math/RotationX.h"
#include "Math/Vector3D.h"

#include "TMatrixD.h"
#include "TVectorD.h"

#include "TGeoManager.h"
#include "TGeoArb8.h"
#include "TGeoMatrix.h"

#include <boost/lexical_cast.hpp>

using namespace ROOT::Math;

/******************/
/*                */
/* Generic module */
/*                */
/******************/

Module::~Module() {
    
}

Module::Module() {
    waferDiameter_= defaultWaferDiameter_;
    setDefaultParameters();
}

Module::Module(double waferDiameter) {
    waferDiameter_ = waferDiameter;
    setDefaultParameters();
}

void Module::setDefaultParameters() {
    // See the header file for the values of these
    height_             = defaultHeight_;
    nHits_              = defaultNHits_;
    thickness_          = defaultThickness_;
    moduleThickness_.value    = defaultModuleThickness_;
    area_               = defaultArea_;
    stereodist_         = defaultStereoDist_;
    stereorot_          = defaultStereoRot_;
    id_                 = "NoId";
    tag_                = "";
    type_               = "NoType";
    thisVolume_         = NULL;
    color_              = defaultColor_;
    inSection_          = defaultInSection_;
    nStripAcross_       = defaultStripAcross_;
    nFaces_             = defaultFaces_;
    readoutType_        = Strip;
    readoutMode_        = Binary;
    computedBoundaries_ = false;
    shape_              = Undefined;
    aspectRatio_        = defaultAspectRatio_;
    resolutionRphi_     = defaultResolutionRphi_;
    resolutionY_        = defaultResolutionY_;
    moduleType_         = NULL;
    processorConnectionsEta_ = 0;
    processorConnectionsPhi_ = 0;
    irradiatedPowerConsumption_ = 0;
    nROCRows_ = 128;
    nROCCols_ = 1;

    for (int i=0;i<maxNFaces;++i) nSegmentsFace_[i] = defaultSegments_;

    zSide_ = 0;

    geometryLocked_ = false;
}

void Module::setNFaces(const int& newNFaces) {
  if (newNFaces<0) {
    std::cerr << "ERROR in Module::setNFaces: negative number of faces!" << std::endl;
    return;
  } else if (newNFaces > maxNFaces) {
    std::cerr << "ERROR in Module::setNFaces: number of faces (" << newNFaces
	      << ") exceeds maximum allowed ("
	      << maxNFaces << ")" << std::endl;
    return;
  } else {
    nFaces_=newNFaces;
  }
}

/**
 * Sets the same number of segments (for all the possible faces)
 * @param newNSegments number of segments
 */
void Module::setNSegments(const int& newNSegments) {
  for (int i=0;i<maxNFaces;++i)  nSegmentsFace_[i] = newNSegments;
}

/**
 * Sets the same number of segments for a given face
 * @param face number of the face (counting from 1)
 * @param newNSegments number of segments
 */
void Module::setNSegments(const int& face, const int& newNSegments) {
  if (face<=0) {
    std::cerr << "ERROR in Module::setNFaces: zero or negative face index" << std::endl;
    return;
  } else if (face > maxNFaces) {
    std::cerr << "ERROR in Module::setNSegments: face index (" << face
	      << ") exceeds maximum allowed ("
	      << maxNFaces << ")" << std::endl;
    return;
  } else if (face > nFaces_) {
    std::cerr << "WARNING in Module::setNSegments: face index (" << face
	      << ") exceeds number of faces ("
	      << maxNFaces << ")" << std::endl;
  }

  nSegmentsFace_[face-1] = newNSegments;
}

/**
 * Total number of channel in the module, taking into account
 * the different faces
 * @return total number of channels
 */
int Module::getNChannels() const {
  int countChannels=0;
  for (int i=0; i<nFaces_; ++i) {
    countChannels += nStripAcross_ * nSegmentsFace_[i] ;
  }
  return countChannels;
}

/** 
 * Computes on the fly the number of channels in a given face
 * @return number of channels in a given face
 */
int Module::getNChannelsFace(int nFace) const {
  if (nFace<=0) {
    std::cerr << "ERROR in Module::getNChannelsFace: zero or negative face index" << std::endl;
    return 0;
  } else if (nFace > maxNFaces) {
    std::cerr << "ERROR in Module::getNChannelsFace: face index (" << nFace
	      << ") exceeds maximum allowed ("
	      << maxNFaces << ")" << std::endl;
    return 0;
  } else if (nFace > nFaces_) {
    std::cerr << "WARNING in Module::getNChannelsFace: face index (" << nFace
	      << ") exceeds number of faces ("
	      << nFaces_ << ")" << std::endl;
  }
  return nStripAcross_ * nSegmentsFace_[nFace-1] ;
}

/**
 * Returns the number of channels on the sensor with the lowest number of channels
 * @return the minimum sensor number of channels in the module
 */ 
int Module::getNMinChannelsFace() {
  return nStripAcross_ * getNMinSegments();
}

/**
 * Returns the number of channels on the sensor with the highest number of channels
 * @return the maximum sensor number of channels in the module
 */ 
int Module::getNMaxChannelsFace() {
  return nStripAcross_ * getNMaxSegments();
}

/**
 * Returns the number of segments of the face with most segments
 * @return max number of segments
 */
int Module::getNMaxSegments() {
  int maxSegments=0;
  // TODO: make such that all these things are computed only once
  for (int i=0; i<nFaces_; ++i) {
    if (nSegmentsFace_[i]>maxSegments) {
      maxSegments = nSegmentsFace_[i];
    }
  }  
  return maxSegments;
}

/**
 * Returns the average number of segments on the faces
 * @return mean number of segments
 */
double Module::getNMeanSegments() {
  int maxSegments=0;
  // TODO: make such that all these things are computed only once
  for (int i=0; i<nFaces_; ++i) {
    maxSegments += nSegmentsFace_[i];
  }  
  return double(maxSegments)/nFaces_;
}

/**
 * Returns the minimum number of segments on the faces
 * @return min number of segments
 */
int Module::getNMinSegments() {
  int minSegments=nSegmentsFace_[0];
  // TODO: make such that all these things are computed only once
  for (int i=1; i<nFaces_; ++i) {
    if (nSegmentsFace_[i]<minSegments) {
      minSegments = nSegmentsFace_[i];
    }
  }  
  return minSegments;
}

/**
 * Computes on the fly the same number of segments for all the
 * possible faces)
 * @param newNSegments number of segments
 */
int Module::getNSegments(int nFace) {
  if ((nFace<=0)||(nFace>maxNFaces)) return 0;
  return nSegmentsFace_[nFace-1];
}

/**
 * Finds the ARRAY INDEX of the face with the most number of segments
 * counting from 0
 * @return the face index with most segments
 */
int Module::findMaxSegmentsFace_() {
  int maxSegments=0;
  int iMostSegments=0;
  // TODO: make such that all these things are computed only once
  for (int i=0; i<nFaces_; ++i) {
    if (nSegmentsFace_[i]>maxSegments) {
      maxSegments = nSegmentsFace_[i];
      iMostSegments = i;
    }
  }
  return iMostSegments;
}

/**
 * Finds the ARRAY INDEX of the face with the least number of segments
 * counting from 0
 * @return the face index with least segments
 */
int Module::findMinSegmentsFace_() {
  int minSegments=nSegmentsFace_[0];
  int iLeastSegments=0;
  // TODO: make such that all these things are computed only once
  for (int i=1; i<nFaces_; ++i) {
    if (nSegmentsFace_[i]<minSegments) {
      minSegments = nSegmentsFace_[i];
      iLeastSegments = i;
    }
  }
  return iLeastSegments;
}


/**
 * Computes on the fly the resolution of the module along the local Y
 * direction as the strip/sqrt(12) of the shortest strip in the module
 * The computed value is overridden if an explicit setResolutionY()
 * was previously called
 * @return the local Y resolution
 */
double Module::getResolutionY() {
 if (resolutionY_!=defaultResolutionY_) return resolutionY_;
 double result;
 // TODO: handle this correctly once and for all!

 // This is the standard stereo module:
 if (stereorot_!=0) {
   result = getResolutionRphi() / sin(stereorot_);
 } else { // Stereo angle is absent
   int iMostSegments = findMaxSegmentsFace_();
   result = height_ / (double)(nSegmentsFace_[iMostSegments]) / sqrt(12); 
 }

 return result;
}

/**
 * Returns the "trigger" Y precision as the standard one, divided by
 * an arbitrary factor
 * 
 * @return the Y resolution in trigger readout
 */
double Module::getResolutionYTrigger() {
  double result = getResolutionY();
  if (moduleType_!=NULL) {
    result *= moduleType_->getTriggerErrorY();
  }
  return result;
}

/**
 * Convenience function to copy the pt error parameters into the
 * ptError object. TODO: solve this in a more elegant way
 *
 * @returns the pt cut for the module as it is configured
 */
void Module::setPterrorParameters() {
  myPtError.setDistance( getStereoDistance() );
  int myModuleType = getSubdetectorType();
  myPtError.setModuleType( myModuleType );
  if (myModuleType == Module::Endcap) {
    myPtError.setEndcapType(shape_); // TODO: check if this is working properly
  }
  myPtError.setPitch((getLowPitch()+getHighPitch())/2.);
  // TODO: in the error computation the strip length is used twice:
  // Once to calculate the geometric inefficiency, and also to compute
  // the pt error due to the r uncertainty in the endcap module. It is
  // possible that for the second computation we might want to use the
  // small strip length (i.e. findMaxSegmentsFace). In this case the
  // object ptError should be changed to take into account this
  // possibility
  myPtError.setStripLength( height_ / (double)(nSegmentsFace_[findMinSegmentsFace_()]) );
  XYZVector center = getMeanPoint();
  myPtError.setZ(fabs(center.Z()));
  myPtError.setR(center.Rho());
  myPtError.setHeight(getHeight());
  myPtError.setInefficiencyType(moduleType_->getInefficiencyType());
}


/**
 * Returns the pT for which we reach a given trigger efficiency
 *
 * @param myEfficiency the threshold efficiency
 *
 * @returns the pT corresponding to the selected efficiency, if it is
 * possible and within the range, returns -1 else
 */
double Module::getPtThreshold(const double& myEfficiency) {
  setPterrorParameters();
  double pt_cut = myPtError.stripsToP(triggerWindow_/2.);
  return myPtError.find_probability(myEfficiency, pt_cut);
}

/**
 * Returns the probability for a track of given pT to be detected
 * as high-pt track by this module
 *
 * @param trackPt the track's transverse momentum
 * @param stereoDistance if defined: use this instead of teh actual module stereodist_
 *
 * @returns the probability of the hit to be detected as high-pt
 */
double Module::getTriggerProbability(const double& trackPt, const double& stereoDistance /*= 0*/, const int& triggerWindow /* = 0 */ ) {
  setPterrorParameters();
  if (stereoDistance!=0) myPtError.setDistance(stereoDistance);
  int thisTriggerWindow;
  if (triggerWindow!=0) thisTriggerWindow = triggerWindow;
  else thisTriggerWindow = triggerWindow_;
  double pt_cut = myPtError.stripsToP(thisTriggerWindow/2.);
  // Error on curvatre is the relative error of trackPt times the
  // curvature (cur = 1/pt)
  double cur_error = myPtError.computeError(trackPt) / trackPt; 
  double result;
  result = myPtError.probabilityInside(1/pt_cut, 1/trackPt, cur_error) * myPtError.geometricEfficiency();
  // std::cerr << "trigger prob @ " << trackPt << " GeV/c is " << result <<std::endl; // debug
  return result;
}

/**
 * Returns the pt cut for this module
 *
 * @returns the pt cut for the module as it is configured
 */
double Module::getPtCut() {
  setPterrorParameters();
  return myPtError.stripsToP(triggerWindow_/2.);
}

/** 
 * Uses the myPtError object to compute the trigger geometric efficiency
 *
 * @returns geometric efficiency
 */
double Module::getGeometricEfficiency() {
  setPterrorParameters();
  return myPtError.geometricEfficiency();
}

// TODO: better special case for PT modules
double BarrelModule::getResolutionRphi() {
  if (resolutionRphi_!=defaultResolutionRphi_) return resolutionRphi_;
  double result = width_ / (double)(nStripAcross_) / sqrt(12);
  if (readoutType_==Pt) result/=sqrt(2);
  if (readoutMode_==Cluster) result/=1.5;
  return result;
}

// TODO: better special case for PT modules
double EndcapModule::getResolutionRphi() {
  if (resolutionRphi_!=defaultResolutionRphi_) return resolutionRphi_;
  double result = (widthLo_ + widthHi_) / 2.0 / (double)(nStripAcross_) / sqrt(12);  
  if (readoutType_==Pt) result/=sqrt(2);
  if (readoutMode_==Cluster) result/=1.5;
  return result;
}

double BarrelModule::getResolutionRphiTrigger() {
  double result = getResolutionRphi();
  if (moduleType_!=NULL) {
    result *= moduleType_->getTriggerErrorX();
  }
  return result;
}

double EndcapModule::getResolutionRphiTrigger() {
  double result = getResolutionRphi();
  if (moduleType_!=NULL) {
    result *= moduleType_->getTriggerErrorX();
  }
  return result;
}

ostream& operator<<(ostream& output, const Module& m) {
  output << "("
	 << m.corner_[3].X() << ", "
	 << m.corner_[3].Y() << ", "
	 << m.corner_[3].Z() << ")\t\t"
	 << "("
	 << m.corner_[2].X() << ", "
	 << m.corner_[2].Y() << ", "
	 << m.corner_[2].Z() << ")" << std::endl
	 << "("
	 << m.corner_[0].X() << ", "
	 << m.corner_[0].Y() << ", "
	 << m.corner_[0].Z() << ")\t\t"
	 << "("
	 << m.corner_[1].X() << ", "
	 << m.corner_[1].Y() << ", "
	 << m.corner_[1].Z() << ")";
  return output;
}

ostream& operator<<(ostream& output, const PosRef& pr) {
  output << "[ "
         << "cnt=" << int(pr.cnt)
         << ", z=" << int(pr.z)
         << ", rho=" << int(pr.rho)
         << ", phi=" << int(pr.phi);
  return output;
}

void Module::print() {
  std::cout << this << std::endl;
}

void Module::translate(XYZVector Delta) {
    for (int i=0; i<4; i++) {
//     corner_[i].x+=Delta.x;
//     corner_[i].y+=Delta.y;
//     corner_[i].z+=Delta.z;
        corner_[i]+=Delta;
    }
}

void Module::rotatePhi(double phi) {
    RotationZ phiRot(phi);
    
    for (int i=0; i<4; i++) {
        corner_[i]=phiRot(corner_[i]);
        //    newx=corner_[i].x*cos(phi)-corner_[i].y*sin(phi);
        //    newy=corner_[i].x*sin(phi)+corner_[i].y*cos(phi);
        //    corner_[i].x=newx;
        //    corner_[i].y=newy;
    }
}

void Module::rotateX(double phi) {
    RotationX xRot(phi);
    
    for (int i=0; i<4; i++) {
        corner_[i]=xRot(corner_[i]);
    }
}

void Module::rotateY_PI() {
    zSide_ *= -1;
    for (int i=0; i<4; i++) {
        corner_[i].SetX(corner_[i].X()*-1.);
        corner_[i].SetZ(corner_[i].Z()*-1.);
    }
}

void Module::reflectZ() {
    zSide_ *= -1;
    for (int i=0; i<4; i++) {
        corner_[i].SetZ(corner_[i].Z()*-1.);
    }
}

void Module::shiftRho(double Delta) {
    XYZVector mean = getMeanPoint();
    double thisRho = mean.Rho();

    if (thisRho != 0.) {
    
        double newRho=thisRho+Delta;
        double factor=newRho/thisRho;
    
        XYZVector vDelta(mean.X()*(factor-1), mean.Y()*(factor-1), 0);
    
        for (int i=0; i<4; i++) {
            corner_[i] = corner_[i] + vDelta;
        }
    
    } else {
        
        XYZVector vDelta(0 , Delta, 0);
    
        for (int i=0; i<4; i++) {
            corner_[i] = corner_[i] + vDelta;
        }

    }
}


XYZVector* Module::marginBorderSide(double margin, int side){
    
    XYZVector* result;
    
    if ((side<0)||(side>3)) {
        std::cerr << "Module::marginBorderSide was invoked with side out of range" << std::endl;
        return NULL;
    };
    
    int otherSideIndex = (side+2) % 4;
    int nextPoint=(side+1) % 4;
    int otherNextPoint=(otherSideIndex+1) % 4;
    
    XYZVector thisSide=(corner_[side]+corner_[nextPoint])/2.;
    XYZVector otherSide=(corner_[otherSideIndex]+corner_[otherNextPoint])/2.;
    
    XYZVector delta=otherSide-thisSide;
    delta *= margin/delta.R();
    
    result = new XYZVector(thisSide + delta);
    
    return result;
}


XYZVector* Module::marginBorder(double widthMargin, double lengthMargin, int corner ) {
    
    XYZVector* result;
    
    if ((corner<0)||(corner>3)) {
        std::cerr << "Module::marginBorder was invoked with corner out of range" << std::endl;
        return NULL;
    };
    
    
    XYZVector *thisCorner, *neighbourWidthCorner, *neighbourLengthCorner;
    XYZVector deltaWidth, deltaLength;
    
    int parity=corner%2;
    int neighbourWidthIndex, neighbourLengthIndex;
    
    if (parity==0) {
        neighbourWidthIndex=corner+1;
        neighbourLengthIndex=corner-1;
    } else {
        neighbourWidthIndex=corner-1;
        neighbourLengthIndex=corner+1;
    }
    
    if (neighbourWidthIndex<0)neighbourWidthIndex+=4;
    if (neighbourLengthIndex<0)neighbourLengthIndex+=4;
    if (neighbourWidthIndex>3)neighbourWidthIndex-=4;
    if (neighbourLengthIndex>3)neighbourLengthIndex-=4;
    
    neighbourWidthCorner=&corner_[neighbourWidthIndex];
    neighbourLengthCorner=&corner_[neighbourLengthIndex];
    thisCorner=&corner_[corner];
    
    deltaWidth=(*neighbourWidthCorner)-(*thisCorner);
    deltaLength=(*neighbourLengthCorner)-(*thisCorner);
    
    deltaWidth*=(widthMargin/deltaWidth.R());
    deltaLength*=(lengthMargin/deltaLength.R());
    
    result=new XYZVector((*thisCorner)+deltaWidth+deltaLength);
    
    return result;
}


int Module::projectSideZ(int side, double destZ, double displaceZ /*=0*/ ) {
    if ((side<0)||(side>3)) {
        std::cerr << "Module::projectSideZ was invoked with a nonsense side index" << std::endl;
        return -1;
    }
    
    XYZVector* displacement = NULL;
    
    if (displaceZ!=0) {
        displacement = new XYZVector(0, 0, displaceZ);
        for (int i=0; i<4; i++) {
            corner_[i]-=(*displacement);
        }
        destZ-=displacement->Z();
    }
    
    
    int firstPoint=side;
    int secondPoint=(side+1) % 4;
    
//   std::cerr << "Refpoints:" << firstPoint << " " << secondPoint << std::endl;
    
    XYZVector refPoint = (corner_[firstPoint]+corner_[secondPoint])/2.;
    
//   std::cerr << "refPoint"
// 	    <<  refPoint.X() << " "
// 	    <<  refPoint.Y() << " "
// 	    <<  refPoint.Z()
// 	    << std::endl;
    
    double ratio = destZ/refPoint.Z();
    
//   std::cerr << "ratio:" << ratio << std::endl;
    
    XYZVector Delta = refPoint*(ratio-1);
    
//   std::cerr << "Delta:"
// 	    <<  Delta.X() << " "
// 	    <<  Delta.Y() << " "
// 	    <<  Delta.Z()
// 	    << std::endl;
    
    for (int i=0; i<4; i++) {
        corner_[i]+=Delta;
    }
    
    if (displacement!=NULL) {
        for (int i=0; i<4; i++) {
            corner_[i]+=(*displacement);
        }
        delete displacement;
    }
    
    return 0;
}

int Module::projectSideRho(int side, double destRho, double displaceZ /*=0*/ ) {
    if ((side<0)||(side>3)) {
        std::cerr << "Module::placeSideRho was invoked with a nonsense side index" << std::endl;
        return -1;
    }
    
    XYZVector* displacement = NULL;
    
    if (displaceZ!=0) {
        displacement = new XYZVector(0, 0, displaceZ);
        for (int i=0; i<4; i++) {
            corner_[i]-=(*displacement);
        }
        destRho-=displacement->Rho();
    }
    
    int firstPoint=side;
    int secondPoint=(side+1) % 4;
    
//   std::cerr << "Refpoints: " << firstPoint << "," << secondPoint << std::endl;
//   std::cerr << firstPoint << "= "
// 	    << corner_[firstPoint].X() << " "
// 	    << corner_[firstPoint].Y() << " "
// 	    << corner_[firstPoint].Z()
// 	    << std::endl;
//   std::cerr << secondPoint << "= "
// 	    << corner_[secondPoint].X() << " "
// 	    << corner_[secondPoint].Y() << " "
// 	    << corner_[secondPoint].Z()
// 	    << std::endl;
    
    
    XYZVector refPoint = (corner_[firstPoint]+corner_[secondPoint])/2.;
    
//   std::cerr << "refPoint= "
// 	    <<  refPoint.X() << " "
// 	    <<  refPoint.Y() << " "
// 	    <<  refPoint.Z()
// 	    << std::endl;
    
    
    double ratio = destRho/refPoint.Rho();
    
//   std::cerr << "ratio:" << ratio << std::endl;
    
    XYZVector Delta = refPoint*(ratio-1);
    
//   std::cerr << "Delta:"
// 	    <<  Delta.X() << " "
// 	    <<  Delta.Y() << " "
// 	    <<  Delta.Z()
// 	    << std::endl;
    
    for (int i=0; i<4; i++) {
        corner_[i]+=Delta;
    }
    
    if (displacement!=NULL) {
        for (int i=0; i<4; i++) {
            corner_[i]+=(*displacement);
        }
        delete displacement;
    }
    
    return 0;
}


// Distance between origin and module if hits
// Otherwise -1
double Module::trackCross(const XYZVector& PL, // Base line point
        const XYZVector& PU) // Line direction
{
    double distance;
    
    distance = triangleCross(corner_[0],
            corner_[1],
            corner_[2],
            PL, PU);
    
    if (distance<0) {
        distance = triangleCross(corner_[0],
                corner_[2],
                corner_[3],
                PL, PU);
    }
    
    if (distance>=0) {
        nHits_++;
        distance /= PU.r();
    } else {
        return -1;
    }
    
    return distance;
    
}



double Module::triangleCross(const XYZVector& P1, // Triangle points
        const XYZVector& P2,
        const XYZVector& P3,
        const XYZVector& PL, // Base line point
        const XYZVector& PU) // Line direction
{
    bool moduleHit = false;
    
    // Triangle coordinates
    // t - P1 = alpha * (P2-P1) + beta * (P3-P1)
    
    // Line coordinates:
    // r = PL + gamma * PU
    
    
    // How to solve the generic problem:
    
    // d = PL - P1
    // A = (P2-P1, P3-P1, -PU)
    // v = intersection point
    TVectorD d = TVectorD(3);
    TMatrixD A = TMatrixD(3, 3);
    TVectorD v = TVectorD(3);
    
    d(0)=PL.x()-P1.x();
    d(1)=PL.y()-P1.y();
    d(2)=PL.z()-P1.z();
    
    A(0, 0)=P2.x()-P1.x();
    A(0, 1)=P3.x()-P1.x();
    A(0, 2)=-1*PU.x();
    A(1, 0)=P2.y()-P1.y();
    A(1, 1)=P3.y()-P1.y();
    A(1, 2)=-1*PU.y();
    A(2, 0)=P2.z()-P1.z();
    A(2, 1)=P3.z()-P1.z();
    A(2, 2)=-1*PU.z();
    
    Double_t determ;
    A.InvertFast(&determ);
    
    // The matrix is invertible
    if (determ!=0) {
        // v = A^{-1} * d
        v = A * d;
        // v(0) = alpha : triangle local coordinate
        // v(1) = beta  : triangle local coordinate
        // v(2) = gamma : line coordinate
        
//     std::cout << "Alpha: " << v(0) << std::endl;
//     std::cout << "Beta:  " << v(1) << std::endl;
//     std::cout << "Gamma: " << v(2) << std::endl;
        
        if (
                (v(0)>=0)&&
                (v(1)>=0)&&
                ((v(0)+v(1))<=1)
                ) {
            moduleHit = true;
        } else {
            moduleHit = false;
        }
        
    } else {
        // It does not cross the triangle
        moduleHit=false;
        std::cout << "Matrix is not invertible" << std::endl; // debug
    }
    
    
    if (moduleHit) {
        return v(2);
    } else {
        return -1.;
    }
    
}

// Creates a new object in the volume container
// of geometry geom
void Module::shapeVolume(TGeoVolume* container,
        TGeoMedium* medium,
        TGeoManager* geom) {
    
    
    // First of all: find the local coordinates of the module
    XYZVector ex, ey, ez;
    XYZVector b, c, d, p;
    
    b = corner_[1]-corner_[0];
    c = corner_[2]-corner_[0];
    d = corner_[3]-corner_[0];
    
    ex = b/b.R();
    p  = (d.Dot(ex) * ex);
    ey = d - p;
    ey /= ey.R();
    ez = ex.Cross(ey);
    
    // Local coordinates of module's corners
    // x'           y'
    // 0            0
    // b.R()        0
    // c.Dot(ex)    c.Dot(ey)
    // d.Dot(ex)    d.Dot(ey)
    
    // TODO: we should check that {b, c, d}.Dot(ez) < 10E-5 or so
//   std::cerr << b.Dot(ez) << "==0 ?" << std::endl;
//   std::cerr << c.Dot(ez) << "==0 ?" << std::endl;
//   std::cerr << d.Dot(ez) << "==0 ?" << std::endl;
    
    // This is the rotation (columns of matrix myRot are
    // the module axes expressed in the global frame)
    Double_t matrixElements[9];
    matrixElements[0]=ex.X();
    matrixElements[1]=ey.X();
    matrixElements[2]=ez.X();
    matrixElements[3]=ex.Y();
    matrixElements[4]=ey.Y();
    matrixElements[5]=ez.Y();
    matrixElements[6]=ex.Z();
    matrixElements[7]=ey.Z();
    matrixElements[8]=ez.Z();
    
    
    TGeoRotation *myRot    = new TGeoRotation();
    myRot->SetMatrix(matrixElements); // Ecco la matrice
    
    // We add the translation (point 0 of module)
    TGeoCombiTrans *myCombi = new TGeoCombiTrans(corner_[0].X(),
            corner_[0].Y(),
            corner_[0].Z(),
            myRot);
    
    // Delete the volume if already existing
    if (thisVolume_) delete thisVolume_;
    
    // Create and get a grip on th object
    // TODO: place a global counter here...
    TGeoVolume *thisVolume_ = geom->MakeArb8(id_.c_str(), medium, moduleThickness_.value); // CUIDADO THIS IGNORES DS DISTANCE AND SENSOR THICKNESS FROM MODULE TYPE!!!!!!
    thisVolume_->SetLineColor(color_);
    TGeoArb8 *arb = (TGeoArb8*)thisVolume_->GetShape();
    
    // Add the points of the module in its local coords
    // with respect to point 0
    for (int i=0; i<5; i+=4) {
        arb->SetVertex(0+i, 0        , 0);
        arb->SetVertex(1+i, b.R()    , 0);
        arb->SetVertex(2+i, c.Dot(ex), c.Dot(ey));
        arb->SetVertex(3+i, d.Dot(ex), d.Dot(ey));
    }
    
    // And finally place it into the empty space!
    container->AddNode(thisVolume_, 1, myCombi);
    
}


TPolyLine3D* Module::getContour() {
    TPolyLine3D *contour = new TPolyLine3D(5);
    for (Int_t i=0;i<4;i++) {
        contour->SetPoint(i,
                corner_[i].X(),
                corner_[i].Y(),
                corner_[i].Z());
    }
    contour->SetPoint(4,
            corner_[0].X(),
            corner_[0].Y(),
            corner_[0].Z());
    contour->SetLineColor(color_);
    return contour;
}





edge Module::getEdgeRhoSide(int direction) {
    Module fakeModule;
    XYZVector* marginBorder;
    
    for (int i=0; i<4; i++) {
      marginBorder = marginBorderSide(0, i);
      fakeModule.corner_[i]=(*marginBorder);
      delete marginBorder;
    }
    
    return fakeModule.getEdgeRho(direction);
}


edge Module::getEdgeRho(int direction) {
    edge result;
    
    if (direction==0) {
        std::cerr << "Module::getEdgeRho direction was chosen as zero." << std::endl;
        result.first=-1;
        result.second=-1;
        return result;
    }
    
    XYZVector* referencePoint[4];
    
    for (int i=0; i<4; i++) {
        referencePoint[i]=&corner_[i];
    }
    
    
    if (direction<0) {
        // Get the point with lowest Rho
        result.first=referencePoint[0]->Rho();
        result.second=0;
        for (int i=1; i<4; i++) {
            if (referencePoint[i]->Rho()<result.first) {
                result.first=referencePoint[i]->Rho();
                result.second=i;
            }
        }
    } else {
        // Get the point with highest Rho
        result.first=referencePoint[0]->Rho();
        result.second=0;
        for (int i=1; i<4; i++) {
            if (referencePoint[i]->Rho()>result.first) {
                result.first=referencePoint[i]->Rho();
                result.second=i;
            }
        }
    }
    
    
    return result;
    
}

int Module::setEdgeRho(double newRho, int direction) {
    if (direction==0) {
        std::cerr << "Module::setEdgeRho direction was chosen as zero." << std::endl;
        return -1;
    }
    
    edge edgePoint = getEdgeRho((-1)*direction);
    
    XYZVector start = corner_[edgePoint.second];
    XYZVector dest  = start/start.Rho()*newRho;
    XYZVector delta = dest-start;
    
    for (int i=0; i<4; i++) {
        corner_[i]+=delta;
    }
    
    return 1;
}

int Module::setEdgeRhoSide(double newRho, int direction) {
    if (direction==0) {
        std::cerr << "Module::setEdgeRho direction was chosen as zero." << std::endl;
        return -1;
    }
    
    edge edgePoint = getEdgeRhoSide((-1)*direction);
    XYZVector* anEdge = marginBorderSide(0, edgePoint.second);
    
    XYZVector start = *(anEdge);
    XYZVector dest  = start/start.Rho()*newRho;
    XYZVector delta = dest-start;
    
    for (int i=0; i<4; i++) {
        corner_[i]+=delta;
    }
    
    delete anEdge;
    
    return 1;
}

double Module::getMinTheta() const {
  if (!minTheta_.inited) {
    double aTheta;
    minTheta_.value = corner_[0].Theta();
    for (int i=1; i<4; i++) {
      aTheta = corner_[i].Theta();
      if (aTheta < minTheta_.value) minTheta_.value = aTheta;
    }
    minTheta_.inited = geometryLocked();
  }
  return minTheta_.value;
}

double Module::getMaxTheta() const {
  if (!maxTheta_.inited) {
    double aTheta;
    maxTheta_.value = corner_[0].Theta();
    for (int i=1; i<4; i++) {
      aTheta = corner_[i].Theta();
      if (aTheta > maxTheta_.value) maxTheta_.value = aTheta;
    }
    maxTheta_.inited = geometryLocked();
  }
  return maxTheta_.value;
}

double Module::getMeanTheta() const {
  if (!meanTheta_.inited) {
    XYZVector meanPoint(0, 0, 0);
    for (int i=0; i<4; i++) {
      meanPoint += corner_[i];
    }
    meanPoint /= 4;
    meanTheta_.value = meanPoint.Theta();
    meanTheta_.inited = geometryLocked();
  }  
  return meanTheta_.value;
}

double Module::getMaxRho() const {
  if (!maxRho_.inited) {
    maxRho_.value=-1;
    for (int i = 0; i < 4 ; i++) {
      if (corner_[i].Rho()>maxRho_.value) maxRho_.value=corner_[i].Rho();
    }
    maxRho_.inited = geometryLocked();
  }
  return maxRho_.value;
}

double Module::getMinRho() const {
  if (!minRho_.inited) {
    minRho_.value = corner_[0].Rho();
    unsigned int i1 = 0, i2 = 0;
    for (int i = 1; i < 4 ; i++) {
      //if (corner_[i].Rho()<minRho) minRho=corner_[i].Rho();
      if (corner_[i].Rho() < corner_[i1].Rho()) i1 = i;
    }
    if (i1 == 0) i2 = 1;
    for (unsigned int i = 1; i < 4; i++) {
      if ((corner_[i].Rho() < corner_[i2].Rho()) && (i != i1)) i2 = i;
    }
    if (((corner_[i1].Rho() + corner_[i2].Rho()) / 2.0) < minRho_.value) minRho_.value = (corner_[i1].Rho() + corner_[i2].Rho()) / 2.0;
    if (getMeanPoint().Rho() < minRho_.value) minRho_.value = getMeanPoint().Rho();
    minRho_.inited = geometryLocked();
  }
  return minRho_.value;
}

double Module::pointSegmentDistance2d(const XYVector& p, const std::pair<XYVector, XYVector>& s) const {
  double x1 = s.first.X(), y1 = s.first.Y();
  double x2 = s.second.X(), y2 = s.second.Y();
  double x3 = p.X(), y3 = p.Y();
  double u = ((x3-x1)*(x2-x1) + (y3-y1)*(y2-y1))/((s.second - s.first).Mag2());
  u = MAX(MIN(u, 1), 0.); // constraining the point in the segment
  XYVector dp(x1 + u*(x2-x1), y1 + u*(y2-y1));
  return sqrt((dp - p).Mag2());
}



double Module::getMaxZ() const {
  if (!maxZ_.inited) {
    maxZ_.value = corner_[0].Z();
    for (int i = 1; i < 4 ; i++) {
        if (corner_[i].Z()>maxZ_.value) maxZ_.value=corner_[i].Z();
    }
    maxZ_.inited = geometryLocked();
  }
  return maxZ_.value;
}

double Module::getMinZ() const {
  if (!minZ_.inited) {
    minZ_.value=corner_[0].Z();
    for (int i = 1; i < 4 ; i++) {
      if (corner_[i].Z()<minZ_.value) minZ_.value = corner_[i].Z();
    }
    minZ_.inited = geometryLocked();
  }
  return minZ_.value;
}


const XYZVector& Module::getMeanPoint() const {
  if (!meanPoint_.inited) {
    meanPoint_.value = XYZVector(0, 0, 0);

    for (int i=0; i<4; i++) {
      meanPoint_.value += corner_[i];
    }
    meanPoint_.value /= 4;
    meanPoint_.inited = geometryLocked();
  } 
  return meanPoint_.value;
}

const XYZVector& Module::getNormal() const {
  if (!normal_.inited) {
    normal_ = XYZVector(getCorner(1) - getCorner(0)).Cross(getCorner(3) - getCorner(0)).Unit(); 
    normal_.inited = geometryLocked();
  }
  return normal_.value;
}

/*
void Module::computeStripArea(int nFace) {
    XYZVector meanPoint = getMeanPoint();
    // double meanEta = meanPoint.eta(); // TODO: remove this line
    double meanPhi = meanPoint.phi();
    
    double maxTheta, minTheta, maxPhi, minPhi, aPhi;
    maxTheta = getMaxTheta();
    minTheta = getMinTheta();
    //  std::cerr << "theta:\t" << minTheta << "\t" << maxTheta << std::endl;
    
    Module* anotherModule = new Module(*this);
    anotherModule->rotatePhi(-1*meanPhi);
    maxPhi=0;
    minPhi=0;
    for (int i=0; i<4 ; i++) {
        aPhi=anotherModule->corner_[i].phi();
        if (aPhi>maxPhi) maxPhi=aPhi;
        if (aPhi<minPhi) minPhi=aPhi;
    }
    
    // std::cerr << "phi:\t" << minPhi << "\t" << maxPhi << std::endl;
    
    double phiWidth, etaWidth;
    phiWidth = maxPhi-minPhi;
    etaWidth = log(tan(maxTheta/2.))-1*log(tan(minTheta/2.));
    //  std::cerr << "widths: theta:\t" << etaWidth << " phi:\t" << phiWidth << std::endl;
    
    stripArea_ = phiWidth * etaWidth / double(getNChannelsFace(nFace));
    //  std::cerr << "area:\t" << stripArea_ << std::endl;
    
    delete anotherModule;
}
*/

/*
 * Computes the dPhi dTheta of the sensor with the lowest number of
 * segments
 */
void Module::computeMaxDphiDeta() {
    XYZVector meanPoint = getMeanPoint();
    double meanPhi = meanPoint.phi();
    
    double maxTheta, minTheta, maxPhi, minPhi, aPhi;
    maxTheta = getMaxTheta();
    minTheta = getMinTheta();
    
    Module* anotherModule = new Module(*this);
    anotherModule->rotatePhi(-1*meanPhi);
    maxPhi=0;
    minPhi=0;
    for (int i=0; i<4 ; i++) {
        aPhi=anotherModule->corner_[i].phi();
        if (aPhi>maxPhi) maxPhi=aPhi;
        if (aPhi<minPhi) minPhi=aPhi;
    }
    
    phiWidth_ = maxPhi-minPhi;
    etaWidth_ = log(tan(maxTheta/2.))-1*log(tan(minTheta/2.));
    
    double minNSegments = nSegmentsFace_[findMinSegmentsFace_()];
    dphideta_ = phiWidth_ * etaWidth_ / minNSegments;
    
    delete anotherModule;
}


double Module::getStripOccupancyPerEvent() {
    std::cout << "ERROR: you are somehow accessing the deprecated generic getOccupancyPerEvent" << std::endl;
    return 0;
}
double Module::getHitOccupancyPerEvent() {
    std::cout << "ERROR: you are somehow accessing the deprecated generic getOccupancyPerEvent" << std::endl;
    return 0;
}

// The occupancy ε estimation is obtained with the following formula:
// ε=μ*Δφ*Δη/f
// where:
//  μ comes from a separate barrel/endcap fit on current tracker
//  Δφ refers to the module
//  Δη refers to the strip
//  f is a compensation factor:
//    sin(theta) for barrel
//    cos(theta) for endcap

double BarrelModule::getStripOccupancyPerEvent() {
    XYZVector meanPoint = getMeanPoint();
    double rho = meanPoint.Rho()/10.;
    double theta = meanPoint.Theta();
    
    double myOccupancyBarrel=(1.63e-4)+(2.56e-4)*rho-(1.92e-6)*rho*rho;
    //std::cerr << "Rho: " << rho << ", "
    //	    << "myOcc: " << myOccupancyBarrel << std::endl;
    double factor=fabs(sin(theta))*2; // 2 is a magic adjustment factor
    computeMaxDphiDeta();

    return myOccupancyBarrel*dphideta_/factor / (90/1e3) * (getWidth() / nStripAcross_ );
}


double BarrelModule::getHitOccupancyPerEvent() {
    return getStripOccupancyPerEvent()/2; // CUIDADO: placeholder formula
}


double EndcapModule::getStripOccupancyPerEvent() {
    XYZVector meanPoint = getMeanPoint();
    double z = fabs(meanPoint.Z())/10.;
    double rho = meanPoint.Rho()/10.;
    double theta = meanPoint.Theta();
    
    double myOccupancyEndcap=(-6.20e-5)+(1.75e-4)*rho-(1.08e-6)*rho*rho+(1.50e-5)*(z);
    //std::cerr << "Rho: " << rho << ", z: " << z << ", "
    //	    << "myOcc: " << myOccupancyEndcap << std::endl;
    double factor=fabs(cos(theta))*2; // 2 is a magic adjustment factor
    computeMaxDphiDeta();

    return myOccupancyEndcap*dphideta_/factor / (90/1e3) * ((getWidthLo() + getWidthHi())/2. / nStripAcross_ );
}


double EndcapModule::getMinRho() const {
  if (!minRho_.inited) {
    std::vector<double> rhos(4);
    for (int i = 0; i < 4; i++) { 
      rhos[i] = pointSegmentDistance2d(XYVector(0.,0.), std::make_pair(XYVector( corner_[i].X(),       corner_[i].Y() ), 
                                                                       XYVector( corner_[(i+1)%4].X(), corner_[(i+1)%4].Y() )));
    }
    std::sort(rhos.begin(), rhos.end());
    minRho_.value = rhos.front();
    minRho_.inited = geometryLocked();
  }
  return minRho_.value;
}


double EndcapModule::getHitOccupancyPerEvent() {
    return getStripOccupancyPerEvent()/2; // CUIDADO: placeholder formula
}

double Module::getLowPitch() {
    //XYZVector acrossV = corner_[0] - corner_[3];
    //return (acrossV.R()/nStripAcross_);
    return 0;
}

double Module::getHighPitch() {
    //XYZVector acrossV = corner_[2] - corner_[1];
    //return (acrossV.R()/nStripAcross_);
    return 0;
}

double BarrelModule::getLowPitch() {
    return (width_/double(nStripAcross_));
}

double BarrelModule::getHighPitch() {
    return (width_/double(nStripAcross_));
}

double EndcapModule::getLowPitch() {
    return (widthLo_/double(nStripAcross_));
}

double EndcapModule::getHighPitch() {
    return (widthHi_/double(nStripAcross_));
}

void Module::computeBoundaries(double zError) {
    double minEta, maxEta;
    double minPhi, maxPhi;
    double averagePhi, artificialRotation;
    Module* fakeModule;
    double thisEta, thisPhi;
    int i, j;
    double z;
    
    // Compute eta boundaries:
    minEta=corner_[0].Eta();
    maxEta=corner_[0].Eta();
    for (z=-zError*BoundaryEtaSafetyMargin; z<=zError*BoundaryEtaSafetyMargin; z+=zError*BoundaryEtaSafetyMargin) {
        for (i=0; i<4; i++) {
            thisEta = (corner_[i]+XYZVector(0, 0, z)).Eta();
            if (thisEta>maxEta) maxEta=thisEta;
            if (thisEta<minEta) minEta=thisEta;
        }
        for (i=0; i<4; i++) {
            j=i+1; if (j==4) j=0;
            XYZVector sideMiddle((corner_[i]+corner_[j])/2.+XYZVector(0, 0, z));
            thisEta = sideMiddle.Eta();
            if (thisEta>maxEta) maxEta=thisEta;
            if (thisEta<minEta) minEta=thisEta;
        }
        if (zError*BoundaryEtaSafetyMargin==0) break;
    }
    
    // Compute Phi boundaries
    artificialRotation=0;
    averagePhi=this->getMeanPoint().Phi();
    if (averagePhi>M_PI/2.) artificialRotation=-1*M_PI/2.;
    if (averagePhi<-1*M_PI/2.) artificialRotation=M_PI/2.;
    fakeModule = new Module(*this);
    fakeModule->rotatePhi(artificialRotation);
    
    minPhi=fakeModule->getCorner(0).Phi();
    maxPhi=fakeModule->getCorner(0).Phi();
    for (i=1; i<4; i++) {
        thisPhi = fakeModule->getCorner(i).Phi();
        if (thisPhi>maxPhi) maxPhi=thisPhi;
        if (thisPhi<minPhi) minPhi=thisPhi;
    }
    for (i=0; i<4; i++) {
        j=i+1; if (j==4) j=0;
        XYZVector sideMiddle((fakeModule->getCorner(i)+fakeModule->getCorner(j))/2.);
        thisPhi = sideMiddle.Phi();
        if (thisPhi>maxPhi) maxPhi=thisPhi;
        if (thisPhi<minPhi) minPhi=thisPhi;
    }
    delete fakeModule;
    //fprintf(stderr, "AveragePhi: %.2f  artRot: %.2f  min/max: %.2f/%.2f", averagePhi, artificialRotation, minPhi, maxPhi); //debug
    minPhi-=artificialRotation;
    maxPhi-=artificialRotation;
    //fprintf(stderr, "->  min/max: %.2f/%.2f\n", minPhi, maxPhi); //debug
    
    
    // Assign boundaries to the module's
    // member variables
    boundaryMinEta_=minEta;
    boundaryMaxEta_=maxEta;
    boundaryMinPhi_=minPhi;
    boundaryMaxPhi_=maxPhi;
    
    // Assign logical member variables
    if (artificialRotation==0) {
        isAcrossPi_=false;
    } else {
        isAcrossPi_=true;
    }
    computedBoundaries_ = true;
    
}


bool Module::couldHit(double eta, double phi) {
    bool withinPhi, withinPhiSub, withinPhiAdd;
    
    if (!computedBoundaries_) {
        std::cerr << "ERROR: missing boundaries after computeBoundaries()" << std::endl;
        return true;
    }
    
    // If track's eta is outside the boundaries, then the result is certainly false
    if ((eta<boundaryMinEta_)||(eta>boundaryMaxEta_)) return false;
    
    // withinPhi represents whether the track's phi
    // is comprised within minimum and maximum phi of the module
    // Phi +- 2Pi is also to be checked
    withinPhi = ((phi>boundaryMinPhi_)&&(phi<boundaryMaxPhi_));
    withinPhiSub = ((phi-2.*M_PI>boundaryMinPhi_)&&(phi-2.*M_PI<boundaryMaxPhi_));
    withinPhiAdd = ((phi+2.*M_PI>boundaryMinPhi_)&&(phi+2.*M_PI<boundaryMaxPhi_));
    
    return (withinPhi||withinPhiSub||withinPhiAdd);
    
}

// TODO: this is VERY ugly!!! :( sorry: hurry !
const double ptMinFit = 0.22;
const double ptMaxFit = 10.;
// log(pt/z) distribution parameters for 12000 events at 14 TeV
const double ptFitParamsLow[]  = { 2.52523e+01, -6.84183e+00, -1.20149e+01,  1.89314e+00}; // 0.22 GeV to 1 GeV  Chi^2 / dof = 22.0408/16 = 1.37755
const double ptFitParamsMid[]  = { 7.27638e-01, -1.04041e+00,  8.56495e+00,  6.52714e-03}; // 1 GeV to 4 GeV     Chi^2 / dof = 84.2299/71 = 1.18634
const double ptFitParamsHigh[] = { 4.66514e+01, -2.88910e+00, -3.78716e+01,  1.26635e-01}; // 4 GeV to 10 GeV    Chi^2 / dof = 102.736/135 = 0.76101

double Module::getTriggerFrequencyTruePerEventAbove(const double& myCut) {
  return getTriggerFrequencyTruePerEventBetween(myCut, ptMaxFit);
}

double Module::getParticleFrequencyPerEventAbove(const double& myCut) {
  return getParticleFrequencyPerEventBetween(myCut, ptMaxFit);
}

double Module::getTriggerFrequencyTruePerEventBelow(const double& myCut) {
  return getTriggerFrequencyTruePerEventBetween(ptMinFit, myCut);
}

// TODO: this and the next one should be merged in the same code...
double Module::getTriggerFrequencyTruePerEventBetween(double myLowCut, double myHighCut) {
  if (myLowCut<ptMinFit) myLowCut=ptMinFit;
  if (myHighCut>ptMaxFit) myHighCut=ptMaxFit;
  computeMaxDphiDeta();
  double r = getMeanPoint().Rho()/1000;
  double ptMin = max(0.3 * insur::magnetic_field * r, myLowCut);
  double integral = 0.0;
  double dPt = 0.05;
  double etaphi = phiWidth_/(2.0*3.141592)*fabs(etaWidth_)/6.0;	
  double nPt;
  const double* ptFitParams;
  for (double pt = ptMin; pt < myHighCut; pt += dPt) {
    if (pt < 1) ptFitParams = ptFitParamsLow;
    else if (pt < 6) ptFitParams = ptFitParamsMid;
    else ptFitParams = ptFitParamsHigh;
    
    nPt = exp(ptFitParams[0]
              + ptFitParams[1] * pt
              + ptFitParams[2] * pow(pt,-0.1)
              + ptFitParams[3] * pow(pt,2))/12000;

    integral += getTriggerProbability(pt) * (nPt/4e-2) * etaphi * dPt;
  }
  
  return integral;
}

// TODO: this and the previous one should be merged in the same code...
double Module::getParticleFrequencyPerEventBetween(double myLowCut, double myHighCut) {
  if (myLowCut<ptMinFit) myLowCut=ptMinFit;
  if (myHighCut>ptMaxFit) myHighCut=ptMaxFit;
  computeMaxDphiDeta();
  double r = getMeanPoint().Rho()/1000;
  double ptMin = max(0.3 * insur::magnetic_field * r, myLowCut);
  double integral = 0.0;
  double dPt = 0.05;
  double etaphi = phiWidth_/(2.0*3.141592)*fabs(etaWidth_)/6.0;	
  double nPt;
  const double* ptFitParams;
  for (double pt = ptMin; pt < myHighCut; pt += dPt) {
    if (pt < 1) ptFitParams = ptFitParamsLow;
    else if (pt < 6) ptFitParams = ptFitParamsMid;
    else ptFitParams = ptFitParamsHigh;
    
    nPt = exp(ptFitParams[0]
              + ptFitParams[1] * pt
              + ptFitParams[2] * pow(pt,-0.1)
              + ptFitParams[3] * pow(pt,2))/12000;

    integral += (nPt/4e-2) * etaphi * dPt;
  }
  
  return integral;
}

double Module::getTriggerFrequencyFakePerEvent() {
	return pow(getHitOccupancyPerEvent(),2)*triggerWindow_*getNChannelsFace(1);
}


/******************/
/*                */
/* Barrel module  */
/*                */
/******************/

BarrelModule::~BarrelModule() {
}

BarrelModule::BarrelModule(double waferDiameter, double heightOverWidth) : Module(waferDiameter) {
    setSensorRectGeometry(heightOverWidth);
}

BarrelModule::BarrelModule(double heightOverWidth /*=1*/ ) : Module() {
    setSensorRectGeometry(heightOverWidth);
}

void BarrelModule::setSensorRectGeometry(double heightOverWidth) {
    double cornerAngle = atan(heightOverWidth);
    shape_ = Rectangular;
    aspectRatio_=heightOverWidth;
    height_ = waferDiameter_ * sin(cornerAngle);
    width_  = waferDiameter_ * cos(cornerAngle);
    area_ = height_*width_;
    
    // Upon creation the module is placed horizontally
    // with its center laying on 0,0,0
    for (int i=0; i<4; i++) corner_[i].SetY(0.);
    corner_[0].SetX((-1)*width_/2);
    corner_[1].SetX(width_/2);
    corner_[2].SetX(width_/2);
    corner_[3].SetX((-1)*width_/2);
    
    corner_[0].SetZ(height_/2);
    corner_[1].SetZ(height_/2);
    corner_[2].SetZ((-1)*height_/2);
    corner_[3].SetZ((-1)*height_/2);
}

void EndcapModule::setSensorRectGeometry(double heightOverWidth, double d) {
    double cornerAngle = atan(heightOverWidth);
    double width;
    height_ = waferDiameter_ * sin(cornerAngle);
    width  = waferDiameter_ * cos(cornerAngle);
    area_ = height_*width;
    widthLo_=width;
    widthHi_=width;
    dist_=d;
    cut_=false;
    lost_=0;
    shape_=Rectangular;
    aspectRatio_=heightOverWidth;
    
    
    // Upon creation the module is placed vertically
    // with its center laying on 0,d+height_/2.,0
    // that is with its base point in 0,d,0
    for (int i=0; i<4; i++) corner_[i].SetY(0.);
    corner_[0].SetX((-1)*width/2);
    corner_[1].SetX(width/2);
    corner_[2].SetX(width/2);
    corner_[3].SetX((-1)*width/2);
    
    corner_[0].SetY(d+height_);
    corner_[1].SetY(d+height_);
    corner_[2].SetY(d);
    corner_[3].SetY(d);
}


// This would not measure correctly the margin! TODO
edge BarrelModule::getEdgeZSide(int direction, double margin /*= 0*/) {
    BarrelModule fakeModule;
    
    
    
    XYZVector* marginBorder;
    
    for (int i=0; i<4; i++) {
        marginBorder = marginBorderSide(margin, i);
        fakeModule.corner_[i]=(*marginBorder);
        delete marginBorder;
    }
    
    return fakeModule.getEdgeZ(direction, 0);
}

edge BarrelModule::getEdgeZ(int direction, double margin /*= 0*/ ) {
    edge result;
    
    if (direction==0) {
        std::cerr << "BarrelModule::getEdgeZ direction was chosen as zero." << std::endl;
        result.first=-1;
        result.second=-1;
        return result;
    }
    
    XYZVector* referencePoint[4];
    
    if (margin==0) {
        for (int i=0; i<4; i++) {
            referencePoint[i]=&corner_[i];
        }
    } else {
        for (int i=0; i<4; i++) {
            referencePoint[i]= marginBorder(margin, margin, i);
        }
    }
    
    
    
    if (direction<0) {
        // Get the point with lowest Z
        result.first=referencePoint[0]->Z();
        result.second=0;
        for (int i=1; i<4; i++) {
            if (referencePoint[i]->Z()<result.first) {
                result.first=referencePoint[i]->Z();
                result.second=i;
            }
        }
    } else {
        // Get the point with highest Z
        result.first=referencePoint[0]->Z();
        result.second=0;
        for (int i=1; i<4; i++) {
            if (referencePoint[i]->Z()>result.first) {
                result.first=referencePoint[i]->Z();
                result.second=i;
            }
        }
    }
    
    
    if (margin!=0) {
        for (int i=0; i<4; i++)
            delete referencePoint[i];
    }
    
    
    return result;
    
}

int BarrelModule::setEdgeZ(double newZ, int direction) {
    if (direction==0) {
        std::cerr << "BarrelModule::setEdgeZ direction was chosen as zero." << std::endl;
        return -1;
    }
    
    edge edgePoint = getEdgeZ((-1)*direction);
    XYZVector delta(0,
            0,
            newZ-edgePoint.first);
    
    
    for (int i=0; i<4; i++) {
        corner_[i]+=delta;
    }
    
    return 1;
}

// This would not measure correctly the margin! TODO
edge BarrelModule::getEdgePhiSide(int direction, double margin /*= 0*/) {
    
    
    BarrelModule fakeModule;
    XYZVector* marginBorder;
    
    int nextIndex;
    for (int i=0; i<4; i++) {
        nextIndex=i+1;
        if (nextIndex==4) nextIndex=0;
        marginBorder = marginBorderSide(margin, i);
        fakeModule.corner_[i]=(*marginBorder);
        delete marginBorder;
    }
    
    return fakeModule.getEdgePhi(direction, 0);
}

edge BarrelModule::getEdgePhi(int direction, double margin /*= 0*/) {
    bool thereIsThirdQuad=false;
    bool thereIsFourthQuad=false;
    edge result;
    
    if (direction==0) {
        std::cerr << "In BarrelModule::getEdgePhi direction was chosen as zero." << std::endl;
        result.first=-1;
        result.second=-1;
        return result;
    }
    
    
    XYZVector* referencePoint[4];
    
    if (margin==0) {
        for (int i=0; i<4; i++) {
            referencePoint[i]=&corner_[i];
        }
    } else {
        for (int i=0; i<4; i++) {
            referencePoint[i]=marginBorder(margin, margin, i);
        }
    }
    
    double phi[4];
    
    
    for (int i=0; i<4; i++) {
        phi[i]=referencePoint[i]->Phi();
        if ((phi[i]>=M_PI/2.)&&(phi[i]<=M_PI)) thereIsThirdQuad=true;
        if ((phi[i]<=(-1)*M_PI/2.)&&(phi[i]>=(-1)*M_PI)) thereIsFourthQuad=true;
    }
    
    // Adjust negative values to be in the range (0, 2Pi)
    // in case we have phis in the third AND fourth quadrant
    if ((thereIsThirdQuad) && (thereIsFourthQuad)) {
        for (int i=0; i<4; i++) {
            if (phi[i]<0) phi[i]+=2*M_PI;
        }
    }
    
    // If direction is greater than zero we are looking for the highest phi
    if (direction>0) {
        result.first=phi[0];
        result.second=0;
        for (int i=1; i<4; i++) {
            if (phi[i]>result.first) {
                result.first=phi[i];
                result.second=i;
            }
        }
    }
    
    // If direction is lower than zero we are looking for the lowest phi
    if (direction<0) {
        result.first=phi[0];
        result.second=0;
        for (int i=1; i<4; i++) {
            if (phi[i]<result.first) {
                result.first=phi[i];
                result.second=i;
            }
        }
    }
    
    
    if (margin!=0) {
        for (int i=0; i<4; i++)
            delete referencePoint[i];
    }
    
    
    return result;
}

int BarrelModule::setEdgePhi(double newPhi, int direction) {
    
    if (direction==0) {
        std::cerr << "In BarrelModule::getEdgePhi direction was chosen as zero." << std::endl;
        return -1;
    }
    
    double startEdgePhi = getEdgePhi(-1*direction).first;
    rotatePhi(newPhi-startEdgePhi);
    
    return 1;
}

double BarrelModule::getMaxPhi() const {
  if (!maxPhi_.inited) {
    const XYZVector& center = getMeanPoint();
    maxPhi_.value = fmod(atan(getWidth()/(2*center.Rho()))+center.Phi(), 2*M_PI);
    maxPhi_.inited = geometryLocked();
  }
  return maxPhi_.value;
}


double BarrelModule::getMinPhi() const {
  if (!minPhi_.inited) {
    const XYZVector& center = getMeanPoint();
    minPhi_.value = -atan(getWidth()/(2*center.Rho()))+center.Phi();
    minPhi_.inited = geometryLocked();
  }
  return minPhi_.value;
}

/******************/
/*                */
/* Endcap module  */
/*                */
/******************/

EndcapModule::~EndcapModule() {
}

// Just to make an endcapmodule out of a module, without touching the geometry
/*EndcapModule::EndcapModule(const Module& aModule) : Module(aModule) {
 * cut_ = false;
 *
 * //phiWidth_ = 0;
 * widthLo_  = 0;
 * widthHi_  = 0;
 * dist_     = 0;
 *
 * }*/

EndcapModule::EndcapModule(int shape /*=wedge*/) : Module() {
    shape_=shape;
    if (shape==Rectangular) {
        setSensorRectGeometry(defaultAspectRatio_, 0);
    }
}


EndcapModule::EndcapModule(double waferDiameter, double heightOverWidth) : Module(waferDiameter) {
    setSensorRectGeometry(heightOverWidth, 0);
}

EndcapModule::EndcapModule(double heightOverWidth) : Module() {
    setSensorRectGeometry(heightOverWidth, 0);
}

// Modifying the geometry

// Wedge-shaped:
EndcapModule::EndcapModule(const Module& sampleModule, double alpha, double d, double maxRho /* = -1 */) : Module(sampleModule) {
    setSensorWedgeGeometry(alpha, d, maxRho);
}

EndcapModule::EndcapModule(const EndcapModule& sampleModule, double alpha, double d, double maxRho /* = -1 */) : Module(sampleModule) {
    //phiWidth_ = sampleModule.phiWidth_;
    widthLo_ = sampleModule.widthLo_;
    widthHi_ = sampleModule.widthHi_;
    dist_ = sampleModule.dist_;
    cut_ = sampleModule.cut_;
    lost_ = sampleModule.lost_;
    ring_ = sampleModule.ring_;
    disk_ = sampleModule.disk_;
    
    if (sampleModule.shape_==Wedge) 
      setSensorWedgeGeometry(alpha, d, maxRho);
    else if (sampleModule.shape_==Rectangular)
      setSensorRectGeometry(height_/widthLo_, d);
    else {
      std::cerr << "ERROR: EndcapModule::EndcapModule() in the sample module has an undefined geometry" << std::endl;
    }
}

EndcapModule::EndcapModule(double alpha, double d, double maxRho /* = -1 */) : Module() {
    setSensorWedgeGeometry(alpha, d, maxRho);
}

EndcapModule::EndcapModule(const Module& aModule, double d) : Module(aModule) {
    if (aModule.getShape()==Wedge) {
        std::cout << "WARNING: creating a rectangular endcap module out of a wedge-shaped module"
        << "I'm going to guess the aspect ratio from the height/averageWidth!" << std::endl;
    } else if (shape_==Undefined) {
        std::cout << "ERROR: creating a rectangular endcap module out of an unknown-shaped module" << std::endl;
    }
    setSensorRectGeometry(aspectRatio_, d);
}


double EndcapModule::getMaxPhi() const {  // for Z+ disks phi0 is always MAX, for Z- disks phi1 is always MAX
  if (!maxPhi_.inited) {
    maxPhi_.value = getCorner(0).Z() < 0 ? getCorner(1).Phi() : getCorner(0).Phi();
    maxPhi_.inited = geometryLocked();
  }
  return maxPhi_.value;
}


double EndcapModule::getMinPhi() const {
  if (!minPhi_.inited) {
    minPhi_.value = getCorner(0).Z() < 0 ? getCorner(0).Phi() : getCorner(1).Phi();
    minPhi_.inited = geometryLocked();
  }
  return minPhi_.value;
}


void EndcapModule::setSensorWedgeGeometry(double alpha, double d, double maxRho /* = -1 */) {
    
    shape_=Wedge;
    cut_=false;
    
    double r = waferDiameter_/2.;
    
    // We need the half angle covered by the module
    double phi;
    //double delta,epsilon; // alternate
    phi = alpha/2.;
    
    //double gamma1; // alternate
    double gamma2;
    double h1, h2, b1, b2, dfar, l;
    
    //std::cout << "d=" << d << ", ";
    //std::cout << "phi=" << phi << ", ";
    // The short (half)base
    h1 = d * tan(phi);
    //std::cout << "h1=" << h1 << ", ";
    // Distance of short base from the wafer center
    b1 = sqrt(pow(r, 2)-pow(h1, 2)); // main + alternate
    //std::cout << "b1=" << b1 << ", ";
    
    // y coordinate of the wafer center
    l = b1 + d; // main
    // gamma1 = d / cos(phi); // alternate
    //std::cout << "g1=" << gamma1 << ", ";
    // delta = atan(h1/b1); // alternate
    //std::cout << "del=" << delta << ", ";
    // epsilon = phi + delta; // alternate
    //std::cout << "eps=" << epsilon << ", ";
    
    
    // Distance of the far angle form the z axis
    gamma2 = l*cos(phi) + sqrt(pow(r, 2)-pow(l*sin(phi), 2)); // main
    // gamma2 = gamma1 + 2 * r * cos(epsilon); // alternate
    //std::cout << "g2=" << gamma2 << ", ";
    
    
    // The long (half)base
    h2 = gamma2 * sin(phi);
    //std::cout << "h2=" << h2 << ", ";
    // Distance of long base from the z axis
    dfar = gamma2 * cos(phi);
    //std::cout << "dfar=" << dfar << ", ";
    
    // The distance of the long base from the wafer center
    //b2 =  sqrt(pow(r,2)-pow(h2,2)); // old
    //std::cout << "b2 = " << b2;
    b2 = dfar - d - b1;
    //std::cout << "b2 = " << b2 << std::endl;
    
    // NOTE: in principle we don't need to compute b2 to get the
    // module's corner coordinates. Still we use this way of computing
    // b2 to ease the computation of the module's area
    
    // Add a check: if the module overcomes the max rho
    // it must be cut.
    if (maxRho>0) {
        if ((dfar)>maxRho) {
            //std::cout << "maxRho=" << maxRho << ", ";
            lost_=dfar-maxRho;
            //std::cout << "lost_=" << lost_ << ", ";
            b1=0;
            b2=maxRho-d;
            //std::cout << "b2=" << b2 << ", ";
            h2=h1/d*maxRho;
            //std::cout << "h2=" << h2 << std::endl;
            cut_=true;
        }
    }
    
    // Some member variable computing:
    area_     = fabs((b1+b2)*(h2+h1));
    height_   = b1+b2;
    //phiWidth_ = 2*phi;
    widthLo_  = 2*h1;
    widthHi_  = 2*h2;
    dist_     = d;
    aspectRatio_ = height_/(h1+h2);
    
    
    // Some checks
    // std::cerr << "r1-r: " << sqrt(pow(h1,2)+pow(b1,2)) - r << std::endl;
    // std::cerr << "r2-r: " << sqrt(pow(h2,2)+pow(b2,2)) - r << std::endl;
    // std::cerr << "Area: " << area_ << std::endl;
    
    // The four corners:
    // std::cerr << "h1: " << h1   << std::endl;
    // std::cerr << "d1: " << d    << std::endl;
    // std::cerr << "h2: " << h2   << std::endl;
    // std::cerr << "d2: " << d+b1+b2 << std::endl;
    
    corner_[0] = XYZVector(h1, d, 0);
    corner_[1] = XYZVector(h2, d+b1+b2, 0);
    corner_[2] = XYZVector(-1*h2, d+b1+b2, 0);
    corner_[3] = XYZVector(-1*h1, d, 0);
    
}



// ############################
// #                          #
// #  Sensor TAG should match #
// #                          #
// ############################

std::string BarrelModule::getSensorTag() {
    std::string result ;
    
    result = "Barrel";
    if (tag_!="") {
        result += "/Tag=" + tag_;
    }
    result += "/Width=" +
    boost::lexical_cast<std::string>(width_) +
    "/Height=" +
    boost::lexical_cast<std::string>(height_) +
    "/Thickness=" +
    boost::lexical_cast<std::string>(thickness_) +
    "/Type=" + type_;
    
    
    
    return result;
}


std::string EndcapModule::getSensorTag() {
    std::string result ;
    
    result = "Endcap";
    if (tag_!="") {
        result += "/Tag=" + tag_;
    }
    result += "/WidthLo=" +
    boost::lexical_cast<std::string>(widthLo_) +
    "/WidthHi=" +
    boost::lexical_cast<std::string>(widthHi_) +
    "/Height=" +
    boost::lexical_cast<std::string>(height_) +
    "/Thickness=" +
    boost::lexical_cast<std::string>(thickness_) +
    "/Type=" + type_;
    
    return result;
}

// Gives the geometric tag
std::string BarrelModule::getSensorGeoTag() {
    std::string result ;
    
    result = "Barrel";
    result += "/Width=" + boost::lexical_cast<std::string>(int(width_*1000)/1000.) +
      "/Height=" + boost::lexical_cast<std::string>(int(height_*1000)/1000.) +
      "/Thickness=" + boost::lexical_cast<std::string>(int(thickness_*1000)/1000.) +
      "/Type=" + type_ +
      "/StripsAcross=" +  boost::lexical_cast<std::string>(nStripAcross_);
    result += "/Faces=" +  boost::lexical_cast<std::string>(nFaces_);
    for (int i=0; i<nFaces_; ++i) result += "/Segments=" +  boost::lexical_cast<std::string>(nSegmentsFace_[i]);
    // "/Layer=" + boost::lexical_cast<std::string>(layer_);
    
    return result;
}

// Gives the geometric tag
std::string EndcapModule::getSensorGeoTag() {
    std::string result ;
    
    result = "Endcap";
    result += "/WidthLo=" + boost::lexical_cast<std::string>(int(widthLo_*1000)/1000.) +
      "/WidthHi=" + boost::lexical_cast<std::string>(int(widthHi_*1000)/1000.) +
      "/Height=" + boost::lexical_cast<std::string>(int(height_*1000)/1000.) +
      "/Thickness=" + boost::lexical_cast<std::string>(int(thickness_*1000)/1000.) +
      "/Type=" + type_+
      "/StripsAcross=" +  boost::lexical_cast<std::string>(nStripAcross_);
    result += "/Faces=" +  boost::lexical_cast<std::string>(nFaces_);
    for (int i=0; i<nFaces_; ++i) result += "/Segments=" +  boost::lexical_cast<std::string>(nSegmentsFace_[i]);
    // "/Disk=" + boost::lexical_cast<std::string>(disk_);
    
    return result;
}

std::string BarrelModule::getPositionTag() {
  std::ostringstream myTag;
  myTag.str("");
  myTag << containerName_ << std::dec ;
  myTag << "L" << setfill('0') << setw(2) << getLayer();
  //myTag << "R" << setfill('0') << setw(2) << getRing();
  
  return myTag.str();
}

std::string EndcapModule::getPositionTag() {
  std::ostringstream myTag;
  myTag.str("");
  myTag << containerName_ << std::dec ;
  myTag << "R" << setfill('0') << setw(2) << getRing();
  //myTag << "D" << setfill('0') << setw(2) << getDisk();

  return myTag.str();
}


