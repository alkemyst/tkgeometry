#ifndef ANALYZERVISITOR_H
#define ANALYZERVISITOR_H

#include "Visitor.h"


class AnalyzerVisitor : public GenericGeometryVisitor {
public:
  struct BarrelPath  { IdentifiableType tracker, barrel, layer, ring, rod;  };
  struct EndcapPath  { IdentifiableType tracker, endcap, disk, ring, blade; };
  struct SummaryPath { IdentifiableType tracker, table, row, column/*, phi*/;}; // provided for backwards compatibility for summary tables (the phi position is ignored, barrel and endcap have z and rho coords swapped)
private:
  union {
    BarrelPath barrelPath_;
    EndcapPath endcapPath_;
   // struct UniformPath { IdentifiableType tracker, cnt, z, rho, phi;          } uniformPath_;
    SummaryPath summaryPath_; // provided for backwards compatibility for summary tables (the phi position is ignored, barrel and endcap have z and rho coords swapped)
  } geomPath_;

protected:
  const BarrelPath& barrelPath() const { return geomPath_.barrelPath_; }
  const EndcapPath& endcapPath() const { return geomPath_.endcapPath_; }
  const SummaryPath& summaryPath() const { return geomPath_.summaryPath_; }

  bool isBarrelModule() const { return barrelModule_; }
  bool isEndcapModule() const { return !barrelModule_; } 

  virtual void doVisit(const Tracker&) {}
  virtual void doVisit(const Barrel&) {}
  virtual void doVisit(const Layer&) {}
  virtual void doVisit(const RodPair&) {}
  virtual void doVisit(const Endcap&) {}
  virtual void doVisit(const Disk&) {}
  virtual void doVisit(const Ring&) {}
  virtual void doVisit(const Module&) {}
  virtual void doVisit(const BarrelModule&) {}
  virtual void doVisit(const EndcapModule&) {}
  virtual void doVisit(const SingleSensorModule&) {}
  virtual void doVisit(const DualSensorModule&) {}
  virtual void doVisit(const PtModule&) {}
  virtual void doVisit(const StereoModule&) {}
  virtual void doVisit(const RectangularModule& rm) { doVisit((Module&)rm); }
  virtual void doVisit(const WedgeModule& wm) { doVisit((Module&)wm); }
public:
  void visit(const Tracker& t) { geomPath_.uniformPath_.tracker = t.myid(); doVisit(t); }
  void visit(const Barrel& b)  { geomPath_.barrelPath_.barrel = b.myid();   doVisit(b); }
  void visit(const Layer& l)   { geomPath_.barrelPath_.layer = l.myid();    doVisit(l); }
  void visit(const RodPair& r) { geomPath_.barrelPath_.rod = r.myid();      doVisit(r); } 
  void visit(const Endcap& e)  { geomPath_.endcapPath_.endcap = e.myid();   doVisit(e); }
  void visit(const Disk& d)    { geomPath_.endcapPath_.disk = d.myid();     doVisit(d); }
  void visit(const Ring& r)    { geomPath_.endcapPath_.ring = r.myid();     doVisit(r); }
  void visit(const BarrelModule& bm) { geomPath_.barrelPath_.ring_ = bm.myid(); barrelModule_ = true;  doVisit(bm); }
  void visit(const EndcapModule& em) { geomPath_.endcapPath_.blade_= em.myid(); barrelModule_ = false; doVisit(em); }
  void visit(const SingleSensorModule& ssm) { doVisit(ssm); }
  //void visit(const DualSensorModule& dsm)   { doVisit(dsm); }
  void visit(const PtModule& pm)            { doVisit((DualSensorModule&)pm); doVisit(pm); }
  void visit(const StereoModule& sm)        { doVisit((DualSensorModule&)sm); doVisit(sm); }
  void visit(const RectangularModule& rm)   { doVisit(rm);  }
  void visit(const WedgeModule& wm)         { doVisit(wm);  }
};



namespace VisitorHelpers {

  bool isModuleInEtaSector(const Tracker& tracker, const Module* module, int etaSector) const; 
  bool isModuleInPhiSector(const Tracker& tracker, const Module* module, int phiSector) const;

}


class TriggerProcessorBandwidthVisitor : public AnalyzerVisitor {
  typedef std::map<std::pair<int, int>, int> ProcessorConnections;
  typedef std::map<std::pair<int, int>, double> ProcessorInboundBandwidths;
  typedef std::map<std::pair<int, int>, double> ProcessorInboundStubsPerEvent;
  ProcessorConnections processorConnections_;
  ProcessorInboundBandwidths processorInboundBandwidths_;
  ProcessorInboundStubsPerEvent processorInboundStubsPerEvent_;

  SummaryTable &processorConnectionSummary_, &processorInboundBandwidthSummary_, &processorInboundStubsPerEventSummary_;
  TH1I& moduleConnectionsDistribution_;


  struct ModuleConnectionData {
    Property<int, Default> phiCpuConnections, etaCpuConnections;
    Property<int, Computable> totalCpuConnections;
    ConnectionData() : phiCpuConnections(0), etaCpuConnections(0), totalCpuConnections([&]() { return phiCpuConnections()*etaCpuConnections(); }) {}
  };
  map<Module*,ModuleConnectionData> moduleConnections_;

  int numProcEta, numProcPhi;

  double inboundBandwidthTotal = 0.;
  int processorConnectionsTotal = 0;
  double inboundStubsPerEventTotal = 0.;
public:

  TriggerProcessorBandwidthVisitor(SummaryTable& processorConnectionSummary, SummaryTable& processorInboundBandwidthSummary, SummaryTable& processorInboundStubsPerEventSummary, TH1I& moduleConnectionsDistribution) :
      processorConnectionSummary_(processorConnectionSummary),
      processorInboundBandwidthSummary_(processorInboundBandwidthSummary),
      processorInboundStubsPerEventSummary_(processorInboundStubsPerEventSummary),
      moduleConnectionsDistribution_(moduleConnectionsDistribution)
  {}

