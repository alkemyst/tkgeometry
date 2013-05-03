
class AnalyzerVisitor {
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

  virtual void doVisit(const Tracker&) {}
  virtual void doVisit(const Barrel&) {}
  virtual void doVisit(const Layer&) {}
  virtual void doVisit(const RodPair&) {}
  virtual void doVisit(const Endcap&) {}
  virtual void doVisit(const Disk&) {}
  virtual void doVisit(const Ring&) {}
  virtual void doVisit(const Module&) {}
  virtual void doVisit(const BarrelModule& bm) { doVisit((Module&)bm); }
  virtual void doVisit(const EndcapModule& em) { doVisit((Module&)em); }
public:
  void visit(const Tracker& t) { geomPath_.uniformPath_.tracker = t.myid(); doVisit(t); }
  void visit(const Barrel& b)  { geomPath_.barrelPath_.barrel = b.myid();   doVisit(b); }
  void visit(const Layer& l)   { geomPath_.barrelPath_.layer = l.myid();    doVisit(l); }
  void visit(const RodPair& r) { geomPath_.barrelPath_.rod = r.myid();      doVisit(r); } 
  void visit(const Endcap& e)  { geomPath_.endcapPath_.endcap = e.myid();   doVisit(e); }
  void visit(const Disk& d)    { geomPath_.endcapPath_.disk = d.myid();     doVisit(d); }
  void visit(const Ring& r)    { geomPath_.endcapPath_.ring = r.myid();     doVisit(r); }
  void visit(const BarrelModule& bm) { geomPath_.barrelPath_.ring = bm.myid();   doVisit(bm); }
  void visit(const EndcapModule& em) { geomPath_.endcapPath_.blade_ = em.myid(); doVisit(em); }

};



namespace VisitorHelpers {

  bool isModuleInEtaSector(const Tracker& tracker, const Module* module, int etaSector) const {
    int numProcEta = tracker.getTriggerProcessorsEta();
    double etaCut = tracker.getTriggerEtaCut();
    double etaSlice = etaCut*2 / numProcEta;
    double maxR = tracker.getMaxR();
    double zError = tracker.getZError();
    double eta = etaSlice*etaSector-etaCut;    

    double modMinZ = module->getMinZ();
    double modMaxZ = module->getMaxZ();
    double modMinR = module->getMinRho();                
    double modMaxR = module->getMaxRho();                

    double etaSliceZ1 = maxR/tan(2*atan(exp(-eta)));
    double etaSliceZ2 = maxR/tan(2*atan(exp(-eta-etaSlice)));

    double etaDist1 =  modMaxZ - ((etaSliceZ1 >= 0 ? modMinR : modMaxR)*(etaSliceZ1 + zError)/maxR - zError); // if etaDists are positive it means the module is in the slice
    double etaDist2 = -modMinZ + ((etaSliceZ2 >= 0 ? modMaxR : modMinR)*(etaSliceZ2 - zError)/maxR + zError); 

    return etaDist1 > 0 && etaDist2 > 0;
  }


  bool isModuleInPhiSector(const Tracker& tracker, const Module* module, int phiSector) const {
    double phiSlice = 2*M_PI / tracker.getTriggerProcessorsPhi();  // aka Psi
    double phi = phiSlice*phiSector;

    double modMinR = module->getMinRho();                
    double modMaxR = module->getMaxRho();                

    double modMinPhi = module->getMinPhi() >= 0 ? module->getMinPhi() : module->getMinPhi() + 2*M_PI;
    double modMaxPhi = module->getMaxPhi() >= 0 ? module->getMaxPhi() : module->getMaxPhi() + 2*M_PI;

    double trajSlice = asin((modMaxR+modMinR)/2 * 0.0003 * magnetic_field / (2 * tracker.getTriggerPtCut())); // aka Alpha
    double sliceMinPhi = phi - trajSlice;
    double sliceMaxPhi = phi + phiSlice + trajSlice;

    if (modMinPhi > modMaxPhi && sliceMaxPhi > 2*M_PI) modMaxPhi += 2*M_PI;      // this solves the issue with modules across the 2 PI line
    else if (modMinPhi > modMaxPhi && sliceMaxPhi < 2*M_PI) modMinPhi -= 2*M_PI; // 

    return ((sliceMinPhi < modMaxPhi && modMinPhi < sliceMaxPhi) ||
            (sliceMinPhi < modMaxPhi+2*M_PI && modMinPhi+2*M_PI < sliceMaxPhi) || // this catches the modules that are at a small angle but must be caught by a sweep crossing the 2 PI line
            (sliceMinPhi < modMaxPhi-2*M_PI && modMinPhi-2*M_PI < sliceMaxPhi)); 
  }

}



