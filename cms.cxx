#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RNTupleView.hxx>
#include <ROOT/RVec.hxx>

#include <TCanvas.h>
#include <TChain.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTreePerfStats.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "util.h"


static void TreeOptimized(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();

   std::unique_ptr<TChain> tree(new TChain("Events"));
   tree->Add(path.c_str());

   TTreePerfStats *ps = new TTreePerfStats("ioperf", tree.get());

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
   std::chrono::steady_clock::time_point ts_first;
   for (decltype(nEntries) entryId = 0; entryId < nEntries; ++entryId) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;
      if (entryId == 1) {
         ts_first = std::chrono::steady_clock::now();
      }
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

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   ps->Print();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
}


static void NTupleOptimized(const std::string &path) {
   using ENTupleInfo = ROOT::Experimental::ENTupleInfo;
   using RNTupleModel = ROOT::Experimental::RNTupleModel;
   using RNTupleReader = ROOT::Experimental::RNTupleReader;
   using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;

   auto ts_init = std::chrono::steady_clock::now();

   auto model = RNTupleModel::Create();
   RNTupleReadOptions options;
   options.SetClusterCache(RNTupleReadOptions::EClusterCache::kOn);
   auto ntuple = RNTupleReader::Open(std::move(model), "Events", path, options);
   ntuple->EnableMetrics();

   auto h = new TH1F("Dimuon_mass", "Dimuon_mass", 30000, 0.25, 300);

   auto viewMuon = ntuple->GetViewCollection("nMuon");
   auto viewMuonCharge = viewMuon.GetView<std::int32_t>("nMuon.Muon_charge");
   auto viewMuonPt = viewMuon.GetView<float>("nMuon.Muon_pt");
   auto viewMuonEta = viewMuon.GetView<float>("nMuon.Muon_eta");
   auto viewMuonPhi = viewMuon.GetView<float>("nMuon.Muon_phi");
   auto viewMuonMass = viewMuon.GetView<float>("nMuon.Muon_mass");

   std::chrono::steady_clock::time_point ts_first;
   for (auto entryId : ntuple->GetViewRange()) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;
      if (entryId == 1) {
         ts_first = std::chrono::steady_clock::now();
      }

      if (viewMuon(entryId) != 2)
         continue;

      std::int32_t charges[2];
      int i = 0;
      for (auto m : viewMuon.GetViewRange(entryId)) {
         charges[i++] = viewMuonCharge(m);
      }
      if (charges[0] == charges[1])
         continue;

      float pt[2];
      float eta[2];
      float phi[2];
      float mass[2];
      i = 0;
      for (auto m : viewMuon.GetViewRange(entryId)) {
         pt[i] = viewMuonPt(m);
         eta[i] = viewMuonEta(m);
         phi[i] = viewMuonPhi(m);
         mass[i] = viewMuonMass(m);
         ++i;
      }

      float x_sum = 0.;
      float y_sum = 0.;
      float z_sum = 0.;
      float e_sum = 0.;
      for (std::size_t i = 0u; i < 2; ++i) {
         // Convert to (e, x, y, z) coordinate system and update sums
         const auto x = pt[i] * std::cos(phi[i]);
         x_sum += x;
         const auto y = pt[i] * std::sin(phi[i]);
         y_sum += y;
         const auto z = pt[i] * std::sinh(eta[i]);
         z_sum += z;
         const auto e = std::sqrt(x * x + y * y + z * z + mass[i] * mass[i]);
         e_sum += e;
      }
      // Return invariant mass with (+, -, -, -) metric
      auto fmass = std::sqrt(e_sum * e_sum - x_sum * x_sum - y_sum * y_sum - z_sum * z_sum);
      h->Fill(fmass);
   }
   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   ntuple->PrintInfo(ENTupleInfo::kMetrics);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
}


static void Usage(const char *progname) {
  printf("%s [-i input.root/ntuple]\n", progname);
}

int main(int argc, char **argv) {
   std::string path;
   int c;
   while ((c = getopt(argc, argv, "hvi:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'i':
         path = optarg;
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   if (path.empty()) {
      Usage(argv[0]);
      return 1;
   }

   auto suffix = GetSuffix(path);
   switch (GetFileFormat(suffix)) {
   case FileFormats::kRoot:
      TreeOptimized(path);
      break;
   case FileFormats::kNtuple:
      NTupleOptimized(path);
      break;
   default:
      std::cerr << "Invalid file format: " << suffix << std::endl;
      return 1;
   }

   return 0;
}