  void preVisit() {
    processorConnectionSummary_.setHeader("Phi", "Eta");
    processorInboundBandwidthSummary_.setHeader("Phi", "Eta");
    processorInboundStubPerEventSummary_.setHeader("Phi", "Eta");

    processorInboundBandwidthSummary_.setPrecision(3);
    processorInboundStubPerEventSummary_.setPrecision(3);

    numProcEta = tracker.getTriggerProcessorsEta();
    numProcPhi = tracker.getTriggerProcessorsPhi();

    moduleConnectionsDistribution.Reset();
    moduleConnectionsDistribution.SetNameTitle("ModuleConnDist", "Number of connections to trigger processors;Connections;Modules");
    moduleConnectionsDistribution.SetBins(11, -.5, 10.5);
  }

  void doVisit(const Module& m) {
    SummaryPath p = summaryPath();

    int etaConnections = 0, totalConnections = 0;

    for (int i=0; i < numProcEta; i++) {
      if (VisitorHelpers::isModuleInEtaSector(tracker(), m, i)) {
        etaConnections++;
        for (int j=0; j < numProcPhi; j++) {
          if (VisitorHelpers::isModuleInPhiSector(tracker(), m, j)) {
            totalConnections++;

            processorConnections_[std::make_pair(j,i)] += 1;
            processorConnectionSummary_.setCell(j+1, i+1, processorConnections_[std::make_pair(j,i)]);

            processorInboundBandwidths_[std::make_pair(j,i)] += triggerDataBandwidths_[p.table][make_pair(p.row, p.col)]; // *2 takes into account negative Z's
            processorInboundBandwidthSummary_.setCell(j+1, i+1, processorInboundBandwidths_[std::make_pair(j,i)]);

            processorInboundStubsPerEvent_[std::make_pair(j,i)] += triggerFrequenciesPerEvent_[p.table][make_pair(p.row, p.col)];
            processorInboundStubPerEventSummary_.setCell(j+1, i+1, processorInboundStubsPerEvent_[std::make_pair(j,i)]);

          } 
        }
      }
    }
    moduleConnections[&module].etaPhiCpuConnections(etaConnections, totalConnections > 0 ? totalConnections/etaConnections : 0);

    for (const auto& mvp : processorInboundBandwidths_) inboundBandwidthTotal += mvp.second;
    for (const auto& mvp : processorConnections_) processorConnectionsTotal += mvp.second;
    for (const auto& mvp : processorInboundStubsPerEvent_) inboundStubsPerEventTotal += mvp.second;
  }

  void postVisit() {
    processorInboundBandwidthSummary_.setSummaryCell("Total", inboundBandwidthTotal);
    processorConnectionSummary_.setSummaryCell("Total", processorConnectionsTotal);
    processorInboundStubPerEventSummary_.setSummaryCell("Total", inboundStubsPerEventTotal);

    for (auto mvp : connectionMap) moduleConnectionsDistribution.Fill(mvp.second.totalCpuConnections(), 1);
  }
};




class IrradiationPowerVisitor : public AnalyzerVisitor {
  double numInvFemtobarns;
  double operatingTemp;
  double chargeDepletionVoltage;
  double alphaParam;
  double referenceTemp;
  IrradiationMap& irradiationMap_;
  MultiSummaryTable& irradiatedPowerConsumptionSummaries_;
public:
  IrradiationPowerVisitor(MultiSummaryTable& irradiatedPowerConsumptionSummaries) : irradiatedPowerConsumptionSummaries_(irradiatedPowerConsumptionSummaries) {}

  void preVisit() {
    irradiatedPowerConsumptionSummaries_.clear();   
  }

  void doVisit(const Tracker& t) {
    numInvFemtobarns = t.numInvFemtobarns();
    operatingTemp    = t.operatingTemp();
    chargeDepletionVoltage    = t.chargeDepletionVoltage();
    alphaParam       = t.alphaParam();
    referenceTemp    = t.referenceTemp();
    irradiationMap   = &t.irradiationMap();
  }

  void doVisit(const Barrel& b) {
    irradiatedPowerConsumptionSummaries_[summaryPath().table].setHeader("layer", "ring");
    irradiatedPowerConsumptionSummaries_[summaryPath().table].setPrecision(3);        
  }

  void doVisit(const Endcap& e) {
    irradiatedPowerConsumptionSummaries_[summaryPath().table].setHeader("layer", "ring");
    irradiatedPowerConsumptionSummaries_[summaryPath().table].setPrecision(3);        
  }

  void doVisit(const Module& m) {
    XYZVector center = m->center();
    if (center.Z()<0) continue;
    double volume  = module->sensorThickness() * module->area() / 1000.0 * module->numFaces(); // volume is in cm^3
    double x  = center.Z()/25;
    double y  = center.Rho()/25;
    double x1 = floor(x);
    double x2 = ceil(x);
    double y1 = floor(y);
    double y2 = ceil(y);
    double irr11 = irradiationMap_[make_pair(int(x1), int(y1))]; 
    double irr21 = irradiationMap_[make_pair(int(x2), int(y1))];
    double irr12 = irradiationMap_[make_pair(int(x1), int(y2))];
    double irr22 = irradiationMap_[make_pair(int(x2), int(y2))];
    double irrxy = irr11/((x2-x1)*(y2-y1))*(x2-x)*(y2-y) + irr21/((x2-x1)*(y2-y1))*(x-x1)*(y2-y) + irr12/((x2-x1)*(y2-y1))*(x2-x)*(y-y1) + irr22/((x2-x1)*(y2-y1))*(x-x1)*(y-y1); // bilinear interpolation
    double fluence = irrxy * numInvFemtobarns * 1e15 * 77 * 1e-3; // fluence is in 1MeV-equiv-neutrons/cm^2 
    double leakCurrentScaled = alphaParam * fluence * volume * pow((operatingTemp+273.15) / (referenceTemp+273.15), 2) * exp(-1.21/(2*8.617334e-5)*(1/(operatingTemp+273.15)-1/(referenceTemp+273.15))); 
    double irradiatedPowerConsumption = leakCurrentScaled * chargeDepletionVoltage;         
    //cout << "mod irr: " << cntName << "," << module->getLayer() << "," << module->getRing() << ";  " << module->getThickness() << "," << center.Rho() << ";  " << volume << "," << fluence << "," << leakCurrentScaled << "," << irradiatedPowerConsumption << endl;

    //module->setIrradiatedPowerConsumption(irradiatedPowerConsumption); // CUIDADO CHECK WHERE IT IS NEEDED

    irradiatedPowerConsumptionSummaries_[summaryPath().table].setCell(summaryPath().row, summaryPath().col, irradiatedPowerConsumption);
  }
};