class TriggerProcessorBandwidthVisitor : public AnalyzerVisitor {
  std::map<std::pair<int, int>, int> processorConnections;
  std::map<std::pair<int, int>, double> processorInboundBandwidths;
  std::map<std::pair<int, int>, double> processorInboundStubsPerEvent;

  struct ModuleConnectionData {
    Property<int, Default> phiCpuConnections, etaCpuConnections;
    Property<int, Computable> totalCpuConnections;
    ConnectionData() : phiCpuConnections(0), etaCpuConnections(0), totalCpuConnections([&]() { return phiCpuConnections()*etaCpuConnections(); }) {}
  };
  map<Module*,ModuleConnectionData> moduleConnections;

  int numProcEta, numProcPhi;

  double inboundBandwidthTotal = 0.;
  int processorConnectionsTotal = 0;
  double inboundStubsPerEventTotal = 0.;
public:

  TriggerProcessorBandwidthVisitor() {}

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

            processorConnections[std::make_pair(j,i)] += 1;
            processorConnectionSummary_.setCell(j+1, i+1, processorConnections[std::make_pair(j,i)]);

            processorInboundBandwidths[std::make_pair(j,i)] += triggerDataBandwidths_[p.table][make_pair(p.row, p.col)]; // *2 takes into account negative Z's
            processorInboundBandwidthSummary_.setCell(j+1, i+1, processorInboundBandwidths[std::make_pair(j,i)]);

            processorInboundStubsPerEvent[std::make_pair(j,i)] += triggerFrequenciesPerEvent_[p.table][make_pair(p.row, p.col)];
            processorInboundStubPerEventSummary_.setCell(j+1, i+1, processorInboundStubsPerEvent[std::make_pair(j,i)]);

          } 
        }
      }
    }
    moduleConnections[&module].etaPhiCpuConnections(etaConnections, totalConnections > 0 ? totalConnections/etaConnections : 0);

    for (auto mvp : processorInboundBandwidths) inboundBandwidthTotal += mvp.second;
    for (auto mvp : processorConnections) processorConnectionsTotal += mvp.second;
    for (auto mvp :  processorInboundStubsPerEvent) inboundStubsPerEventTotal += mvp.second;
  }

  void endVisit() {
    processorInboundBandwidthSummary_.setSummaryCell("Total", inboundBandwidthTotal);
    processorConnectionSummary_.setSummaryCell("Total", processorConnectionsTotal);
    processorInboundStubPerEventSummary_.setSummaryCell("Total", inboundStubsPerEventTotal);

    for (auto mvp : connectionMap) moduleConnectionsDistribution.Fill(mvp.second.totalCpuConnections(), 1);
  }
};




