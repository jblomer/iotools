/// \file cms_10br.cc
/// \brief Measure throughput for reading 10 TTree/RNTuple columns. Based on `cms.cxx`
/// in the parent directory.

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDS.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RNTupleView.hxx>
#include <ROOT/RVec.hxx>

#include <TChain.h>
#include <TFile.h>
#include <TTreePerfStats.h>

#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include <utility>

#include "cms_event.h"
#include "util.h"

bool g_perf_stats = false;
unsigned int g_cluster_bunch_size = 1;

static ROOT::Experimental::RNTupleReadOptions GetRNTupleOptions() {
   using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;

   RNTupleReadOptions options;
   options.SetClusterBunchSize(g_cluster_bunch_size);
   return options;
}

static void TreeDirect(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();

   auto file = OpenOrDownload(path);
   auto tree = file->Get<TTree>("Events");
   TTreePerfStats *ps = nullptr;
   if (g_perf_stats)
      ps = new TTreePerfStats("ioperf", tree);

   int nMuon;
   int Muon_eta[MUON_COUNT];
   float Muon_phi[MUON_COUNT];
   float MET_pt;
   int nJet;
   float Jet_pt[JET_COUNT];
   float Jet_eta[JET_COUNT];
   float Jet_phi[JET_COUNT];
   float Jet_mass[JET_COUNT];
   float Jet_btag[JET_COUNT];

   TBranch *br[10];
   tree->SetBranchAddress("nMuon", &nMuon, &br[0]);
   tree->SetBranchAddress("Muon_eta", &Muon_eta, &br[1]);
   tree->SetBranchAddress("Muon_phi", &Muon_phi, &br[2]);
   tree->SetBranchAddress("MET_pt", &MET_pt, &br[3]);
   tree->SetBranchAddress("nJet", &nJet, &br[4]);
   tree->SetBranchAddress("Jet_pt", &Jet_pt, &br[5]);
   tree->SetBranchAddress("Jet_eta", &Jet_eta, &br[6]);
   tree->SetBranchAddress("Jet_phi", &Jet_phi, &br[7]);
   tree->SetBranchAddress("Jet_mass", &Jet_mass, &br[8]);
   tree->SetBranchAddress("Jet_btag", &Jet_btag, &br[9]);

   auto nEntries = tree->GetEntries();
   std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
   for (decltype(nEntries) entryId = 0; entryId < nEntries; ++entryId) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;

      tree->LoadTree(entryId);

      for (size_t i = 0; i < std::extent<decltype(br)>::value; ++i)
         br[i]->GetEntry(entryId);
   }

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ps->Print();
}

static void NTupleDirect(const std::string &path) {
   using ENTupleInfo = ROOT::Experimental::ENTupleInfo;
   using RNTupleModel = ROOT::Experimental::RNTupleModel;
   using RNTupleReader = ROOT::Experimental::RNTupleReader;

   // Trigger download if needed.
   delete OpenOrDownload(path);

   auto ts_init = std::chrono::steady_clock::now();

   auto model = RNTupleModel::Create();
   auto options = GetRNTupleOptions();
   auto ntuple = RNTupleReader::Open(std::move(model), "Events", path, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();

   auto viewMuon = ntuple->GetViewCollection("nMuon");
   auto viewMuonEta = viewMuon.GetView<float>("Muon_eta");
   auto viewMuonPhi = viewMuon.GetView<float>("Muon_phi");
   auto viewMET_pt = ntuple->GetView<float>("MET_pt");
   auto viewJet = ntuple->GetViewCollection("nJet");
   auto viewJetPt = viewJet.GetView<float>("Jet_pt");
   auto viewJetEta = viewJet.GetView<float>("Jet_eta");
   auto viewJetPhi = viewJet.GetView<float>("Jet_phi");
   auto viewJetMass = viewJet.GetView<float>("Jet_mass");
   auto viewJetBtag = viewJet.GetView<float>("Jet_btag");

   std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
   for (auto entryId : ntuple->GetEntryRange()) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;

      (void)viewMuon(entryId);
      for (auto m : viewMuon.GetCollectionRange(entryId)) {
         (void)viewMuonEta(m);
         (void)viewMuonPhi(m);
      }
      (void)viewMET_pt(entryId);
      (void)viewJet(entryId);
      for (auto m : viewJet.GetCollectionRange(entryId)) {
         (void)viewJetPt(m);
         (void)viewJetEta(m);
         (void)viewJetPhi(m);
         (void)viewJetMass(m);
         (void)viewJetBtag(m);
      }
   }
   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ntuple->PrintInfo(ENTupleInfo::kMetrics);
}

static void Usage(const char *progname) {
  printf("%s [-i input.root/ntuple] [-m(t)] [-p(erformance stats)] [-x cluster bunch size]\n",
         progname);
}

int main(int argc, char **argv) {
   auto ts_init = std::chrono::steady_clock::now();

   std::string path;
   int c;
   while ((c = getopt(argc, argv, "hvpmi:x:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'i':
         path = optarg;
         break;
      case 'p':
         g_perf_stats = true;
         break;
      case 'm':
         ROOT::EnableImplicitMT();
         break;
      case 'x':
         g_cluster_bunch_size = atoi(optarg);
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
     TreeDirect(path);
     break;
   case FileFormats::kNtuple:
     NTupleDirect(path);
     break;
   default:
     std::cerr << "Invalid file format: " << suffix << std::endl;
     return 1;
   }

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_main = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_init).count();
   std::cout << "Runtime-Main: " << runtime_main << "us" << std::endl;

   return 0;
}