class BandwidthVisitor : public AnalyzerVisitor {
  TH1D &chanHitDistribution_, &bandwidthDistribution, &bandwidthDistributionSparsified;

  double nMB_;
public:
  BandwidthVisitor(TH1D& chanHitDistribution, TH1D& bandwidthDistribution, TH1D& bandwidthDistributionSparsified) :
      chanHitDistribution_(chanHitDistribution),
      bandwidthDistribution_(bandwidthDistribution),
      bandwidthDistributionSparsified_(bandwidthDistributionSparsified)
  {}

  void preVisit() {
    chanHitDistribution_.Reset();
    bandwidthDistribution_.Reset();
    bandwidthDistributionSparsified_.Reset();
    chanHitDistribution_.SetNameTitle("NHitChannels", "Number of hit channels;Hit Channels;Modules");
    bandwidthDistribution_.SetNameTitle("BandWidthDist", "Module Needed Bandwidth;Bandwidth (bps);Modules");
    bandwidthDistributionSparsified_.SetNameTitle("BandWidthDistSp", "Module Needed Bandwidth (sparsified);Bandwidth (bps);Modules");
    chanHitDistribution_.SetBins(200, 0., 400);
    bandwidthDistribution_.SetBins(100, 0., 6E+8);
    bandwidthDistributionSparsified_.SetBins(100, 0., 6E+8);
    bandwidthDistribution_.SetLineColor(kBlack);
    bandwidthDistributionSparsified_.SetLineColor(kRed);
  }

  void doVisit(const Tracker& t) {
    nMB_ = t.numMinBiasEvents();
  }

  void doVisit(const DualSensorModule& m) {
    double hitChannels;
    // Clear and reset the histograms

    int nChips;

  //  for (layIt=layerSet.begin(); layIt!=layerSet.end(); layIt++) {
  //   aLay = (*layIt)->getModuleVector();
  //   for (modIt=aLay->begin(); modIt!=aLay->end(); modIt++) {
    if (m.sensors.at(1).type() == SensorType::Strip) {
      for (auto s : m.sensors()) {
        //   for (int nFace=1; nFace<=(*modIt)->getNFaces() ; nFace++) {
        hitChannels = m.hitOccupancyPerEvent()*nMB_*(s.numChannels());
        chanHitDistribution_.Fill(hitChannels);
        nChips = s.numROCs();

        // TODO: place the computing model choice here

        // ACHTUNG!!!! whenever you change the numbers here, you have to change
        // also the numbers in the summary

        // Binary unsparsified (bps)
        bandwidthDistribution_.Fill((16*nChips + s.numChannels())*100E3);

        int spHdr = m.sparsifiedHeaderBits();
        int spPay = m.sparsifiedPayloadBits();      

        //cout << "sparsified header: " << spHdr << " payload: " << spPay << endl;
        // Binary sparsified
        bandwidthDistributionSparsified_.Fill(((spHdr*nChips)+(hitChannels*spPay))*100E3);
      }
    }

    savingGeometryV.push_back(chanHitDistribution_);
    savingGeometryV.push_back(bandwidthDistribution_);
    savingGeometryV.push_back(bandwidthDistributionSparsified_);
  }
};


class TriggerDistanceTuningPlotsVisitor : public AnalyzerVisitor {

  std::map<std::string, ModuleVector> selectedModules_;
  std::vector<double> spacingOptions_;
  const unsigned int nWindows_ = 5;

  SummaryTable &optimalSpacingDistribution_, &optimalSpacingDistributionAW_;
  std::map<std::string, bool> preparedProfiles_;
  std::map<std::string, bool> preparedTurnOn_;

  double findXThreshold(const TProfile& aProfile, const double& yThreshold, const bool& goForward) {  
    // TODO: add a linear interpolation here
    if (goForward) {
      double xThreshold=0;
      double binContent;
      for (int i=1; i<=aProfile.GetNbinsX(); ++i) {
        binContent = aProfile.GetBinContent(i);
        if ((binContent!=0)&&(binContent<yThreshold)) {
          // TODO: add a linear interpolation here
          xThreshold = aProfile.GetBinCenter(i);
          return xThreshold;
        }
      }
      return 100;
    } else {
      double xThreshold=100;
      double binContent;
      for (int i=aProfile.GetNbinsX(); i>=1; --i) {
        binContent = aProfile.GetBinContent(i);
        if ((binContent!=0)&&(binContent>yThreshold)) {
          // TODO: add a linear interpolation here
          xThreshold = aProfile.GetBinCenter(i);
          return xThreshold;
        }
      }
      return 0;
    }
  }
 