class IrradiationPowerVisitor : public AnalyzerVisitor {
  double numInvFemtobarns = tracker.getNumInvFemtobarns();
  double operatingTemp    = tracker.getOperatingTemp();
  double chargeDepletionVoltage    = tracker.getChargeDepletionVoltage();
  double alphaParam       = tracker.getAlphaParam();
  double referenceTemp    = tracker.getReferenceTemp();
public:
  void preVisit() {
    irradiatedPowerConsumptionSummaries_.clear();   

  }
  void doVisit(const Tracker& t) {
    numInvFemtobarns = t.getNumInvFemtobarns();
    operatingTemp    = t.getOperatingTemp();
    chargeDepletionVoltage    = t.getChargeDepletionVoltage();
    alphaParam       = t.getAlphaParam();
    referenceTemp    = t.getReferenceTemp();
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
    double irr11 = phys.irradiationMap()[make_pair(int(x1), int(y1))]; 
    double irr21 = phys.irradiationMap()[make_pair(int(x2), int(y1))];
    double irr12 = phys.irradiationMap()[make_pair(int(x1), int(y2))];
    double irr22 = phys.irradiationMap()[make_pair(int(x2), int(y2))];
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

public:
  void doVisit(const Module& m) {
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
    double nMB = tracker.getNMB();

  //  for (layIt=layerSet.begin(); layIt!=layerSet.end(); layIt++) {
  //   aLay = (*layIt)->getModuleVector();
  //   for (modIt=aLay->begin(); modIt!=aLay->end(); modIt++) {
    if (m.readoutType()==Module::Strip) {
      for (auto f : m.faces()) {
   //   for (int nFace=1; nFace<=(*modIt)->getNFaces() ; nFace++) {
        hitChannels = m.getHitOccupancyPerEvent()*nMB*(f.numChannels());
        chanHitDistribution.Fill(hitChannels);
        nChips = f.numROCs();

        // TODO: place the computing model choice here

        // ACHTUNG!!!! whenever you change the numbers here, you have to change
        // also the numbers in the summary

        // Binary unsparsified (bps)
        bandwidthDistribution.Fill((16*nChips + f.numChannels())*100E3);

        int spHdr = m.sparsifiedHeaderBits();
        int spPay = m.sparsifiedPayloadBits();      

        //cout << "sparsified header: " << spHdr << " payload: " << spPay << endl;
        // Binary sparsified
        bandwidthDistributionSparsified.Fill(((spHdr*nChips)+(hitChannels*spPay))*100E3);
      }
    }

    savingGeometryV.push_back(chanHitDistribution);
    savingGeometryV.push_back(bandwidthDistribution);
    savingGeometryV.push_back(bandwidthDistributionSparsified);
  }
};


class TriggerDistanceTuningPlotsVisitor : public AnalyzerVisitor {

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

public:
  TriggerDistanceTuningPlotsVisitor(Tracker& tracker, const std::vector<double>& triggerMomenta) {

    // TODO: put these in a configuration file somewhere
    /************************************************/
    std::pair<double, double> spacingTuningMomenta;
    std::vector<double> spacingOptions;
    fillAvailableSpacing(tracker, spacingOptions);
    //spacingOptions.push_back(1.1);
    //spacingOptions.push_back(1.8);
    //spacingOptions.push_back(2.5);
    //spacingOptions.push_back(5); // TODO: make these configurable
    //spacingOptions.push_back(0.8);
    //spacingOptions.push_back(1.6);
    //spacingOptions.push_back(3);
    //std::sort(spacingOptions.begin(), spacingOptions.end()); // TODO: keep this here!!
    unsigned int nSpacingOptions = spacingOptions.size();             // TODO: keep this here!!
    spacingTuningMomenta.first = 1.;
    spacingTuningMomenta.second = 2.5;
    const unsigned int nWindows = 5;
    /************************************************/

    // TODO: clear only the relevant ones?
    myProfileBag.clearTriggerNamedProfiles();
    optimalSpacingDistribution.SetName("optimalSpacing");
    optimalSpacingDistribution.SetTitle("Optimal spacing [default window]");
    optimalSpacingDistribution.SetXTitle("Spacing [mm]");
    optimalSpacingDistribution.SetYTitle("# modules");
    optimalSpacingDistribution.SetBins(100, 0.5, 6);
    optimalSpacingDistribution.Reset();

    optimalSpacingDistributionAW.SetName("optimalSpacingAW");
    optimalSpacingDistributionAW.SetTitle("Optimal spacing [actual window]");
    optimalSpacingDistributionAW.SetXTitle("Spacing [mm]");
    optimalSpacingDistributionAW.SetYTitle("# modules");
    optimalSpacingDistributionAW.SetBins(100, 0.5, 6);
    optimalSpacingDistributionAW.Reset();


    std::string myName;
    std::string myBaseName;
    std::map<std::string, bool> preparedProfiles;
    std::map<std::string, bool> preparedTurnOn;

  }


