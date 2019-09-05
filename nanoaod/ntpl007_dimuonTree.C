/// \file
/// \ingroup tutorial_ntuple
/// \notebook
/// Convert CMS open data from a TTree to RNTuple.
/// This tutorial illustrates data conversion and data processing with RNTuple and RDataFrame.  In contrast to the
/// LHCb open data tutorial, the data model in this tutorial is not tabular but entries have variable lengths vectors
/// Based on RDataFrame's df102_NanoAODDimuonAnalysis.C
///
/// \macro_image
/// \macro_code
///
/// \date April 2019
/// \author The ROOT Team

// NOTE: The RNTuple classes are experimental at this point.
// Functionality, interface, and data format is still subject to changes.
// Do not use for real data!

#ifdef __CINT__
R__LOAD_LIBRARY(ROOTNTuple)
R__LOAD_LIBRARY(Hist)
#endif

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDS.hxx>
#include <ROOT/RVec.hxx>

#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTreePerfStats.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "ntuple/classes.hxx"

// Import classes from experimental namespace for the time being
using RNTupleModel = ROOT::Experimental::RNTupleModel;
using RNTupleReader = ROOT::Experimental::RNTupleReader;
using RNTupleWriter = ROOT::Experimental::RNTupleWriter;
using RNTupleDS = ROOT::Experimental::RNTupleDS;

//constexpr char const* kTreeFileName = "/data/cms/C5E0A4F9-8AFD-9C43-96C1-7A644E60E390.root";
constexpr char const* kTreeFileName = "/data/cms/tree/real~lzma.root";
//constexpr char const* kTreeFileName = "/data/cms/tree/real~uncompressed.root";


void ntpl007_dimuonTree() {
   std::unique_ptr<TFile> f(TFile::Open(kTreeFileName));
   auto tree = f->Get<TTree>("Events");
   TTreePerfStats *ps = new TTreePerfStats("ioperf", tree);

   unsigned int nMuons;
   TBranch *br_nMuons;
   tree->SetBranchAddress("nMuon", &nMuons, &br_nMuons);

   int Muon_charge[2];
   TBranch *br_MuonCharge;
   tree->SetBranchAddress("Muon_charge", &Muon_charge, &br_MuonCharge);

   float Muon_phi[2];
   TBranch *br_MuonPhi;
   tree->SetBranchAddress("Muon_phi", &Muon_phi, &br_MuonPhi);
   float Muon_pt[2];
   TBranch *br_MuonPt;
   tree->SetBranchAddress("Muon_pt", &Muon_pt, &br_MuonPt);
   float Muon_eta[2];
   TBranch *br_MuonEta;
   tree->SetBranchAddress("Muon_eta", &Muon_eta, &br_MuonEta);
   float Muon_mass[2];
   TBranch *br_MuonMass;
   tree->SetBranchAddress("Muon_mass", &Muon_mass, &br_MuonMass);

   auto h = new TH1F("Dimuon_mass", "Dimuon_mass", 30000, 0.25, 300);

   auto nEntries = tree->GetEntries();
   for (decltype(nEntries) entryId = 0; entryId < nEntries; ++entryId) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;
      br_nMuons->GetEntry(entryId);
      if (nMuons != 2)
         continue;
      br_MuonCharge->GetEntry(entryId);
      if (Muon_charge[0] == Muon_charge[1])
         continue;

      br_MuonPhi->GetEntry(entryId);
      br_MuonPt->GetEntry(entryId);
      br_MuonEta->GetEntry(entryId);
      br_MuonMass->GetEntry(entryId);
      float x_sum = 0.;
      float y_sum = 0.;
      float z_sum = 0.;
      float e_sum = 0.;
      for (std::size_t i = 0u; i < 2; ++i) {
         // Convert to (e, x, y, z) coordinate system and update sums
         const auto x = Muon_pt[i] * std::cos(Muon_phi[i]);
         x_sum += x;
         const auto y = Muon_pt[i] * std::sin(Muon_phi[i]);
         y_sum += y;
         const auto z = Muon_pt[i] * std::sinh(Muon_eta[i]);
         z_sum += z;
         const auto e = std::sqrt(x * x + y * y + z * z + Muon_mass[i] * Muon_mass[i]);
         e_sum += e;
      }
      // Return invariant mass with (+, -, -, -) metric
      auto mass = std::sqrt(e_sum * e_sum - x_sum * x_sum - y_sum * y_sum - z_sum * z_sum);
      h->Fill(mass);
   }

   ps->Print();

   gStyle->SetOptStat(0); gStyle->SetTextFont(42);
   auto c = new TCanvas("c", "", 800, 700);
   c->SetLogx(); c->SetLogy();

   h->SetTitle("");
   h->GetXaxis()->SetTitle("m_{#mu#mu} (GeV)"); h->GetXaxis()->SetTitleSize(0.04);
   h->GetYaxis()->SetTitle("N_{Events}"); h->GetYaxis()->SetTitleSize(0.04);
   h->DrawCopy();

   TLatex label; label.SetNDC(true);
   label.DrawLatex(0.175, 0.740, "#eta");
   label.DrawLatex(0.205, 0.775, "#rho,#omega");
   label.DrawLatex(0.270, 0.740, "#phi");
   label.DrawLatex(0.400, 0.800, "J/#psi");
   label.DrawLatex(0.415, 0.670, "#psi'");
   label.DrawLatex(0.485, 0.700, "Y(1,2,3S)");
   label.DrawLatex(0.755, 0.680, "Z");
   label.SetTextSize(0.040); label.DrawLatex(0.100, 0.920, "#bf{CMS Open Data}");
   label.SetTextSize(0.030); label.DrawLatex(0.630, 0.920, "#sqrt{s} = 8 TeV, L_{int} = 11.6 fb^{-1}");
}

int main() {
   ntpl007_dimuonTree();
}
