#ifndef TRIGGERPROCESSORBANDWIDTH_H
#define TRIGGERPROCESSORBANDWIDTH_H

#include <string>
#include <map>
#include <vector>
#include <utility>

#include "TH1.h"
#include "Math/Point2D.h"
#include "TRandom3.h"
#include "Math/Functor.h"
#include "Math/BrentMinimizer1D.h"

#include "Tracker.h"
#include "SimParms.h"

#include "Visitor.h"
#include "SummaryTable.h"

using std::string;
using std::map;
using std::vector;
using std::pair;

namespace AnalyzerHelpers {
  struct Point { double x, y; };
  struct Circle { double x0, y0, r; };
  std::pair<Circle, Circle> findCirclesTwoPoints(const Point& p1, const Point& p2, double r);
  bool isPointInCircle(const Point& p, const Circle& c);
  bool areClockwise(const Point& p1, const Point& p2);

  double calculatePetalAreaMC(const Tracker& tracker, const SimParms& simParms, double crossoverR);
  double calculatePetalAreaModules(const Tracker& tracker, const SimParms& simParms, double crossoverR);
  double calculatePetalCrossover(const Tracker& tracker, const SimParms& simParms);

  bool isModuleInPetal(const DetectorModule& module, double petalPhi, double curvatureR, double crossoverR);
  bool isModuleInCircleSector(const DetectorModule& module, double startPhi, double endPhi);

  bool isModuleInEtaSector(const SimParms& simParms, const Tracker& tracker, const DetectorModule& module, int etaSector); 
  bool isModuleInPhiSector(const SimParms& simParms, const DetectorModule& module, double crossoverR, int phiSector);

}

class TriggerProcessorBandwidthVisitor : public ConstGeometryVisitor {
  typedef std::map<std::pair<int, int>, int> ProcessorConnections;
  typedef std::map<std::pair<int, int>, double> ProcessorInboundBandwidths;
  typedef std::map<std::pair<int, int>, double> ProcessorInboundStubsPerEvent;
  ProcessorConnections processorConnections_;
  ProcessorInboundBandwidths processorInboundBandwidths_;
  ProcessorInboundStubsPerEvent processorInboundStubsPerEvent_;

  map<string, map<pair<int, int>, double>> &triggerDataBandwidths_, triggerFrequenciesPerEvent_;

  const Tracker* tracker_;
  const SimParms* simParms_;

public:
  SummaryTable processorConnectionSummary, processorInboundBandwidthSummary, processorInboundStubPerEventSummary;
  SummaryTable processorCommonConnectionSummary;
  TH1I moduleConnectionsDistribution;

  class ModuleConnectionData {
    int phiCpuConnections_, etaCpuConnections_;
  public:
    set<std::pair<int, int>> connectedProcessors;
    int phiCpuConnections() const { return phiCpuConnections_; }
    int etaCpuConnections() const { return etaCpuConnections_; }
    int totalCpuConnections() const { return phiCpuConnections_*etaCpuConnections_; }
    void phiCpuConnections(int conn) { phiCpuConnections_ = conn; }
    void etaCpuConnections(int conn) { etaCpuConnections_ = conn; }
    ModuleConnectionData() : phiCpuConnections_(0), etaCpuConnections_(0) {}
  };
  typedef map<const Module*,ModuleConnectionData> ModuleConnectionMap; 

  ModuleConnectionMap moduleConnections;

private:
  int numProcEta, numProcPhi;
  double crossoverR_;

  double inboundBandwidthTotal = 0.;
  int processorConnectionsTotal = 0;
  double inboundStubsPerEventTotal = 0.;
public:

  TriggerProcessorBandwidthVisitor(map<string, map<pair<int, int>, double>>& triggerDataBandwidths, map<string, map<pair<int, int>, double>>& triggerFrequenciesPerEvent) :
      triggerDataBandwidths_(triggerDataBandwidths),
      triggerFrequenciesPerEvent_(triggerFrequenciesPerEvent)
  {}

