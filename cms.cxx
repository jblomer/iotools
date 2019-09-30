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
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>


static void Usage(const char *progname) {
  printf("%s [-i input.root] [-i ...]\n", progname);
}

int main(int argc, char **argv) {
   std::vector<std::string> inputPaths;
   int c;
   while ((c = getopt(argc, argv, "hvi:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'i':
         inputPaths.emplace_back(optarg);
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   if (inputPaths.empty()) {
      Usage(argv[0]);
      return 1;
   }

   std::unique_ptr<TChain> tree(new TChain("Events"));
   for (const auto &p : inputPaths)
      tree->Add(p.c_str());

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

   return 0;
}
