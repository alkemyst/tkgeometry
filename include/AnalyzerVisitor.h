#ifndef ANALYZERVISITOR_H
#define ANALYZERVISITOR_H

#include <string>
#include <map>
#include <vector>
#include <utility>

#include <TH1.h>
#include <TH2.h>
#include <TProfile.h>
#include <TGraph.h>
#include <TGraphErrors.h>

#include "Tracker.h"
#include "Barrel.h"
#include "Endcap.h"
#include "Layer.h"
#include "Disk.h"
#include "RodPair.h"
#include "Ring.h"
#include "Module.h"
#include "SimParms.h"

#include "Visitor.h"

#include "ShapeShifter.h"

#include "Bag.h"
#include "SummaryTable.h"

using std::string;
using std::map;
using std::vector;
using std::pair;

class AnalyzerVisitor : public GenericGeometryVisitor {
  struct GeometryPath {
    string tracker, subdet;
    int modref[3];
  };
public:
  struct BarrelPath {
    string &tracker, &barrel;
    int &layer, &ring, &rod;
    BarrelPath(GeometryPath& gp) : tracker(gp.tracker), barrel(gp.subdet), layer(gp.modref[0]), ring(gp.modref[1]), rod(gp.modref[2]) {}
  };
  struct EndcapPath {
    string &tracker, &endcap;
    int &disk, &ring, &blade;
    EndcapPath(GeometryPath& gp) : tracker(gp.tracker), endcap(gp.subdet), disk(gp.modref[0]), ring(gp.modref[1]), blade(gp.modref[2]) {}
  };
  struct SummaryPath {
    string &tracker, &table;
    int &row, &col;
    SummaryPath(GeometryPath& gp) : tracker(gp.tracker), table(gp.subdet), row(gp.modref[0]), col(gp.modref[1]) {}
  };

private:
  GeometryPath geometryPath_;
  union {
    BarrelPath barrelPath_;
    EndcapPath endcapPath_;
   // struct UniformPath { IdentifiableType tracker, cnt, z, rho, phi;          } uniformPath_;
    SummaryPath summaryPath_; // provided for backwards compatibility for summary tables (the phi position is ignored, barrel and endcap have z and rho coords swapped)
  } convertedPath_;

  bool barrelModule_;

protected:
  AnalyzerVisitor() : convertedPath_{BarrelPath(geometryPath_)} {}
  const BarrelPath& barrelPath() const { return convertedPath_.barrelPath_; }
  const EndcapPath& endcapPath() const { return convertedPath_.endcapPath_; }
  const SummaryPath& summaryPath() const { return convertedPath_.summaryPath_; }

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

  virtual void doVisit(const DetectorModule&) {}
  virtual void doVisit(const BarrelModule&) {}
  virtual void doVisit(const EndcapModule&) {}

  virtual void doVisit(const TypedModule&) {}
  virtual void doVisit(const SingleSensorModule&) {}
  virtual void doVisit(const DualSensorModule&) {}
  virtual void doVisit(const PtModule&) {}
  virtual void doVisit(const StereoModule&) {}

  virtual void doVisit(const GeometricModule&) {}
  virtual void doVisit(const RectangularModule&) {}
  virtual void doVisit(const WedgeModule&) {}

  virtual void doVisit(const SimParms&) {}
public:
  void visit(const Tracker& t) override { convertedPath_.summaryPath_.tracker = t.myid(); doVisit(t); }
  void visit(const Barrel& b) override  { convertedPath_.barrelPath_.barrel   = b.myid(); doVisit(b); }
  void visit(const Layer& l) override   { convertedPath_.barrelPath_.layer = str2any<int>(l.myid()); doVisit(l); }
  void visit(const RodPair& r) override { convertedPath_.barrelPath_.rod   = str2any<int>(r.myid()); doVisit(r); } 
  void visit(const Endcap& e) override  { convertedPath_.endcapPath_.endcap= str2any<int>(e.myid()); doVisit(e); }
  void visit(const Disk& d) override    { convertedPath_.endcapPath_.disk  = str2any<int>(d.myid()); doVisit(d); }
  void visit(const Ring& r) override    { convertedPath_.endcapPath_.ring  = str2any<int>(r.myid()); doVisit(r); }
  void visit(const BarrelModule& bm) override { convertedPath_.barrelPath_.ring = str2any<int>(bm.myid()); barrelModule_ = true;  doVisit((Module&)bm); doVisit((DetectorModule&)bm); doVisit(bm); }
  void visit(const EndcapModule& em) override { convertedPath_.endcapPath_.blade= str2any<int>(em.myid()); barrelModule_ = false; doVisit((Module&)em); doVisit((DetectorModule&)em); doVisit(em); }
  void visit(const SingleSensorModule& ssm) override { doVisit((TypedModule&)ssm); doVisit(ssm); }
  void visit(const PtModule& pm) override          { doVisit((TypedModule&)pm); doVisit((DualSensorModule&)pm); doVisit(pm); }
  void visit(const StereoModule& sm) override      { doVisit((TypedModule&)sm); doVisit((DualSensorModule&)sm); doVisit(sm); }
  void visit(const RectangularModule& rm) override { doVisit((GeometricModule&)rm); doVisit(rm); }
  void visit(const WedgeModule& wm) override       { doVisit((GeometricModule&)wm); doVisit(wm); }

  void visit(const SimParms& sp) override { doVisit(sp); }
};



namespace AnalyzerHelpers {

  bool isModuleInEtaSector(const SimParms& simParms, const Tracker& tracker, const DetectorModule& module, int etaSector); 
  bool isModuleInPhiSector(const SimParms& simParms, const DetectorModule& module, int phiSector);

}


class TriggerProcessorBandwidthVisitor : public AnalyzerVisitor {
  typedef std::map<std::pair<int, int>, int> ProcessorConnections;
  typedef std::map<std::pair<int, int>, double> ProcessorInboundBandwidths;
  typedef std::map<std::pair<int, int>, double> ProcessorInboundStubsPerEvent;
  ProcessorConnections processorConnections_;
  ProcessorInboundBandwidths processorInboundBandwidths_;
  ProcessorInboundStubsPerEvent processorInboundStubsPerEvent_;