  void doVisitBarrelPtModule(const PtModule& aModule) {
    double myValue;

    std::string myBaseName;

    std::ostringstream tempSS;
    std::string myName = barrelPath().barrel() + "_" + barrelPath().layer();

    std::vector<Module*>& theseBarrelModules = selectedModules[myName];

    if (aModule.dsDistance()<=0) || (aModule.triggerWindow()==0) continue;

    // Prepare the variables to hold the profiles
    std::map<double, TProfile>& tuningProfiles = myProfileBag.getNamedProfiles(profileBag::TriggerProfileName + myName);
    // Prepare the variables to hold the turn-on curve profiles
    std::map<double, TProfile>& turnonProfiles = myProfileBag.getNamedProfiles(profileBag::TurnOnCurveName + myName);

    //  Profiles
    if (!preparedProfiles_[myName]) {
      preparedProfiles_[myName] = true;
      for (std::vector<double>::const_iterator it=triggerMomenta.begin(); it!=triggerMomenta.end(); ++it) {
        tempSS.str(""); tempSS << "Trigger efficiency for " << myName.c_str() << ";Sensor spacing [mm];Efficiency [%]";
        tuningProfiles[*it].SetTitle(tempSS.str().c_str());
        tempSS.str(""); tempSS << "TrigEff" << myName.c_str() << "_" << (*it) << "GeV";
        tuningProfiles[*it].SetName(tempSS.str().c_str());
        tuningProfiles[*it].SetBins(100, 0.5, 6); // TODO: these numbers should go into some kind of const
      }     
    }

    // Turn-on curve
    if (!preparedTurnOn[myName]) {
      preparedTurnOn[myName] = true;
      for (unsigned int iWindow=0; iWindow<nWindows; ++iWindow) {
        double windowSize=iWindow*2+1;
        tempSS.str(""); tempSS << "Trigger efficiency for " << myName.c_str() << ";p_{T} [GeV/c];Efficiency [%]";
        turnonProfiles[windowSize].SetTitle(tempSS.str().c_str());
        tempSS.str(""); tempSS << "TurnOn" << myName.c_str() << "_window" << int(windowSize);
        turnonProfiles[windowSize].SetName(tempSS.str().c_str());
        turnonProfiles[windowSize].SetBins(100, 0.5, 10); // TODO: these numbers should go into some kind of const
      }     
    }


    XYZVector center = aModule.center();
    if ((center.Z()<0) || (center.Phi()<0) || (center.Phi()>M_PI/2)) continue;
    theseBarrelModules.push_back(aModule);

    // Fill the tuning profiles for the windows actually set
    for (double dist=0.5; dist<=6; dist+=0.02) {
      for (std::vector<double>::const_iterator it=triggerMomenta.begin(); it!=triggerMomenta.end(); ++it) {
        double myPt = (*it);
        myValue = 100 * aModule.triggerProbability(myPt, dist);
        if ((myValue>=0) && (myValue<=100))
          tuningProfiles[myPt].Fill(dist, myValue);
      }
    }

    // Fill the turnon curves profiles for the distance actually set
    for (double myPt=0.5; myPt<=10; myPt+=0.02) {
      for (unsigned int iWindow=0; iWindow<nWindows; ++iWindow) {
        double windowSize=iWindow*2+1;
        double distance = aModule.dsDistance();
        myValue = 100 * aModule.triggerProbability(myPt, distance, int(windowSize));
        if ((myValue>=0) && (myValue<=100))
          turnonProfiles[windowSize].Fill(myPt, myValue);
      }
    }
  }


  void doVisit(const EndcapModule& aModule) {
  // If it's an endcap layer, scan over all disk 1 modules

  // Sort the plot by ring (using only disk 1)
    tempSS << "_D";
    tempSS.width(2).fill('0'); 
    tempSS << endcapPath().disk(); 
    myBaseName = aLayer.getContainerName() + tempSS.str();

    // Actually scan the modules
    if ((aModule.getStereoDistance()<=0) || (aModule->getTriggerWindow()==0)) continue;

    XYZVector center = aModule->getMeanPoint();
    // std::cerr << myBaseName << " z=" << center.Z() << ", Phi=" << center.Phi() << ", Rho=" << center.Rho() << std::endl; // debug
    if ((center.Z()<0) || (center.Phi()<0) || (center.Phi()>M_PI/2)) continue;


    tempSS.str("");
    tempSS << "R";
    tempSS.width(2); tempSS.fill('0');
    tempSS << endcapPath().ring();
    myName = myBaseName + tempSS.str();
    ModuleVector& theseEndcapModules = selectedModules[myName];
    theseEndcapModules.push_back(aModule);

              // Prepare the variables to hold the profiles
    std::map<double, TProfile>& tuningProfiles = myProfileBag.getNamedProfiles(profileBag::TriggerProfileName + myName);
    // Prepare the variables to hold the turn-on curve profiles
    std::map<double, TProfile>& turnonProfiles = myProfileBag.getNamedProfiles(profileBag::TurnOnCurveName + myName);

    // Tuning profile
    if (!preparedProfiles[myName]) {
      preparedProfiles[myName] = true;
      for (std::vector<double>::const_iterator it=triggerMomenta.begin(); it!=triggerMomenta.end(); ++it) {
        tempSS.str(""); tempSS << "Trigger efficiency for " << myName.c_str() << " GeV;Sensor spacing [mm];Efficiency [%]";
        tuningProfiles[*it].SetTitle(tempSS.str().c_str());
        tempSS.str(""); tempSS << "TrigEff" << myName.c_str() << "_" << (*it);
        tuningProfiles[*it].SetName(tempSS.str().c_str());
        tuningProfiles[*it].SetBins(100, 0.5, 6); // TODO: these numbers should go into some kind of const
      }
    }

    // Turn-on curve
    if (!preparedTurnOn[myName]) {
      preparedTurnOn[myName] = true;
      for (unsigned int iWindow=0; iWindow<nWindows; ++iWindow) {
        double windowSize=iWindow*2+1;
        tempSS.str(""); tempSS << "Trigger efficiency for " << myName.c_str() << ";p_{T} [GeV/c];Efficiency [%]";
        turnonProfiles[windowSize].SetTitle(tempSS.str().c_str());
        tempSS.str(""); tempSS << "TurnOn" << myName.c_str() << "_window" << windowSize;
        turnonProfiles[windowSize].SetName(tempSS.str().c_str());
        turnonProfiles[windowSize].SetBins(100, 0.5, 10); // TODO: these numbers should go into some kind of const
      }     
    }



    // Fill the tuning profiles for the windows actually set
    for (double dist=0.5; dist<=6; dist+=0.02) {
      for (std::vector<double>::const_iterator it=triggerMomenta.begin(); it!=triggerMomenta.end(); ++it) {
        double myPt = (*it);
        myValue = 100 * aModule->getTriggerProbability(myPt, dist);
        if ((myValue>=0) && (myValue<=100))
          tuningProfiles[myPt].Fill(dist, myValue);
      }
    }

    // Fill the turnon curves profiles for the distance actually set
    for (double myPt=0.5; myPt<=10; myPt+=0.02) {
      for (unsigned int iWindow=0; iWindow<nWindows; ++iWindow) {
        double windowSize=iWindow*2+1;
        double distance = aModule->getStereoDistance();
        myValue = 100 * aModule->getTriggerProbability(myPt, distance, int(windowSize));
        if ((myValue>=0) && (myValue<=100))
          turnonProfiles[windowSize].Fill(myPt, myValue);
      }
    }
  }

public:
  TriggerDistanceTuningPlotsVisitor(SummaryTable& optimalSpacingDistribution, SummaryTable& optimalSpacingDistributionAW, const std::vector<double>& triggerMomenta) {

    /************************************************/

    // TODO: clear only the relevant ones?
    myProfileBag.clearTriggerNamedProfiles();
    optimalSpacingDistribution_.SetName("optimalSpacing");
    optimalSpacingDistribution_.SetTitle("Optimal spacing [default window]");
    optimalSpacingDistribution_.SetXTitle("Spacing [mm]");
    optimalSpacingDistribution_.SetYTitle("# modules");
    optimalSpacingDistribution_.SetBins(100, 0.5, 6);
    optimalSpacingDistribution_.Reset();

    optimalSpacingDistributionAW_.SetName("optimalSpacingAW");
    optimalSpacingDistributionAW_.SetTitle("Optimal spacing [actual window]");
    optimalSpacingDistributionAW_.SetXTitle("Spacing [mm]");
    optimalSpacingDistributionAW_.SetYTitle("# modules");
    optimalSpacingDistributionAW_.SetBins(100, 0.5, 6);
    optimalSpacingDistributionAW_.Reset();



  }