  void doVisit(const BarrelModule& aModule) {

    // Loop over all the tracker
    double myValue;

    std::ostringstream tempSS;


    std::map<std::string, ModuleVector> selectedModules;


    std::vector<Module*>& theseBarrelModules = selectedModules[barrelPath().barrel() + "_" + barrelPath().layer()];

    if ((aModule.stereoDistance()<=0) || (aModule.triggerWindow()==0)) continue;
    if (aModule.readoutType() != Module::Pt) {
      std::cerr << "WARNING: a non-pT module has a non-zero trigger window!"  << std::endl; // TODO: put this in the logger as a warning
      continue;
    }

    // Prepare the variables to hold the profiles
    std::map<double, TProfile>& tuningProfiles = myProfileBag.getNamedProfiles(profileBag::TriggerProfileName + myName);
    // Prepare the variables to hold the turn-on curve profiles
    std::map<double, TProfile>& turnonProfiles = myProfileBag.getNamedProfiles(profileBag::TurnOnCurveName + myName);

    //  Profiles
    if (!preparedProfiles[myName]) {
      preparedProfiles[myName] = true;
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


  void doVisit(const EndcapModule& aModule) {
  // If it's an endcap layer, scan over all disk 1 modules

  // Sort the plot by ring (using only disk 1)
    tempSS << "_D";
    tempSS.width(2).fill('0'); 
    tempSS << endcapPath().disk(); 
    myBaseName = aLayer->getContainerName() + tempSS.str();

    // Actually scan the modules
    if ((aModule->getStereoDistance()<=0) || (aModule->getTriggerWindow()==0)) continue;
    if (aModule->getReadoutType() != Module::Pt) {
      std::cerr << "WARNING: a non-pT module has a non-zero trigger window!"  << std::endl; // TODO: put this in the logger as a warning
      continue;
    }

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

  void postVisit() {
      // TODO: put also the limits into a configurable parameter
      // Scan again over the plots I just made in order to find the
      // interesting range, if we have 1 and 2 in the range (TODO: find
      // a better way to find the interesting margins)

      // Run once per possible position in the tracker
    std::vector<std::string> profileNames = myProfileBag.getProfileNames(profileBag::TriggerProfileName);
    for (std::vector<std::string>::const_iterator itName=profileNames.begin(); itName!=profileNames.end(); ++itName) {
      std::map<double, TProfile>& tuningProfiles = myProfileBag.getNamedProfiles(*itName);
      TProfile& lowTuningProfile = tuningProfiles[spacingTuningMomenta.first];
      TProfile& highTuningProfile = tuningProfiles[spacingTuningMomenta.second];
      triggerRangeLowLimit[*itName] = findXThreshold(lowTuningProfile, 1, true);
      triggerRangeHighLimit[*itName] = findXThreshold(highTuningProfile, 90, false);
    }


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
    nbins = l.numModulesPerRod();
  }

  void doVisit(const Disk& d) {
    setupSummaries(summaryPath().cnt);
    nbins = d.numRings();
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
        currentTotalHisto = new TH1D(("totalStubsPerEventHisto" + cntName + any2str(layerIndex)).c_str(), ";Modules;MHz/cm^2", nbins, 0.5, nbins+0.5);
        currentTrueHisto = new TH1D(("trueStubsPerEventHisto" + cntName + any2str(layerIndex)).c_str(), ";Modules;MHz/cm^2", nbins, 0.5, nbins+0.5); 
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

}