  map<string, map<pair<int, int>, double>> &triggerDataBandwidths_, triggerFrequenciesPerEvent_;
  SummaryTable &processorConnectionSummary_, &processorInboundBandwidthSummary_, &processorInboundStubPerEventSummary_;
  TH1I& moduleConnectionsDistribution_;

  const Tracker* tracker_;
  const SimParms* simParms_;


  class ModuleConnectionData {
    int phiCpuConnections_, etaCpuConnections_;
  public:
    int phiCpuConnections() const { return phiCpuConnections_; }
    int etaCpuConnections() const { return etaCpuConnections_; }
    int totalCpuConnections() const { return phiCpuConnections_*etaCpuConnections_; }
    void phiCpuConnections(int conn) { phiCpuConnections_ = conn; }
    void etaCpuConnections(int conn) { etaCpuConnections_ = conn; }
    ModuleConnectionData() : phiCpuConnections_(0), etaCpuConnections_(0) {}
  };
  map<const Module*,ModuleConnectionData> moduleConnections_;

  int numProcEta, numProcPhi;

  double inboundBandwidthTotal = 0.;
  int processorConnectionsTotal = 0;
  double inboundStubsPerEventTotal = 0.;
public:

  TriggerProcessorBandwidthVisitor(SummaryTable& processorConnectionSummary, SummaryTable& processorInboundBandwidthSummary, SummaryTable& processorInboundStubPerEventSummary, map<string, map<pair<int, int>, double>>& triggerDataBandwidths, map<string, map<pair<int, int>, double>>& triggerFrequenciesPerEvent, TH1I& moduleConnectionsDistribution) :
      processorConnectionSummary_(processorConnectionSummary),
      processorInboundBandwidthSummary_(processorInboundBandwidthSummary),
      processorInboundStubPerEventSummary_(processorInboundStubPerEventSummary),
      triggerDataBandwidths_(triggerDataBandwidths),
      triggerFrequenciesPerEvent_(triggerFrequenciesPerEvent),
      moduleConnectionsDistribution_(moduleConnectionsDistribution)
  {}

  void preVisit() {
    processorConnectionSummary_.setHeader("Phi", "Eta");
    processorInboundBandwidthSummary_.setHeader("Phi", "Eta");
    processorInboundStubPerEventSummary_.setHeader("Phi", "Eta");

    processorInboundBandwidthSummary_.setPrecision(3);
    processorInboundStubPerEventSummary_.setPrecision(3);

    moduleConnectionsDistribution_.Reset();
    moduleConnectionsDistribution_.SetNameTitle("ModuleConnDist", "Number of connections to trigger processors;Connections;Modules");
    moduleConnectionsDistribution_.SetBins(11, -.5, 10.5);
  }

  void doVisit(const SimParms& sp) {
    simParms_ = &sp;
    numProcEta = sp.numTriggerTowersEta();
    numProcPhi = sp.numTriggerTowersPhi();
  }

  void doVisit(const Tracker& t) { tracker_ = &t; }

  void doVisit(const DetectorModule& m) {
    SummaryPath p = summaryPath();

    int etaConnections = 0, totalConnections = 0;

    for (int i=0; i < numProcEta; i++) {
      if (AnalyzerHelpers::isModuleInEtaSector(*simParms_, *tracker_, m, i)) {
        etaConnections++;
        for (int j=0; j < numProcPhi; j++) {
          if (AnalyzerHelpers::isModuleInPhiSector(*simParms_, m, j)) {
            totalConnections++;

            processorConnections_[std::make_pair(j,i)] += 1;
            processorConnectionSummary_.setCell(j+1, i+1, processorConnections_[std::make_pair(j,i)]);

            processorInboundBandwidths_[std::make_pair(j,i)] += triggerDataBandwidths_[p.table][std::make_pair(p.row, p.col)]; // *2 takes into account negative Z's
            processorInboundBandwidthSummary_.setCell(j+1, i+1, processorInboundBandwidths_[std::make_pair(j,i)]);

            processorInboundStubsPerEvent_[std::make_pair(j,i)] += triggerFrequenciesPerEvent_[p.table][std::make_pair(p.row, p.col)];
            processorInboundStubPerEventSummary_.setCell(j+1, i+1, processorInboundStubsPerEvent_[std::make_pair(j,i)]);

          } 
        }
      }
    }
    moduleConnections_[&m].etaCpuConnections(etaConnections);
    moduleConnections_[&m].phiCpuConnections(totalConnections > 0 ? totalConnections/etaConnections : 0);

    for (const auto& mvp : processorInboundBandwidths_) inboundBandwidthTotal += mvp.second;
    for (const auto& mvp : processorConnections_) processorConnectionsTotal += mvp.second;
    for (const auto& mvp : processorInboundStubsPerEvent_) inboundStubsPerEventTotal += mvp.second;
  }

  void postVisit() {
    processorInboundBandwidthSummary_.setSummaryCell("Total", inboundBandwidthTotal);
    processorConnectionSummary_.setSummaryCell("Total", processorConnectionsTotal);
    processorInboundStubPerEventSummary_.setSummaryCell("Total", inboundStubsPerEventTotal);

    for (auto mvp : moduleConnections_) moduleConnectionsDistribution_.Fill(mvp.second.totalCpuConnections(), 1);
  }
};




class IrradiationPowerVisitor : public AnalyzerVisitor {
  double numInvFemtobarns;
  double operatingTemp;
  double chargeDepletionVoltage;
  double alphaParam;
  double referenceTemp;
  const IrradiationMap* irradiationMap_;
  MultiSummaryTable& irradiatedPowerConsumptionSummaries_;
public:
  IrradiationPowerVisitor(MultiSummaryTable& irradiatedPowerConsumptionSummaries) : irradiatedPowerConsumptionSummaries_(irradiatedPowerConsumptionSummaries) {}

  void preVisit() {
    irradiatedPowerConsumptionSummaries_.clear();   
  }

  void doVisit(const SimParms& sp) {
    numInvFemtobarns = sp.timeIntegratedLumi();
    operatingTemp    = sp.operatingTemp();
    chargeDepletionVoltage = sp.chargeDepletionVoltage();
    alphaParam       = sp.alphaParm();
    referenceTemp    = sp.referenceTemp();
    irradiationMap_  = &sp.irradiationMap();
  }