  void doVisit(const Tracker& tracker) {
    fillAvailableSpacing(tracker, spacingOptions_);
  }


  void doVisit(const PtModule& aModule) {
    if (isBarrelModule()) doVisitBarrelPtModule(aModule);
    else doVisitEndcapPtModule(aModule);
  }



  void postVisit() {
      // TODO: put also the limits into a configurable parameter
      // Scan again over the plots I just made in order to find the
      // interesting range, if we have 1 and 2 in the range (TODO: find
      // a better way to find the interesting margins)

      // Run once per possible position in the tracker
    unsigned int nSpacingOptions = spacingOptions.size();             // TODO: keep this here!!

    std::vector<std::string> profileNames = myProfileBag.getProfileNames(profileBag::TriggerProfileName);
    for (std::vector<std::string>::const_iterator itName=profileNames.begin(); itName!=profileNames.end(); ++itName) {
      std::map<double, TProfile>& tuningProfiles = myProfileBag.getNamedProfiles(*itName);
      TProfile& lowTuningProfile = tuningProfiles[spacingTuningMomenta_.first];
      TProfile& highTuningProfile = tuningProfiles[spacingTuningMomenta_.second];
      triggerRangeLowLimit[*itName] = findXThreshold(lowTuningProfile, 1, true);
      triggerRangeHighLimit[*itName] = findXThreshold(highTuningProfile, 90, false);
    }

    std::pair<double, double> spacingTuningMomenta;
    spacingTuningMomenta.first = 1.;
    spacingTuningMomenta.second = 2.5;

    // Now loop over the selected modules and build the curves for the
    // hi-lo thingy (sensor spacing tuning)
    int windowSize;
    XYZVector center;
    double myPt;
    TProfile tempProfileLow("tempProfileLow", "", 100, 0.5, 6); // TODO: these numbers should go into some kind of const
    TProfile tempProfileHigh("tempProfileHigh", "", 100, 0.5, 6); // TODO: these numbers should go into some kind of const

    // TODO: IMPORTANT!!!!!! clear the spacing tuning graphs and frame here
    spacingTuningFrame.SetBins(selectedModules.size(), 0, selectedModules.size());
    spacingTuningFrame.SetYTitle("Optimal distance range [mm]");
    spacingTuningFrame.SetMinimum(0);
    spacingTuningFrame.SetMaximum(6);
    TAxis* xAxis = spacingTuningFrame.GetXaxis();

    int iType=0;
    std::map<double, bool> availableThinkness;
    // Loop over the selected module types
    for(std::map<std::string, ModuleVector>::iterator itTypes = selectedModules.begin();
        itTypes!=selectedModules.end(); ++itTypes) {
      const std::string& myName = itTypes->first;
      const ModuleVector& myModules = itTypes->second;
      xAxis->SetBinLabel(iType+1, myName.c_str());

      // Loop over the possible search windows
      for (unsigned int iWindow = 0; iWindow<nWindows; ++iWindow) {
        windowSize = 1 + iWindow * 2;
        // Loop over the modules of type myName
        for (ModuleVector::const_iterator itModule = myModules.begin(); itModule!=myModules.end(); ++itModule) {
          aModule = (*itModule);
          // Loop over the possible distances
          double minDistBelow = 0.;
          availableThinkness[aModule->getStereoDistance()] = true;
          for (double dist=0.5; dist<=6; dist+=0.02) { // TODO: constant here
            // First with the high momentum
            myPt = (spacingTuningMomenta.second);
            myValue = 100 * aModule->getTriggerProbability(myPt, dist, windowSize);
            if ((myValue>=0)&&(myValue<=100))
              tempProfileHigh.Fill(dist, myValue);
            // Then with low momentum
            myPt = (spacingTuningMomenta.first);
            myValue = 100 * aModule->getTriggerProbability(myPt, dist, windowSize);
            if ((myValue>=0)&&(myValue<=100))
              tempProfileLow.Fill(dist, myValue);
            if (myValue>1) minDistBelow = dist;
          }
          if (minDistBelow>=0) {
            if (windowSize==5) optimalSpacingDistribution.Fill(minDistBelow);
            if (windowSize==aModule->getTriggerWindow()) optimalSpacingDistributionAW.Fill(minDistBelow);
          }

          /*if ((myName=="ENDCAP_D02R05")&&(windowSize==5)) { // debug
            std::cout << myName << " - " << aModule->getTag() << " - minDistBelow = " << minDistBelow;
            std::cout <<  ", so[0]=" << spacingOptions[0] << ", so[n-1]=" << spacingOptions[nSpacingOptions-1];
            }*/
          if (minDistBelow<spacingOptions[0]) minDistBelow=spacingOptions[0];
          else if (minDistBelow>spacingOptions[nSpacingOptions-1]) minDistBelow=spacingOptions[nSpacingOptions-1];
          else {
            for (unsigned int iSpacing = 0; iSpacing < nSpacingOptions-1; ++iSpacing) {
              /*if ((myName=="ENDCAP_D02R05")&&(windowSize==5)) {// debug
                std::cout << " spacingOptions[" << iSpacing << "] = " << spacingOptions[iSpacing];
                std::cout << " spacingOptions[" << iSpacing+1 << "] = " << spacingOptions[iSpacing+1];
                }*/
              if ((minDistBelow>=spacingOptions[iSpacing]) && (minDistBelow<spacingOptions[iSpacing+1])) {
                minDistBelow=spacingOptions[iSpacing+1];
                /*if ((myName=="ENDCAP_D02R05")&&(windowSize==5)) // debug
                  std::cout << " here it is: ";*/
                break;
              }
            }
          }
          /*if ((myName=="ENDCAP_D02R05")&&(windowSize==5)) // debug
            std::cout << " - approx to " << minDistBelow << " for a window of " << windowSize << std::endl;*/
          aModule->setOptimalSpacing(windowSize, minDistBelow);
        }
        // Find the "high" and "low" points
        double lowEdge = findXThreshold(tempProfileLow, 1, true);
        double highEdge = findXThreshold(tempProfileHigh, 90, false);
        // std::cerr << myName << ": " << lowEdge << " -> " << highEdge << std::endl; // debug
        double centerX; double sizeX;
        centerX = iType+(double(iWindow)+0.5)/(double(nWindows));
        sizeX = 1./ (double(nWindows)) * 0.8; // 80% of available space, so that they will not touch
        if (lowEdge<highEdge) {
          spacingTuningGraphs[iWindow].SetPoint(iType, centerX, (highEdge+lowEdge)/2.);
          spacingTuningGraphs[iWindow].SetPointError(iType, sizeX/2., (highEdge-lowEdge)/2.);
        } else {
          spacingTuningGraphsBad[iWindow].SetPoint(iType, centerX, (highEdge+lowEdge)/2.);
          spacingTuningGraphsBad[iWindow].SetPointError(iType, sizeX/2., (highEdge-lowEdge)/2.);
        }
        tempProfileLow.Reset();
        tempProfileHigh.Reset();
      }
      iType++;
    }

    // TODO: properly reset this!
    TGraphErrors& antani = spacingTuningGraphs[-1];
    int iPoints=0;
    for (std::map<double, bool>::iterator it = availableThinkness.begin(); it!= availableThinkness.end(); ++it) {
      iPoints++;
      antani.SetPoint(iPoints, selectedModules.size()/2., it->first);
      antani.SetPointError(iPoints, selectedModules.size()/2., 0);
      iPoints++;
    }
  }
};