  void preVisit() {
    processorConnectionSummary.setHeader("Phi", "Eta");
    processorCommonConnectionSummary.setHeader("Phi", "Eta");
    processorInboundBandwidthSummary.setHeader("Phi", "Eta");
    processorInboundStubPerEventSummary.setHeader("Phi", "Eta");

    processorInboundBandwidthSummary.setPrecision(3);
    processorInboundStubPerEventSummary.setPrecision(3);

    moduleConnectionsDistribution.Reset();
    moduleConnectionsDistribution.SetNameTitle("ModuleConnDist", "Number of connections to trigger processors;Connections;Modules");
    moduleConnectionsDistribution.SetBins(11, -.5, 10.5);


  }

  void visit(const SimParms& sp) {
    simParms_ = &sp;
    numProcEta = sp.numTriggerTowersEta();
    numProcPhi = sp.numTriggerTowersPhi();
  }

  void visit(const Tracker& t) { 
    tracker_ = &t; 
    crossoverR_ = AnalyzerHelpers::calculatePetalCrossover(*tracker_, *simParms_);
  }

  void visit(const DetectorModule& m) {
    TableRef p = m.tableRef();

    int etaConnections = 0, totalConnections = 0;

    for (int i=0; i < numProcEta; i++) {
      if (AnalyzerHelpers::isModuleInEtaSector(*simParms_, *tracker_, m, i)) {
        etaConnections++;
        for (int j=0; j < numProcPhi; j++) {
          if (AnalyzerHelpers::isModuleInPhiSector(*simParms_, m, crossoverR_, j)) {
            totalConnections++;

            processorConnections_[std::make_pair(j,i)] += 1;
            processorConnectionSummary.setCell(j+1, i+1, processorConnections_[std::make_pair(j,i)]);

            moduleConnections[&m].connectedProcessors.insert(make_pair(i+1, j+1));

            processorInboundBandwidths_[std::make_pair(j,i)] += triggerDataBandwidths_[p.table][std::make_pair(p.row, p.col)]; // *2 takes into account negative Z's
            processorInboundBandwidthSummary.setCell(j+1, i+1, processorInboundBandwidths_[std::make_pair(j,i)]);

            processorInboundStubsPerEvent_[std::make_pair(j,i)] += triggerFrequenciesPerEvent_[p.table][std::make_pair(p.row, p.col)];
            processorInboundStubPerEventSummary.setCell(j+1, i+1, processorInboundStubsPerEvent_[std::make_pair(j,i)]);

          } 
        }
      }
    }
    moduleConnections[&m].etaCpuConnections(etaConnections);
    moduleConnections[&m].phiCpuConnections(totalConnections > 0 ? totalConnections/etaConnections : 0);

    for (const auto& mvp : processorInboundBandwidths_) inboundBandwidthTotal += mvp.second;
    for (const auto& mvp : processorConnections_) processorConnectionsTotal += mvp.second;
    for (const auto& mvp : processorInboundStubsPerEvent_) inboundStubsPerEventTotal += mvp.second;
  }

  void postVisit() {
    processorInboundBandwidthSummary.setSummaryCell("Total", inboundBandwidthTotal);
    processorConnectionSummary.setSummaryCell("Total", processorConnectionsTotal);
    processorInboundStubPerEventSummary.setSummaryCell("Total", inboundStubsPerEventTotal);

    for (auto mvp : moduleConnections) {
      moduleConnectionsDistribution.Fill(mvp.second.totalCpuConnections(), 1);
      std::set<pair<int, int>> connectedProcessors = mvp.second.connectedProcessors; // we make a copy of the set here
      while (!connectedProcessors.empty()) {
        pair<int, int> colRef = *connectedProcessors.begin();
        int col = colRef.second + numProcPhi*(colRef.first-1);
        if (!processorCommonConnectionSummary.hasCell(-1, col)) processorCommonConnectionSummary.setCell(0, col, "t" + any2str(colRef.first) + "," + any2str(colRef.second)); // set column header
        for (std::set<pair<int, int> >::const_iterator pIt = connectedProcessors.begin(); pIt != connectedProcessors.end(); ++pIt) {
          int row = pIt->second + numProcPhi*(pIt->first-1);
          if (!processorCommonConnectionSummary.hasCell(row, 0)) processorCommonConnectionSummary.setCell(row, 0, "t" + any2str(pIt->first) + "," + any2str(pIt->second));
          processorCommonConnectionSummary.setCell(row, col, 1, std::plus<int>());
        }
        connectedProcessors.erase(connectedProcessors.begin());
      } 
    }
  }
};



#endif