  void doVisit(const Barrel& b) {
    irradiatedPowerConsumptionSummaries_[summaryPath().table].setHeader("layer", "ring");
    irradiatedPowerConsumptionSummaries_[summaryPath().table].setPrecision(3);        
  }

  void doVisit(const Endcap& e) {
    irradiatedPowerConsumptionSummaries_[summaryPath().table].setHeader("layer", "ring");
    irradiatedPowerConsumptionSummaries_[summaryPath().table].setPrecision(3);        
  }

  void doVisit(const TypedModule& m) {
    XYZVector center = m.center();
    if (center.Z() < 0) return;
    double volume = 0.;
    for (const auto& s : m.sensors()) volume += s.sensorThickness() * m.area() / 1000.0; // volume is in cm^3
    double x  = center.Z()/25;
    double y  = center.Rho()/25;
    double x1 = floor(x);
    double x2 = ceil(x);
    double y1 = floor(y);
    double y2 = ceil(y);
    double irr11 = irradiationMap_->at(std::make_pair(int(x1), int(y1))); 
    double irr21 = irradiationMap_->at(std::make_pair(int(x2), int(y1)));
    double irr12 = irradiationMap_->at(std::make_pair(int(x1), int(y2)));
    double irr22 = irradiationMap_->at(std::make_pair(int(x2), int(y2)));
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
  TH1D &chanHitDistribution_, &bandwidthDistribution_, &bandwidthDistributionSparsified_;

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

  void doVisit(const SimParms& sp) {
    nMB_ = sp.numMinBiasEvents();
  }

  void doVisit(const DetectorModule& m) { stripOccupancyPerEvent_ = m.phiAperture() * (log(tan(m.maxTheta()/2.)) - log(tan(m.minTheta()/2.))); }
  void doVisit(const BarrelModule& m) { 
    double rho = m.center().Rho()/10.;
    double theta = m.center().Theta();
    double myOccupancyBarrel=(1.63e-4)+(2.56e-4)*rho-(1.92e-6)*rho*rho;
    double factor = fabs(sin(theta))*2; // 2 is a magic adjustment factor
    stripOccupancyPerEvent_ *= myOccupancyBarrel / factor / (90/1e3); 
  }
  void doVisit(const EndcapModule& m) {
    double rho = m.center().Rho()/10.;
    double theta = m.center().Theta();
    double myOccupancyEndcap=(-6.20e-5)+(1.75e-4)*rho-(1.08e-6)*rho*rho+(1.50e-5)*(z);
    double factor=fabs(cos(theta))*2; // 2 is a magic adjustment factor
    stripOccupancyPerEvent_ *= myOccupancyBarrel / factor / (90/1e3);
  }
  void doVisit(const TypedModule& m) { stripOccupancyPerEvent_ /= std::min_element(m.sensors.begin(), m.sensors.end(), [](const Sensor& s1, const Sensor& s2) { return s1.numSegments() < s2.numSegments(); })->numSegments(); }
  void doVisit(const DualSensorModule& m) { stripOccupancyPerEvent_ /= m.sensors[0].numStripsAcross(); }
  void doVisit(const WedgeModule& m) { stripOccupancyPerEvent_ *= (m.minWidth() + m.maxWidth())/2; }
  void doVisit(const RectangularModule& m) { stripOccupancyPerEvent_ *= m.width(); }


  void doVisit(const BarrelModule& m) { ss_ << m; }
  void doVisit(const EndcapModule& m) { ss_ << m; }
  

  void doVisit(const ShapeShifter<BarrelModule, DualSensorModule, RectangularModule>& m) {
    auto bm = m.as<BarrelModule>();
    auto dsm = m.as<DualSensorModule>();
    auto rm = m.as<RectangularModule>();

    double rho = rm.center().Rho()/10.;
    double theta = rm.center().Theta();
    double myOccupancyBarrel=(1.63e-4)+(2.56e-4)*rho-(1.92e-6)*rho*rho;
    double factor = fabs(sin(theta))*2; // 2 is a magic adjustment factor
    
    double stripOccupancyPerEvent = rm.phiAperture() * (log(tan(rm.maxTheta()/2.)) - log(tan(rm.minTheta()/2.))) * myOccupancyBarrel / factor / (90/1e3);
  }

  void doVisit(const ShapeShifter<EndcapModule, DualSensorModule, RectangularModule>& m) {


  }

  void doVisit(const ShapeShifter<


  void doVisit(const DualSensorModule& m) {
    double hitChannels;
    // Clear and reset the histograms

    int nChips;

  //  for (layIt=layerSet.begin(); layIt!=layerSet.end(); layIt++) {
  //   aLay = (*layIt)->getModuleVector();
  //   for (modIt=aLay->begin(); modIt!=aLay->end(); modIt++) {
    if (m.sensors().back().type() == SensorType::Strip) {
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

 //   savingGeometryV.push_back(chanHitDistribution_);
 //   savingGeometryV.push_back(bandwidthDistribution_);
 //   savingGeometryV.push_back(bandwidthDistributionSparsified_);
  }
};


class TriggerDistanceTuningPlotsVisitor : public AnalyzerVisitor {
  typedef std::vector<const PtModule*> ModuleVector;
  std::map<std::string, ModuleVector> selectedModules_;
  std::set<double> foundSpacing_;
  const std::vector<double>& triggerMomenta_;
  const unsigned int nWindows_ = 5;

  profileBag& myProfileBag_;
  TH1D &optimalSpacingDistribution_, &optimalSpacingDistributionAW_;
  std::map<std::string, bool> preparedProfiles_;
  std::map<std::string, bool> preparedTurnOn_;

  std::map<std::string, double>& triggerRangeLowLimit_;
  std::map<std::string, double>& triggerRangeHighLimit_;
  
  TH1D& spacingTuningFrame_;
  std::map<int, TGraphErrors>& spacingTuningGraphs_; // TODO: find a way to communicate the limits, not their plots!
  std::map<int, TGraphErrors>& spacingTuningGraphsBad_; // TODO: find a way to communicate the limits, not their plots!

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

    std::string myName = any2str(barrelPath().barrel) + "_" + any2str(barrelPath().layer);

    std::vector<const PtModule*>& theseBarrelModules = selectedModules_[myName];

    if (aModule.dsDistance() <= 0 || aModule.triggerWindow() == 0) return;

    // Prepare the variables to hold the profiles
    std::map<double, TProfile>& tuningProfiles = myProfileBag_.getNamedProfiles(profileBag::TriggerProfileName + myName);
    // Prepare the variables to hold the turn-on curve profiles
    std::map<double, TProfile>& turnonProfiles = myProfileBag_.getNamedProfiles(profileBag::TurnOnCurveName + myName);

    //  Profiles
    if (!preparedProfiles_[myName]) {
      preparedProfiles_[myName] = true;
      for (std::vector<double>::const_iterator it=triggerMomenta_.begin(); it!=triggerMomenta_.end(); ++it) {
        std::ostringstream tempSS; tempSS << "Trigger efficiency for " << myName.c_str() << ";Sensor spacing [mm];Efficiency [%]";
        tuningProfiles[*it].SetTitle(tempSS.str().c_str());
        tempSS.str(""); tempSS << "TrigEff" << myName.c_str() << "_" << (*it) << "GeV";
        tuningProfiles[*it].SetName(tempSS.str().c_str());
        tuningProfiles[*it].SetBins(100, 0.5, 6); // TODO: these numbers should go into some kind of const
      }     
    }

    // Turn-on curve
    if (!preparedTurnOn_[myName]) {
      preparedTurnOn_[myName] = true;
      for (unsigned int iWindow=0; iWindow<nWindows_; ++iWindow) {
        double windowSize=iWindow*2+1;
        std::ostringstream tempSS; tempSS << "Trigger efficiency for " << myName.c_str() << ";p_{T} [GeV/c];Efficiency [%]";
        turnonProfiles[windowSize].SetTitle(tempSS.str().c_str());
        tempSS.str(""); tempSS << "TurnOn" << myName.c_str() << "_window" << int(windowSize);
        turnonProfiles[windowSize].SetName(tempSS.str().c_str());
        turnonProfiles[windowSize].SetBins(100, 0.5, 10); // TODO: these numbers should go into some kind of const
      }     
    }


    XYZVector center = aModule.center();
    if ((center.Z()<0) || (center.Phi()<0) || (center.Phi()>M_PI/2)) return;
    theseBarrelModules.push_back(&aModule);

    // Fill the tuning profiles for the windows actually set
    for (double dist=0.5; dist<=6; dist+=0.02) {
      for (std::vector<double>::const_iterator it=triggerMomenta_.begin(); it!=triggerMomenta_.end(); ++it) {
        double myPt = (*it);
        double myValue = 100 * aModule.triggerProbability(myPt, dist);
        if ((myValue>=0) && (myValue<=100))
          tuningProfiles[myPt].Fill(dist, myValue);
      }
    }

    // Fill the turnon curves profiles for the distance actually set
    for (double myPt=0.5; myPt<=10; myPt+=0.02) {
      for (unsigned int iWindow=0; iWindow<nWindows_; ++iWindow) {
        double windowSize=iWindow*2+1;
        double distance = aModule.dsDistance();
        double myValue = 100 * aModule.triggerProbability(myPt, distance, int(windowSize));
        if ((myValue>=0) && (myValue<=100))
          turnonProfiles[windowSize].Fill(myPt, myValue);
      }
    }
  }


  void doVisitEndcapPtModule(const PtModule& aModule) {



    string myBaseName = any2str(endcapPath().ring) + "_D" + any2str(endcapPath().disk);

    if ((aModule.dsDistance()<=0) || (aModule.triggerWindow()==0)) return;

    XYZVector center = aModule.center();
    // std::cerr << myBaseName << " z=" << center.Z() << ", Phi=" << center.Phi() << ", Rho=" << center.Rho() << std::endl; // debug
    if ((center.Z()<0) || (center.Phi()<0) || (center.Phi()>M_PI/2)) return;


    string myName = myBaseName + "R" + any2str(endcapPath().ring);
    ModuleVector& theseEndcapModules = selectedModules_[myName];
    theseEndcapModules.push_back(&aModule);

              // Prepare the variables to hold the profiles
    std::map<double, TProfile>& tuningProfiles = myProfileBag_.getNamedProfiles(profileBag::TriggerProfileName + myName);
    // Prepare the variables to hold the turn-on curve profiles
    std::map<double, TProfile>& turnonProfiles = myProfileBag_.getNamedProfiles(profileBag::TurnOnCurveName + myName);

    // Tuning profile
    if (!preparedProfiles_[myName]) {
      preparedProfiles_[myName] = true;
      for (std::vector<double>::const_iterator it=triggerMomenta_.begin(); it!=triggerMomenta_.end(); ++it) {
        std::ostringstream tempSS; tempSS << "Trigger efficiency for " << myName.c_str() << " GeV;Sensor spacing [mm];Efficiency [%]";
        tuningProfiles[*it].SetTitle(tempSS.str().c_str());
        tempSS.str(""); tempSS << "TrigEff" << myName.c_str() << "_" << (*it);
        tuningProfiles[*it].SetName(tempSS.str().c_str());
        tuningProfiles[*it].SetBins(100, 0.5, 6); // TODO: these numbers should go into some kind of const
      }
    }

    // Turn-on curve
    if (!preparedTurnOn_[myName]) {
      preparedTurnOn_[myName] = true;
      for (unsigned int iWindow=0; iWindow<nWindows_; ++iWindow) {
        double windowSize=iWindow*2+1;
        std::ostringstream tempSS; tempSS << "Trigger efficiency for " << myName.c_str() << ";p_{T} [GeV/c];Efficiency [%]";
        turnonProfiles[windowSize].SetTitle(tempSS.str().c_str());
        tempSS.str(""); tempSS << "TurnOn" << myName.c_str() << "_window" << windowSize;
        turnonProfiles[windowSize].SetName(tempSS.str().c_str());
        turnonProfiles[windowSize].SetBins(100, 0.5, 10); // TODO: these numbers should go into some kind of const
      }     
    }

    // Fill the tuning profiles for the windows actually set
    for (double dist=0.5; dist<=6; dist+=0.02) {
      for (std::vector<double>::const_iterator it=triggerMomenta_.begin(); it!=triggerMomenta_.end(); ++it) {
        double myPt = (*it);
        double myValue = 100 * aModule.triggerProbability(myPt, dist);
        if ((myValue>=0) && (myValue<=100))
          tuningProfiles[myPt].Fill(dist, myValue);
      }
    }

    // Fill the turnon curves profiles for the distance actually set
    for (double myPt=0.5; myPt<=10; myPt+=0.02) {
      for (unsigned int iWindow=0; iWindow<nWindows_; ++iWindow) {
        double windowSize=iWindow*2+1;
        double distance = aModule.dsDistance();
        double myValue = 100 * aModule.triggerProbability(myPt, distance, int(windowSize));
        if ((myValue>=0) && (myValue<=100))
          turnonProfiles[windowSize].Fill(myPt, myValue);
      }
    }
  }

public:
  TriggerDistanceTuningPlotsVisitor(TH1D& optimalSpacingDistribution,
                                    TH1D& optimalSpacingDistributionAW, 
                                    profileBag& myProfileBag, 
                                    std::pair<std::map<string, double>, std::map<string, double>>& triggerRangeLimits, 
                                    TH1D& spacingTuningFrame,
                                    std::map<int, TGraphErrors>& spacingTuningGraphs,
                                    std::map<int, TGraphErrors>& spacingTuningGraphsBad,
                                    const std::vector<double>& triggerMomenta) :
    optimalSpacingDistribution_(optimalSpacingDistribution), 
    optimalSpacingDistributionAW_(optimalSpacingDistributionAW_), 
    myProfileBag_(myProfileBag), 
    triggerRangeLowLimit_(triggerRangeLimits.first), 
    triggerRangeHighLimit_(triggerRangeLimits.second),
    spacingTuningFrame_(spacingTuningFrame),
    spacingTuningGraphs_(spacingTuningGraphs),
    spacingTuningGraphsBad_(spacingTuningGraphsBad),
    triggerMomenta_(triggerMomenta) 
  {

    /************************************************/

    // TODO: clear only the relevant ones?
    myProfileBag_.clearTriggerNamedProfiles();
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

  void doVisit(const PtModule& aModule) {
    if (aModule.dsDistance() > 0.) foundSpacing_.insert(aModule.dsDistance());
    if (isBarrelModule()) doVisitBarrelPtModule(aModule);
    else doVisitEndcapPtModule(aModule);
  }



  void postVisit() {
      // TODO: put also the limits into a configurable parameter
      // Scan again over the plots I just made in order to find the
      // interesting range, if we have 1 and 2 in the range (TODO: find
      // a better way to find the interesting margins)

      // Run once per possible position in the tracker
      
    std::vector<double> spacingOptions(foundSpacing_.begin(), foundSpacing_.end());
    foundSpacing_.clear();    
     
    unsigned int nSpacingOptions = spacingOptions.size();             // TODO: keep this here!!

    std::pair<double, double> spacingTuningMomenta;
    spacingTuningMomenta.first = 1.;
    spacingTuningMomenta.second = 2.5;

    std::vector<std::string> profileNames = myProfileBag_.getProfileNames(profileBag::TriggerProfileName);
    for (std::vector<std::string>::const_iterator itName=profileNames.begin(); itName!=profileNames.end(); ++itName) {
      std::map<double, TProfile>& tuningProfiles = myProfileBag_.getNamedProfiles(*itName);
      TProfile& lowTuningProfile = tuningProfiles[spacingTuningMomenta.first];
      TProfile& highTuningProfile = tuningProfiles[spacingTuningMomenta.second];
      triggerRangeLowLimit_[*itName] = findXThreshold(lowTuningProfile, 1, true);
      triggerRangeHighLimit_[*itName] = findXThreshold(highTuningProfile, 90, false);
    }


    // Now loop over the selected modules and build the curves for the
    // hi-lo thingy (sensor spacing tuning)
    int windowSize;
    XYZVector center;
    double myPt;
    TProfile tempProfileLow("tempProfileLow", "", 100, 0.5, 6); // TODO: these numbers should go into some kind of const
    TProfile tempProfileHigh("tempProfileHigh", "", 100, 0.5, 6); // TODO: these numbers should go into some kind of const

    // TODO: IMPORTANT!!!!!! clear the spacing tuning graphs and frame here
    spacingTuningFrame_.SetBins(selectedModules_.size(), 0, selectedModules_.size());
    spacingTuningFrame_.SetYTitle("Optimal distance range [mm]");
    spacingTuningFrame_.SetMinimum(0);
    spacingTuningFrame_.SetMaximum(6);
    TAxis* xAxis = spacingTuningFrame_.GetXaxis();

    int iType=0;
    std::map<double, bool> availableThinkness;
    // Loop over the selected module types
    for(std::map<std::string, ModuleVector>::iterator itTypes = selectedModules_.begin();
        itTypes!=selectedModules_.end(); ++itTypes) {
      const std::string& myName = itTypes->first;
      const ModuleVector& myModules = itTypes->second;
      xAxis->SetBinLabel(iType+1, myName.c_str());

      // Loop over the possible search windows
      for (unsigned int iWindow = 0; iWindow<nWindows_; ++iWindow) {
        windowSize = 1 + iWindow * 2;
        // Loop over the modules of type myName
        for (ModuleVector::const_iterator itModule = myModules.begin(); itModule!=myModules.end(); ++itModule) {
          const PtModule* aModule = (*itModule);
          // Loop over the possible distances
          double minDistBelow = 0.;
          availableThinkness[aModule->dsDistance()] = true;
          for (double dist=0.5; dist<=6; dist+=0.02) { // TODO: constant here
            // First with the high momentum
            myPt = (spacingTuningMomenta.second);
            double myValue = 100 * aModule->triggerProbability(myPt, dist, windowSize);
            if ((myValue>=0)&&(myValue<=100))
              tempProfileHigh.Fill(dist, myValue);
            // Then with low momentum
            myPt = (spacingTuningMomenta.first);
            myValue = 100 * aModule->triggerProbability(myPt, dist, windowSize);
            if ((myValue>=0)&&(myValue<=100))
              tempProfileLow.Fill(dist, myValue);
            if (myValue>1) minDistBelow = dist;
          }
          if (minDistBelow>=0) {
            if (windowSize==5) optimalSpacingDistribution_.Fill(minDistBelow);
            if (windowSize==aModule->triggerWindow()) optimalSpacingDistributionAW_.Fill(minDistBelow);
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
          aModule->optimalSpacing(windowSize, minDistBelow);
        }
        // Find the "high" and "low" points
        double lowEdge = findXThreshold(tempProfileLow, 1, true);
        double highEdge = findXThreshold(tempProfileHigh, 90, false);
        // std::cerr << myName << ": " << lowEdge << " -> " << highEdge << std::endl; // debug
        double centerX; double sizeX;
        centerX = iType+(double(iWindow)+0.5)/(double(nWindows_));
        sizeX = 1./ (double(nWindows_)) * 0.8; // 80% of available space, so that they will not touch
        if (lowEdge<highEdge) {
          spacingTuningGraphs_[iWindow].SetPoint(iType, centerX, (highEdge+lowEdge)/2.);
          spacingTuningGraphs_[iWindow].SetPointError(iType, sizeX/2., (highEdge-lowEdge)/2.);
        } else {
          spacingTuningGraphsBad_[iWindow].SetPoint(iType, centerX, (highEdge+lowEdge)/2.);
          spacingTuningGraphsBad_[iWindow].SetPointError(iType, sizeX/2., (highEdge-lowEdge)/2.);
        }
        tempProfileLow.Reset();
        tempProfileHigh.Reset();
      }
      iType++;
    }

    // TODO: properly reset this!
    TGraphErrors& antani = spacingTuningGraphs_[-1];
    int iPoints=0;
    for (std::map<double, bool>::iterator it = availableThinkness.begin(); it!= availableThinkness.end(); ++it) {
      iPoints++;
      antani.SetPoint(iPoints, selectedModules_.size()/2., it->first);
      antani.SetPointError(iPoints, selectedModules_.size()/2., 0);
      iPoints++;
    }
  }
};




class TriggerFrequencyVisitor : public AnalyzerVisitor {
  typedef std::map<std::pair<std::string, int>, TH1D*> StubRateHistos;

  std::map<std::string, std::map<std::pair<int,int>, int> >   triggerFrequencyCounts_;
  std::map<std::string, std::map<std::pair<int,int>, double> >  triggerFrequencyAverageTrue_, triggerFrequencyInterestingParticleTrue_, triggerFrequencyAverageFake_, triggerDataBandwidths_, triggerFrequenciesPerEvent_; // trigger frequency by module in Z and R, averaged over Phi
  StubRateHistos totalStubRateHistos_, trueStubRateHistos_;

  MultiSummaryTable &triggerFrequencyTrueSummaries_, &triggerFrequencyFakeSummaries_, &triggerFrequencyInterestingSummaries_, &triggerRateSummaries_, &triggerEfficiencySummaries_,&triggerPuritySummaries_, &triggerDataBandwidthSummaries_;

  int nbins_;
  double bunchSpacingNs_, nMB_;

  void setupSummaries(const string& cntName) {
    triggerFrequencyTrueSummaries_[cntName].setHeader("Layer", "Ring");
    triggerFrequencyFakeSummaries_[cntName].setHeader("Layer", "Ring");
    triggerFrequencyInterestingSummaries_[cntName].setHeader("Layer", "Ring");
    triggerRateSummaries_[cntName].setHeader("Layer", "Ring");
    triggerEfficiencySummaries_[cntName].setHeader("Layer", "Ring");
    triggerPuritySummaries_[cntName].setHeader("Layer", "Ring");
    triggerDataBandwidthSummaries_[cntName].setHeader("Layer", "Ring");
    triggerFrequencyTrueSummaries_[cntName].setPrecision(3);
    triggerFrequencyFakeSummaries_[cntName].setPrecision(3);
    triggerFrequencyInterestingSummaries_[cntName].setPrecision(3);
    triggerRateSummaries_[cntName].setPrecision(3);
    triggerEfficiencySummaries_[cntName].setPrecision(3);
    triggerPuritySummaries_[cntName].setPrecision(3);
    triggerDataBandwidthSummaries_[cntName].setPrecision(3);
  }
public:
  TriggerFrequencyVisitor(MultiSummaryTable& triggerFrequencyTrueSummaries, 
                          MultiSummaryTable& triggerFrequencyFakeSummaries, 
                          MultiSummaryTable& triggerFrequencyInterestingSummaries, 
                          MultiSummaryTable& triggerRateSummaries, 
                          MultiSummaryTable& triggerEfficiencySummaries,
                          MultiSummaryTable& triggerPuritySummaries,
                          MultiSummaryTable& triggerDataBandwidthSummaries) :
    triggerFrequencyTrueSummaries_(triggerFrequencyTrueSummaries),
    triggerFrequencyFakeSummaries_(triggerFrequencyFakeSummaries),
    triggerFrequencyInterestingSummaries_(triggerFrequencyInterestingSummaries),
    triggerRateSummaries_(triggerRateSummaries),
    triggerEfficiencySummaries_(triggerEfficiencySummaries),
    triggerPuritySummaries_(triggerPuritySummaries),
    triggerDataBandwidthSummaries_(triggerDataBandwidthSummaries)
  {
    triggerFrequencyTrueSummaries_.clear();
    triggerFrequencyFakeSummaries_.clear();
    triggerRateSummaries_.clear();
    triggerPuritySummaries_.clear();
    triggerDataBandwidthSummaries_.clear();
    triggerDataBandwidths_.clear();
  }

  void doVisit(const SimParms& sp) {
    bunchSpacingNs_ = sp.bunchSpacingNs();
    nMB_ = sp.numMinBiasEvents();
  }

  void doVisit(const Layer& l) {
    setupSummaries(summaryPath().table);
    nbins_ = l.numModules(); // numModules() returns the num modules per rod -- CUIDADO misleading?
  }

  void doVisit(const Disk& d) {
    setupSummaries(summaryPath().table);
    nbins_ = d.numRings();
  }

  void doVisit(const PtModule& module) {

    XYZVector center = module.center();
    if ((center.Z()<0) || (center.Phi()<0) || (center.Phi()>M_PI/2) || (module.dsDistance()==0.0)) return;

    TH1D* currentTotalHisto;
    TH1D* currentTrueHisto;

    string table = summaryPath().table;
    int row = summaryPath().row;
    int col = summaryPath().col;

    if (totalStubRateHistos_.count(std::make_pair(table, row)) == 0) {
      currentTotalHisto = new TH1D(("totalStubsPerEventHisto" + table + any2str(row)).c_str(), ";Modules;MHz/cm^2", nbins_, 0.5, nbins_+0.5);
      currentTrueHisto = new TH1D(("trueStubsPerEventHisto" + table + any2str(row)).c_str(), ";Modules;MHz/cm^2", nbins_, 0.5, nbins_+0.5); 
      totalStubRateHistos_[std::make_pair(table, row)] = currentTotalHisto; 
      trueStubRateHistos_[std::make_pair(table, row)] = currentTrueHisto; 
    } else {
      currentTotalHisto = totalStubRateHistos_[std::make_pair(table, row)]; 
      currentTrueHisto = trueStubRateHistos_[std::make_pair(table, row)]; 
    }


    int curCnt = triggerFrequencyCounts_[table][std::make_pair(row, col)]++;
    double curAvgTrue = triggerFrequencyAverageTrue_[table][std::make_pair(row, col)];
    double curAvgInteresting = triggerFrequencyInterestingParticleTrue_[table][std::make_pair(row, col)];
    double curAvgFake = triggerFrequencyAverageFake_[table][std::make_pair(row, col)];

    //curAvgTrue  = curAvgTrue + (module->getTriggerFrequencyTruePerEvent()*tracker.getNMB() - curAvgTrue)/(curCnt+1);
    //curAvgFake  = curAvgFake + (module->getTriggerFrequencyFakePerEvent()*pow(tracker.getNMB(),2) - curAvgFake)/(curCnt+1); // triggerFrequencyFake scales with the square of Nmb!

    // TODO! Important <- make this interestingPt cut configurable
    const double interestingPt = 2;
    curAvgTrue  = curAvgTrue + (module.triggerFrequencyTruePerEventAbove(interestingPt)*nMB_ - curAvgTrue)/(curCnt+1);
    curAvgInteresting += (module.particleFrequencyPerEventAbove(interestingPt)*nMB_ - curAvgInteresting)/(curCnt+1);
    curAvgFake  = curAvgFake + ((module.triggerFrequencyFakePerEvent()*nMB_ + module.triggerFrequencyTruePerEventBelow(interestingPt))*nMB_ - curAvgFake)/(curCnt+1); // triggerFrequencyFake scales with the square of Nmb!

    double curAvgTotal = curAvgTrue + curAvgFake;

    triggerFrequencyAverageTrue_[table][std::make_pair(row, col)] = curAvgTrue;            
    triggerFrequencyInterestingParticleTrue_[table][std::make_pair(row, col)] = curAvgInteresting;    
    triggerFrequencyAverageFake_[table][std::make_pair(row, col)] = curAvgFake;    

    int triggerDataHeaderBits  = module.numTriggerDataHeaderBits();
    int triggerDataPayloadBits = module.numTriggerDataPayloadBits();
    double triggerDataBandwidth = (triggerDataHeaderBits + curAvgTotal*triggerDataPayloadBits) / (bunchSpacingNs_); // GIGABIT/second
    triggerDataBandwidths_[table][std::make_pair(row, col)] = triggerDataBandwidth;
    triggerFrequenciesPerEvent_[table][std::make_pair(row, col)] = curAvgTotal;


    //                currentTotalGraph->SetPoint(module->getRing()-1, module->getRing(), curAvgTotal*(1000/tracker.getBunchSpacingNs())*(100/module->getArea()));
    //                currentTrueGraph->SetPoint(module->getRing()-1, module->getRing(), curAvgTrue*(1000/tracker.getBunchSpacingNs())*(100/module->getArea()));

    currentTotalHisto->SetBinContent(col, curAvgTotal*(1000/bunchSpacingNs_)*(100/module.area()));
    currentTrueHisto->SetBinContent(col, curAvgTrue*(1000/bunchSpacingNs_)*(100/module.area()));

    triggerFrequencyTrueSummaries_[table].setCell(row, col, curAvgTrue);
    triggerFrequencyInterestingSummaries_[table].setCell(row, col, curAvgInteresting);
    triggerFrequencyFakeSummaries_[table].setCell(row, col, curAvgFake);
    triggerRateSummaries_[table].setCell(row, col, curAvgTotal);             
    triggerEfficiencySummaries_[table].setCell(row, col, curAvgTrue/curAvgInteresting);                
    triggerPuritySummaries_[table].setCell(row, col, curAvgTrue/(curAvgTrue+curAvgFake));                
    triggerDataBandwidthSummaries_[table].setCell(row, col, triggerDataBandwidth);

  }

};


namespace AnalyzerHelpers {

  void drawModuleOnMap(const Module& m, double val, TH2D& map, TH2D& counter);
  void drawModuleOnMap(const Module& m, double val, TH2D& map);

}



class TriggerEfficiencyMapVisitor : public AnalyzerVisitor {
  double myPt_;
  TH2D& myMap_;
  TH2D* counter_;
public:
  TriggerEfficiencyMapVisitor(TH2D& map, double pt) : myMap_(map), myPt_(pt) { counter_ = (TH2D*)map.Clone(); }

  void doVisit(const PtModule& aModule) {
    double myValue = aModule.triggerProbability(myPt_);
    if (myValue>=0) AnalyzerHelpers::drawModuleOnMap(aModule, myValue, myMap_, *counter_);
  }

  void postVisit() {
    for (int i=1; i<=myMap_.GetNbinsX(); ++i)
      for (int j=1; j<=myMap_.GetNbinsY(); ++j)
        if (counter_->GetBinContent(i,j)!=0)
          myMap_.SetBinContent(i,j, myMap_.GetBinContent(i,j) / counter_->GetBinContent(i,j));
    // ... and get rid of the counter
  }

  ~TriggerEfficiencyMapVisitor() { delete counter_; }
};





class PtThresholdMapVisitor : public AnalyzerVisitor {
  double myPt_;
  TH2D& myMap_;
  TH2D* counter_;
public:
  PtThresholdMapVisitor(TH2D& map, double pt) : myMap_(map), myPt_(pt) { counter_ = (TH2D*)map.Clone(); }

  void doVisit(const PtModule& aModule) {
    double myValue = aModule.ptThreshold(myPt_);
    if (myValue >= 0) AnalyzerHelpers::drawModuleOnMap(aModule, myValue, myMap_, *counter_);
  }

  void postVisit() {
    for (int i=1; i<=myMap_.GetNbinsX(); ++i)
      for (int j=1; j<=myMap_.GetNbinsY(); ++j)
        if (counter_->GetBinContent(i,j)!=0)
          myMap_.SetBinContent(i,j, myMap_.GetBinContent(i,j) / counter_->GetBinContent(i,j));
    // ... and get rid of the counter
  }

  ~PtThresholdMapVisitor() { delete counter_; }
};





class SpacingCutVisitor : public AnalyzerVisitor {
  TH2D& thicknessMap_;
  TH2D& windowMap_;
  TH2D& suggestedSpacingMap_;
  TH2D& suggestedSpacingMapAW_;
  TH2D& nominalCutMap_;
  TH2D *counter_, *counterSpacing_, *counterSpacingAW_;
public:
  SpacingCutVisitor(TH2D& thicknessMap, TH2D& windowMap, TH2D& suggestedSpacingMap, TH2D& suggestedSpacingMapAW, TH2D& nominalCutMap) : 
      thicknessMap_(thicknessMap), windowMap_(windowMap), suggestedSpacingMap_(suggestedSpacingMap), suggestedSpacingMapAW_(suggestedSpacingMapAW), nominalCutMap_(nominalCutMap) {
        counter_ = (TH2D*)thicknessMap.Clone();
        counterSpacing_ = (TH2D*)suggestedSpacingMap.Clone();
        counterSpacingAW_ = (TH2D*)suggestedSpacingMapAW.Clone();
  }

  void doVisit(const PtModule& aModule) {
    double myThickness = aModule.thickness();
    double myWindow = aModule.triggerWindow();
    //myWindowmm = myWindow * (aModule->getLowPitch() + aModule->getHighPitch())/2.;
    double mySuggestedSpacing = aModule.optimalSpacingWithTriggerWindow(5); // TODO: put this 5 in a configuration of some sort
    double mySuggestedSpacingAW = aModule.optimalSpacingWithTriggerWindow(aModule.triggerWindow());
    double nominalCut = aModule.ptCut();

    AnalyzerHelpers::drawModuleOnMap(aModule, myThickness, thicknessMap_);
    AnalyzerHelpers::drawModuleOnMap(aModule, myWindow, windowMap_);
    AnalyzerHelpers::drawModuleOnMap(aModule, nominalCut, nominalCutMap_);
    if (mySuggestedSpacing != 0) AnalyzerHelpers::drawModuleOnMap(aModule, mySuggestedSpacing, suggestedSpacingMap_, *counterSpacing_);
    if (mySuggestedSpacingAW != 0) AnalyzerHelpers::drawModuleOnMap(aModule, mySuggestedSpacingAW, suggestedSpacingMapAW_, *counterSpacingAW_);

  }

  void postVisit() {
    for (int i=1; i<=thicknessMap_.GetNbinsX(); ++i) {
      for (int j=1; j<=thicknessMap_.GetNbinsY(); ++j) {
        if (counter_->GetBinContent(i,j)!=0) {
          thicknessMap_.SetBinContent(i,j, thicknessMap_.GetBinContent(i,j) / counter_->GetBinContent(i,j));
          windowMap_.SetBinContent(i,j, windowMap_.GetBinContent(i,j) / counter_->GetBinContent(i,j));
          nominalCutMap_.SetBinContent(i,j, nominalCutMap_.GetBinContent(i,j) / counter_->GetBinContent(i,j));
          if ((suggestedSpacingMap_.GetBinContent(i,j)/counterSpacing_->GetBinContent(i,j))>50) {
            std::cout << "debug: for bin " << i << ", " << j << " suggestedSpacing is " << suggestedSpacingMap_.GetBinContent(i,j)
              << " and counter is " << counterSpacing_->GetBinContent(i,j) << std::endl;
          }
          suggestedSpacingMap_.SetBinContent(i,j, suggestedSpacingMap_.GetBinContent(i,j) / counterSpacing_->GetBinContent(i,j));
          suggestedSpacingMapAW_.SetBinContent(i,j, suggestedSpacingMapAW_.GetBinContent(i,j) / counterSpacingAW_->GetBinContent(i,j));
        }
      }
    }
  }

  ~SpacingCutVisitor() {
    delete counter_;
    delete counterSpacing_;
    delete counterSpacingAW_;
  }

};


class IrradiatedPowerMapVisitor : public AnalyzerVisitor {
  TH2D &irradiatedPowerConsumptionMap_, &totalPowerConsumptionMap_;
  TH2D *counter_;
public:
  IrradiatedPowerMapVisitor(TH2D& irradiatedPowerConsumptionMap, TH2D& totalPowerConsumptionMap) : irradiatedPowerConsumptionMap_(irradiatedPowerConsumptionMap), totalPowerConsumptionMap_(totalPowerConsumptionMap) {
    counter_ = (TH2D*)irradiatedPowerConsumptionMap_.Clone();
  }
  void visit(const TypedModule& aModule) {
    if ((aModule.center().Z()<0) || (aModule.center().Phi()<0) || (aModule.center().Phi()>M_PI/2)) return;
    double myPower = aModule.sensorPowerConsumptionAfterIrradiation();
    double myPowerChip = aModule.chipPowerConsumption();

    AnalyzerHelpers::drawModuleOnMap(aModule, myPower, irradiatedPowerConsumptionMap_, *counter_);
    AnalyzerHelpers::drawModuleOnMap(aModule, myPower+myPowerChip, totalPowerConsumptionMap_); // only the first time counter is updated, but in postVisit both maps are averaged bin for bin over the counter value  
  }

  void postVisit() {
    for (int i=1; i<=irradiatedPowerConsumptionMap_.GetNbinsX(); ++i) {
      for (int j=1; j<=irradiatedPowerConsumptionMap_.GetNbinsY(); ++j) {
        if (counter_->GetBinContent(i,j)!=0) {
          irradiatedPowerConsumptionMap_.SetBinContent(i,j, irradiatedPowerConsumptionMap_.GetBinContent(i,j) / counter_->GetBinContent(i,j));
          totalPowerConsumptionMap_.SetBinContent(i,j, totalPowerConsumptionMap_.GetBinContent(i,j) / counter_->GetBinContent(i,j));
        }
      }
    }
  }

  ~IrradiatedPowerMapVisitor() { delete counter_; }

};



#endif