class TriggerFrequencyVisitor : public AnalyzerVisitor {

  std::map<std::string, std::map<std::pair<int,int>, int> >   triggerFrequencyCounts;
  std::map<std::string, std::map<std::pair<int,int>, double> > triggerFrequencyAverageTrue, triggerFrequencyInterestingParticleTrue, triggerFrequencyAverageFake; // trigger frequency by module in Z and R, averaged over Phi

  void setupSummaries(const string& cntName) {
    triggerFrequencyTrueSummaries_[cntName].setHeader("Layer", "Ring");
    triggerFrequencyFakeSummaries_[cntName].setHeader("Layer", "Ring");
    triggerRateSummaries_[cntName].setHeader("Layer", "Ring");
    triggerPuritySummaries_[cntName].setHeader("Layer", "Ring");
    triggerDataBandwidthSummaries_[cntName].setHeader("Layer", "Ring");
    triggerFrequencyTrueSummaries_[cntName].setPrecision(3);
    triggerFrequencyFakeSummaries_[cntName].setPrecision(3);
    triggerRateSummaries_[cntName].setPrecision(3);
    triggerPuritySummaries_[cntName].setPrecision(3);
    triggerDataBandwidthSummaries_[cntName].setPrecision(3);
  }

  int nbins_;
public:
  TriggerFrequencyVisitor(Analyzer& analyzer) : analyzer_(analyzer) {
    triggerFrequencyTrueSummaries_.clear();
    triggerFrequencyFakeSummaries_.clear();
    triggerRateSummaries_.clear();
    triggerPuritySummaries_.clear();
    triggerDataBandwidthSummaries_.clear();
    triggerDataBandwidths_.clear();
  }

  void doVisit(const Layer& l) {
    setupSummaries(summaryPath().cnt);
    nbins_ = l.numModulesPerRod();
  }

  void doVisit(const Disk& d) {
    setupSummaries(summaryPath().cnt);
    nbins_ = d.numRings();
  }

  void doVisit(const Module& module) {
    std::string cntName = summaryPath().cnt;

    XYZVector center = module.getCenter();
    if ((center.Z()<0) || (center.Phi()<0) || (center.Phi()>M_PI/2) || (module.dsDistance()==0.0)) continue;

      TH1D* currentTotalHisto;
      TH1D* currentTrueHisto;

      string cntName = summaryPath().cnt;
      int layerIndex = summaryPath().row;
      int ringIndex = summaryPath().col;

      if (totalStubRateHistos_.count(std::make_pair(cntName, layerIndex)) == 0) {
        currentTotalHisto = new TH1D(("totalStubsPerEventHisto" + cntName + any2str(layerIndex)).c_str(), ";Modules;MHz/cm^2", nbins_, 0.5, nbins_+0.5);
        currentTrueHisto = new TH1D(("trueStubsPerEventHisto" + cntName + any2str(layerIndex)).c_str(), ";Modules;MHz/cm^2", nbins_, 0.5, nbins_+0.5); 
        totalStubRateHistos_[std::make_pair(cntName, layerIndex)] = currentTotalHisto; 
        trueStubRateHistos_[std::make_pair(cntName, layerIndex)] = currentTrueHisto; 
      } else {
        currentTotalHisto = totalStubRateHistos_[std::make_pair(cntName, layerIndex)]; 
        currentTrueHisto = trueStubRateHistos_[std::make_pair(cntName, layerIndex)]; 
      }


      int curCnt = triggerFrequencyCounts[cntName][make_pair(layerIndex, ringIndex)]++;
      double curAvgTrue = triggerFrequencyAverageTrue[cntName][make_pair(layerIndex, ringIndex)];
      double curAvgInteresting = triggerFrequencyInterestingParticleTrue[cntName][make_pair(layerIndex, ringIndex)];
      double curAvgFake = triggerFrequencyAverageFake[cntName][make_pair(layerIndex, ringIndex)];

      //curAvgTrue  = curAvgTrue + (module->getTriggerFrequencyTruePerEvent()*tracker.getNMB() - curAvgTrue)/(curCnt+1);
      //curAvgFake  = curAvgFake + (module->getTriggerFrequencyFakePerEvent()*pow(tracker.getNMB(),2) - curAvgFake)/(curCnt+1); // triggerFrequencyFake scales with the square of Nmb!

      // TODO! Important <- make this interestingPt cut configurable
      const double interestingPt = 2;
      curAvgTrue  = curAvgTrue + (module.triggerFrequencyTruePerEventAbove(interestingPt)*tracker.getNMB() - curAvgTrue)/(curCnt+1);
      curAvgInteresting += (module.particleFrequencyPerEventAbove(interestingPt)*tracker.getNMB() - curAvgInteresting)/(curCnt+1);
      curAvgFake  = curAvgFake + ((module.triggerFrequencyFakePerEvent()*tracker.getNMB() + module.triggerFrequencyTruePerEventBelow(interestingPt))*tracker.getNMB() - curAvgFake)/(curCnt+1); // triggerFrequencyFake scales with the square of Nmb!

      double curAvgTotal = curAvgTrue + curAvgFake;

      triggerFrequencyAverageTrue[cntName][make_pair(module->getLayer(), module->getRing())] = curAvgTrue;            
      triggerFrequencyInterestingParticleTrue[cntName][make_pair(module->getLayer(), module->getRing())] = curAvgInteresting;    
      triggerFrequencyAverageFake[cntName][make_pair(module->getLayer(), module->getRing())] = curAvgFake;    

      int triggerDataHeaderBits  = tracker.getModuleType(module->getType()).getTriggerDataHeaderBits();
      int triggerDataPayloadBits = tracker.getModuleType(module->getType()).getTriggerDataPayloadBits();
      double triggerDataBandwidth = (triggerDataHeaderBits + curAvgTotal*triggerDataPayloadBits) / (tracker.getBunchSpacingNs()); // GIGABIT/second
      triggerDataBandwidths_[cntName][make_pair(module->getLayer(), module->getRing())] = triggerDataBandwidth;
      triggerFrequenciesPerEvent_[cntName][make_pair(module->getLayer(), module->getRing())] = curAvgTotal;

      module->setProperty("triggerDataBandwidth", triggerDataBandwidth); // averaged over phi
      module->setProperty("triggerFrequencyPerEvent", curAvgTotal); // averaged over phi


      //                currentTotalGraph->SetPoint(module->getRing()-1, module->getRing(), curAvgTotal*(1000/tracker.getBunchSpacingNs())*(100/module->getArea()));
      //                currentTrueGraph->SetPoint(module->getRing()-1, module->getRing(), curAvgTrue*(1000/tracker.getBunchSpacingNs())*(100/module->getArea()));

      currentTotalHisto->SetBinContent(module->getRing(), curAvgTotal*(1000/tracker.getBunchSpacingNs())*(100/module->getArea()));
      currentTrueHisto->SetBinContent(module->getRing(), curAvgTrue*(1000/tracker.getBunchSpacingNs())*(100/module->getArea()));

      triggerFrequencyTrueSummaries_[cntName].setCell(module->getLayer(), module->getRing(), curAvgTrue);
      triggerFrequencyInterestingSummaries_[cntName].setCell(module->getLayer(), module->getRing(), curAvgInteresting);
      triggerFrequencyFakeSummaries_[cntName].setCell(module->getLayer(), module->getRing(), curAvgFake);
      triggerRateSummaries_[cntName].setCell(module->getLayer(), module->getRing(), curAvgTotal);             
      triggerEfficiencySummaries_[cntName].setCell(module->getLayer(), module->getRing(), curAvgTrue/curAvgInteresting);                
      triggerPuritySummaries_[cntName].setCell(module->getLayer(), module->getRing(), curAvgTrue/(curAvgTrue+curAvgFake));                
      triggerDataBandwidthSummaries_[cntName].setCell(module->getLayer(), module->getRing(), triggerDataBandwidth);

    }
  }

};


namespace AnalyzerHelpers {

  void drawModuleOnMap(const Module& m, double val, TH2D& map, TH2D& counter) {
    const Polygon3d<4>& poly = m.basePoly();
    XYZVector start = (poly.getVertex(0) + poly.getVertex(1))/2;
    XYZVector end = (poly.getVertex(2) + poly.getVertex(3))/2;
    XYZVector diff = end-start;
    XYZVector point;
    for (double l=0; l<=1; l+=0.1) {
      point = start + l * diff;
      map.Fill(point.Z(), point.Rho(), val);
      counter.Fill(point.Z(), point.Rho(), 1);
    }
  }
  void drawModuleOnMap(const Module& m, double val, TH2D& map) {
    const Polygon3d<4>& poly = m.basePoly();
    XYZVector start = (poly.getVertex(0) + poly.getVertex(1))/2;
    XYZVector end = (poly.getVertex(2) + poly.getVertex(3))/2;
    XYZVector diff = end-start;
    XYZVector point;
    for (double l=0; l<=1; l+=0.1) {
      point = start + l * diff;
      map.Fill(point.Z(), point.Rho(), val);
    }
  }

}



class TriggerEfficiencyMapVisitor : public AnalyzerVisitor {
  double myPt;
  TH2D& myMap;
  TH2D* counter;
public:
  PtThresholdMapVisitor(TH2D& map, double pt) : myMap(map), myPt(pt) { counter = (TH2D*)map.Clone(); }

  void doVisit(const DualSensorModule& aModule) {
    double myValue = aModule.triggerProbability(myPt);
    if (myValue>=0) VisitorHelpers::drawModuleOnMap(aModule, myValue, myMap, counter);
  }

  void postVisit() {
    for (int i=1; i<=myMap.GetNbinsX(); ++i)
      for (int j=1; j<=myMap.GetNbinsY(); ++j)
        if (counter->GetBinContent(i,j)!=0)
          myMap.SetBinContent(i,j, myMap.GetBinContent(i,j) / counter->GetBinContent(i,j));
    // ... and get rid of the counter
  }

  ~TriggerEfficiencyMapVisitor() { delete counter; }
};





class PtThresholdMapVisitor : public TriggerPerformanceVisitor {
  double myPt;
  TH2D& myMap;
  TH2D counter;
public:
  PtThresholdMapVisitor(TH2D& map, double pt) : myMap(map), myPt(pt) { counter = (TH2D*)map.Clone(); }

  void doVisit(const DualSensorModule& aModule) {
    double myValue = aModule.ptThreshold(myPt);
    if (myValue >= 0) AnalyzerHelpers::drawModuleOnMap(aModule, myValue, myMap, counter);
  }

  void postVisit() {
    for (int i=1; i<=myMap.GetNbinsX(); ++i)
      for (int j=1; j<=myMap.GetNbinsY(); ++j)
        if (counter()->GetBinContent(i,j)!=0)
          myMap.SetBinContent(i,j, myMap.GetBinContent(i,j) / counter()->GetBinContent(i,j));
    // ... and get rid of the counter
  }

  ~PtThresholdMapVisitor() { delete counter; }
};





class SpacingCutVisitor : public TriggerPerformanceVisitor {
  TH2D& thicknessMap;
  TH2D& windowMap;
  TH2D& suggestedSpacingMap;
  TH2D& suggestedSpacingMapAW;
  TH2D& nominalCutMap;
  TH2D *counter, *counterSpacing, *counterSpacingAW;
public:
  ModuleSpacingVisitor(TH2D& thicknessMap_, TH2D& windowMap_, TH2D& suggestedSpacingMap_, TH2D& suggestedSpacingMapAW_, TH2D& nominalCutMap_) : 
      thicknessMap(thicknessMap_), windowMap(windowMap_), suggestedSpacingMap(suggestedSpacingMap), suggestedSpacingMapAW(suggestedSpacingMapAW_), nominalCutMap(nominalCutMap_) {
        counter = (TH2D*)thicknessMap.Clone();
        counterSpacing = (TH2D*)suggestedSpacingMap.Clone();
        counterSpacingAW = (TH2D*)suggestedSpacingMapAW.Clone();
  }

  void doVisit(const DualSensorModule& aModule) {
    if (!aModule.ptCapable()) continue;
    double myThickness = aModule.thickness();
    double myWindow = aModule.triggerWindow();
    //myWindowmm = myWindow * (aModule->getLowPitch() + aModule->getHighPitch())/2.;
    double mySuggestedSpacing = aModule.optimalSpacingWithTriggerWindow(5); // TODO: put this 5 in a configuration of some sort
    double mySuggestedSpacingAW = aModule.optimalSpacingWithTriggerWindow(aModule.triggerWindow());
    double nominalCut = aModule.ptCut();

    drawModuleOnMap(aModule, myThickness, thicknessMap);
    drawModuleOnMap(aModule, myWindow, windowMap);
    drawModuleOnMap(aModule, nominalCut, nominalCut);
    if (mySuggestedSpacing != 0) drawModuleOnMap(aModule, mySuggestedSpacing, suggestedSpacingMap, counterSpacing);
    if (mySuggestedSpacingAW != 0) drawModuleOnMap(aModule, mySuggestedSpacingAW, suggestedSpacingMapAW, counterSpacingAW);

  }

  void postVisit() {
    for (int i=1; i<=thicknessMap.GetNbinsX(); ++i) {
      for (int j=1; j<=thicknessMap.GetNbinsY(); ++j) {
        if (counter->GetBinContent(i,j)!=0) {
          thicknessMap.SetBinContent(i,j, thicknessMap.GetBinContent(i,j) / counter->GetBinContent(i,j));
          windowMap.SetBinContent(i,j, windowMap.GetBinContent(i,j) / counter->GetBinContent(i,j));
          nominalCutMap.SetBinContent(i,j, nominalCutMap.GetBinContent(i,j) / counter->GetBinContent(i,j));
          if ((suggestedSpacingMap.GetBinContent(i,j)/counterSpacing->GetBinContent(i,j))>50) {
            std::cout << "debug: for bin " << i << ", " << j << " suggestedSpacing is " << suggestedSpacingMap.GetBinContent(i,j)
              << " and counter is " << counterSpacing->GetBinContent(i,j) << std::endl;
          }
          suggestedSpacingMap.SetBinContent(i,j, suggestedSpacingMap.GetBinContent(i,j) / counterSpacing->GetBinContent(i,j));
          suggestedSpacingMapAW.SetBinContent(i,j, suggestedSpacingMapAW.GetBinContent(i,j) / counterSpacingAW->GetBinContent(i,j));
        }
      }
    }
  }

  ~ModuleSpacingVisitor() {
    delete counter;
    delete counterSpacing;
    delete counterSpacingAW;
  }

};


class IrradiatedPowerMapVisitor {
  TH2D &irradiatedPowerConsumptionMap, &totalPowerConsumptionMap;
  TH2D *counter;
public:
  IrradiatedPowerMapVisitor(TH2D& irradiatedPowerConsumptionMap_, TH2D& totalPowerConsumptionMap_) : irradiatedPowerConsumptionMap(irradiatedPowerConsumptionMap_), totalPowerConsumptionMap(totalPowerConsumptionMap_) {
    counter = (TH2D*)irradiatedPowerConsumptionMap.Clone();
  }
  void visit(const TypedModule& aModule) {
    if ((aModule.center().Z()<0) || (aModule.center().Phi()<0) || (aModule.center().Phi()>M_PI/2)) continue;
    double myPower = aModule.sensorPowerConsumptionAfterIrradiation();
    double myPowerChip = aModule.chipPowerConsumption();

    VisitorHelpers::drawModuleOnMap(aModule, myPower, irradiatedPowerConsumptionMap, counter);
    VisitorHelpers::drawModuleOnMap(aModule, myPower+myPowerChip, totalPowerConsumptionMap); // only the first time counter is updated, but in postVisit both maps are averaged bin for bin over the counter value  
  }

  void postVisit() {
    for (int i=1; i<=irradiatedPowerConsumptionMap.GetNbinsX(); ++i) {
      for (int j=1; j<=irradiatedPowerConsumptionMap.GetNbinsY(); ++j) {
        if (counter->GetBinContent(i,j)!=0) {
          irradiatedPowerConsumptionMap.SetBinContent(i,j, irradiatedPowerConsumptionMap.GetBinContent(i,j) / counter->GetBinContent(i,j));
          totalPowerConsumptionMap.SetBinContent(i,j, totalPowerConsumptionMap.GetBinContent(i,j) / counter->GetBinContent(i,j));
        }
      }
    }
  }

  ~IrradiatedPowerMapVisitor() { delete counter; }

};



#endif
