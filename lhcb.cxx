/**
 * Copyright CERN; jblomer@cern.ch
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDS.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <Compression.h>
#include <TApplication.h>
#include <TBranch.h>
#include <TCanvas.h>
#include <TClassTable.h>
#include <TFile.h>
#include <TH1D.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreePerfStats.h>

#include "util.h"

bool g_perf_stats = false;
bool g_show = false;

constexpr double kKaonMassMeV = 493.677;


static void Show(TH1D *h = nullptr) {
   new TApplication("", nullptr, nullptr);

   gStyle->SetTextFont(42);
   auto c = new TCanvas("c", "", 800, 700);
   h->GetXaxis()->SetTitle("m_{KKK} [MeV/c^{2}]");
   h->DrawCopy();
   c->Modified();

   std::cout << "press ENTER to exit..." << std::endl;
   auto future = std::async(std::launch::async, getchar);
   while (true) {
      gSystem->ProcessEvents();
      if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
         break;
   }
}


static double GetP2(double px, double py, double pz)
{
   return px*px + py*py + pz*pz;
}

static double GetKE(double px, double py, double pz)
{
   double p2 = GetP2(px, py, pz);
   return sqrt(p2 + kKaonMassMeV*kKaonMassMeV);
}



static void Dataframe(ROOT::RDataFrame &frame, int nslots)
{
   auto ts_init = std::chrono::steady_clock::now();
   std::chrono::steady_clock::time_point ts_first;
   bool ts_first_set = false;

   std::vector<double> sums(nslots, 0.0);

   auto fn_muon_cut = [](int is_muon) { return !is_muon; };
   auto fn_sum_slot = [&sums](
      unsigned int slot,
      double h1_px,
      double h1_py,
      double h1_pz,
      double h1_prob_k,
      double h1_prob_pi,
      int h1_charge,
      double h2_px,
      double h2_py,
      double h2_pz,
      double h2_prob_k,
      double h2_prob_pi,
      int h2_charge,
      double h3_px,
      double h3_py,
      double h3_pz,
      double h3_prob_k,
      double h3_prob_pi,
      int h3_charge)
   {
      sums[slot] +=
         h1_px +
         h1_py +
         h1_pz +
         h1_prob_k +
         h1_prob_pi +
         double(h1_charge) +
         h2_px +
         h2_py +
         h2_pz +
         h2_prob_k +
         h2_prob_pi +
         double(h2_charge) +
         h3_px +
         h3_py +
         h3_pz +
         h3_prob_k +
         h3_prob_pi +
         double(h3_charge);
   };

   auto df_timing = frame.Define("TIMING", [&ts_first, &ts_first_set]() {
      if (!ts_first_set)
         ts_first = std::chrono::steady_clock::now();
      ts_first_set = true;
      return ts_first_set;}).Filter([](bool b){ return b; }, {"TIMING"});

   df_timing.Filter(fn_muon_cut, {"H1_isMuon"})
            .Filter(fn_muon_cut, {"H2_isMuon"})
            .Filter(fn_muon_cut, {"H3_isMuon"})
            .ForeachSlot(fn_sum_slot, {
               "H1_PX",
               "H1_PY",
               "H1_PZ",
               "H1_ProbK",
               "H1_ProbPi",
               "H1_Charge",
               "H2_PX",
               "H2_PY",
               "H2_PZ",
               "H2_ProbK",
               "H2_ProbPi",
               "H2_Charge",
               "H3_PX",
               "H3_PY",
               "H3_PZ",
               "H3_ProbK",
               "H3_ProbPi",
               "H3_Charge"});

   double total_sum = 0.0;
   for (unsigned i = 0; i < sums.size(); ++i)
      total_sum += sums[i];

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;

   if (g_show)
      Show();

   //unsigned nevent = root_chain.GetEntries();
   printf("finished, result: %lf, skipped ?\n", total_sum);
}


static void TreeDirect(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();

   auto file = TFile::Open(path.c_str());
   auto tree = file->Get<TTree>("DecayTree");
   TTreePerfStats *ps = nullptr;
   if (g_perf_stats)
      ps = new TTreePerfStats("ioperf", tree);

   TBranch *br_h1_px = nullptr;
   TBranch *br_h1_py = nullptr;
   TBranch *br_h1_pz = nullptr;
   TBranch *br_h1_prob_k = nullptr;
   TBranch *br_h1_prob_pi = nullptr;
   TBranch *br_h1_is_muon = nullptr;
   TBranch *br_h2_px = nullptr;
   TBranch *br_h2_py = nullptr;
   TBranch *br_h2_pz = nullptr;
   TBranch *br_h2_prob_k = nullptr;
   TBranch *br_h2_prob_pi = nullptr;
   TBranch *br_h2_is_muon = nullptr;
   TBranch *br_h3_px = nullptr;
   TBranch *br_h3_py = nullptr;
   TBranch *br_h3_pz = nullptr;
   TBranch *br_h3_prob_k = nullptr;
   TBranch *br_h3_prob_pi = nullptr;
   TBranch *br_h3_is_muon = nullptr;

   double h1_px;
   double h1_py;
   double h1_pz;
   double h1_prob_k;
   double h1_prob_pi;
   int h1_is_muon;
   double h2_px;
   double h2_py;
   double h2_pz;
   double h2_prob_k;
   double h2_prob_pi;
   int h2_is_muon;
   double h3_px;
   double h3_py;
   double h3_pz;
   double h3_prob_k;
   double h3_prob_pi;
   int h3_is_muon;

   tree->SetBranchAddress("H1_PX",     &h1_px,      &br_h1_px);
   tree->SetBranchAddress("H1_PY",     &h1_py,      &br_h1_py);
   tree->SetBranchAddress("H1_PZ",     &h1_pz,      &br_h1_pz);
   tree->SetBranchAddress("H1_ProbK",  &h1_prob_k,  &br_h1_prob_k);
   tree->SetBranchAddress("H1_ProbPi", &h1_prob_pi, &br_h1_prob_pi);
   tree->SetBranchAddress("H1_isMuon", &h1_is_muon, &br_h1_is_muon);
   tree->SetBranchAddress("H2_PX",     &h2_px,      &br_h2_px);
   tree->SetBranchAddress("H2_PY",     &h2_py,      &br_h2_py);
   tree->SetBranchAddress("H2_PZ",     &h2_pz,      &br_h2_pz);
   tree->SetBranchAddress("H2_ProbK",  &h2_prob_k,  &br_h2_prob_k);
   tree->SetBranchAddress("H2_ProbPi", &h2_prob_pi, &br_h2_prob_pi);
   tree->SetBranchAddress("H2_isMuon", &h2_is_muon, &br_h2_is_muon);
   tree->SetBranchAddress("H3_PX",     &h3_px,      &br_h3_px);
   tree->SetBranchAddress("H3_PY",     &h3_py,      &br_h3_py);
   tree->SetBranchAddress("H3_PZ",     &h3_pz,      &br_h3_pz);
   tree->SetBranchAddress("H3_ProbK",  &h3_prob_k,  &br_h3_prob_k);
   tree->SetBranchAddress("H3_ProbPi", &h3_prob_pi, &br_h3_prob_pi);
   tree->SetBranchAddress("H3_isMuon", &h3_is_muon, &br_h3_is_muon);

   auto hMass = new TH1D("B_mass", "", 500, 5050, 5500);

   auto nEntries = tree->GetEntries();
   std::chrono::steady_clock::time_point ts_first;
   for (decltype(nEntries) entryId = 0; entryId < nEntries; ++entryId) {
      if ((entryId % 100000) == 0) {
         printf("processed %llu k events\n", entryId / 1000);
         //printf("dummy is %lf\n", dummy); abort();
      }
      if (entryId == 1) {
         ts_first = std::chrono::steady_clock::now();
      }

      br_h1_is_muon->GetEntry(entryId);
      if (h1_is_muon) continue;
      br_h2_is_muon->GetEntry(entryId);
      if (h2_is_muon) continue;
      br_h3_is_muon->GetEntry(entryId);
      if (h3_is_muon) continue;

      constexpr double prob_k_cut = 0.5;
      br_h1_prob_k->GetEntry(entryId);
      if (h1_prob_k < prob_k_cut) continue;
      br_h2_prob_k->GetEntry(entryId);
      if (h2_prob_k < prob_k_cut) continue;
      br_h3_prob_k->GetEntry(entryId);
      if (h3_prob_k < prob_k_cut) continue;

      constexpr double prob_pi_cut = 0.5;
      br_h1_prob_pi->GetEntry(entryId);
      if (h1_prob_pi > prob_pi_cut) continue;
      br_h2_prob_pi->GetEntry(entryId);
      if (h2_prob_pi > prob_pi_cut) continue;
      br_h3_prob_pi->GetEntry(entryId);
      if (h3_prob_pi > prob_pi_cut) continue;

      br_h1_px->GetEntry(entryId);
      br_h1_py->GetEntry(entryId);
      br_h1_pz->GetEntry(entryId);
      br_h2_px->GetEntry(entryId);
      br_h2_py->GetEntry(entryId);
      br_h2_pz->GetEntry(entryId);
      br_h3_px->GetEntry(entryId);
      br_h3_py->GetEntry(entryId);
      br_h3_pz->GetEntry(entryId);

      double b_px = h1_px + h2_px + h3_px;
      double b_py = h1_py + h2_py + h3_py;
      double b_pz = h1_pz + h2_pz + h3_pz;
      double b_p2 = GetP2(b_px, b_py, b_pz);
      double k1_E = GetKE(h1_px, h1_py, h1_pz);
      double k2_E = GetKE(h2_px, h2_py, h2_pz);
      double k3_E = GetKE(h3_px, h3_py, h3_pz);
      double b_E = k1_E + k2_E + k3_E;
      double b_mass = sqrt(b_E*b_E - b_p2);
      hMass->Fill(b_mass);

      //printf("BMASS %lf\n", b_mass);
   }

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;

   if (g_perf_stats)
      ps->Print();
   if (g_show) {
      Show(hMass);
   }

   delete hMass;
}


static void NTupleDirect(const std::string &path)
{
   using RNTupleReader = ROOT::Experimental::RNTupleReader;
   using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;

   auto ts_init = std::chrono::steady_clock::now();

   RNTupleReadOptions options;
   options.SetClusterCache(RNTupleReadOptions::EClusterCache::kOn);
   auto ntuple = RNTupleReader::Open("DecayTree", path, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();

   auto viewH1IsMuon = ntuple->GetView<int>("H1_isMuon");
   auto viewH2IsMuon = ntuple->GetView<int>("H2_isMuon");
   auto viewH3IsMuon = ntuple->GetView<int>("H3_isMuon");

   auto viewH1PX = ntuple->GetView<double>("H1_PX");
   auto viewH1PY = ntuple->GetView<double>("H1_PY");
   auto viewH1PZ = ntuple->GetView<double>("H1_PZ");
   auto viewH1ProbK = ntuple->GetView<double>("H1_ProbK");
   auto viewH1ProbPi = ntuple->GetView<double>("H1_ProbPi");
   auto viewH1Charge = ntuple->GetView<int>("H1_Charge");

   auto viewH2PX = ntuple->GetView<double>("H2_PX");
   auto viewH2PY = ntuple->GetView<double>("H2_PY");
   auto viewH2PZ = ntuple->GetView<double>("H2_PZ");
   auto viewH2ProbK = ntuple->GetView<double>("H2_ProbK");
   auto viewH2ProbPi = ntuple->GetView<double>("H2_ProbPi");
   auto viewH2Charge = ntuple->GetView<int>("H2_Charge");

   auto viewH3PX = ntuple->GetView<double>("H3_PX");
   auto viewH3PY = ntuple->GetView<double>("H3_PY");
   auto viewH3PZ = ntuple->GetView<double>("H3_PZ");
   auto viewH3ProbK = ntuple->GetView<double>("H3_ProbK");
   auto viewH3ProbPi = ntuple->GetView<double>("H3_ProbPi");
   auto viewH3Charge = ntuple->GetView<int>("H3_Charge");

   double dummy = 0;
   int nskipped = 0;
   int nevents = 0;
   std::chrono::steady_clock::time_point ts_first;
   for (auto i : ntuple->GetViewRange()) {
      nevents++;
      if ((nevents % 100000) == 0) {
         printf("processed %u k events\n", nevents / 1000);
         //printf("dummy is %lf\n", dummy); abort();
      }
      if (nevents == 1) {
         ts_first = std::chrono::steady_clock::now();
      }

      if (viewH1IsMuon(i) || viewH2IsMuon(i) || viewH3IsMuon(i)) {
         nskipped++;
         continue;
      }

      dummy +=
        viewH1PX(i) +
        viewH1PY(i) +
        viewH1PZ(i) +
        viewH1ProbK(i) +
        viewH1ProbPi(i) +
        double(viewH1Charge(i)) +
        //double(viewH1IsMuon(i)) +
        viewH2PX(i) +
        viewH2PY(i) +
        viewH2PZ(i) +
        viewH2ProbK(i) +
        viewH2ProbPi(i) +
        double(viewH2Charge(i)) +
        //double(viewH2IsMuon(i)) +
        viewH3PX(i) +
        viewH3PY(i) +
        viewH3PZ(i) +
        viewH3ProbK(i) +
        viewH3ProbPi(i) +
        double(viewH3Charge(i))
        // + double(viewH3IsMuon(i))
        ;
   }
   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;

   printf("Optimized RNTuple run: %u events read, %u events skipped "
          "(dummy: %lf)\n", nevents, nskipped, dummy);
   if (g_perf_stats)
      ntuple->PrintInfo(ROOT::Experimental::ENTupleInfo::kMetrics);
   if (g_show)
      Show();
}


static void Usage(const char *progname) {
  printf("%s [-i input.root] [-r(df)] [-p(erformance stats)] [-s(show)]\n", progname);
}


int main(int argc, char **argv) {
   std::string input_path;
   std::string input_suffix;
   bool use_rdf = false;
   int c;
   while ((c = getopt(argc, argv, "hvi:rb:psfgGV")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'i':
         input_path = optarg;
         break;
      case 'p':
         g_perf_stats = true;
         break;
      case 's':
         g_show = true;
         break;
      case 'r':
         use_rdf = true;
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   if (input_path.empty()) {
      Usage(argv[0]);
      return 1;
   }

   auto suffix = GetSuffix(input_path);
   switch (GetFileFormat(suffix)) {
   case FileFormats::kRoot:
      if (use_rdf) {
         ROOT::RDataFrame df("DecayTree", input_path);
         Dataframe(df, 1);
      } else {
         TreeDirect(input_path);
      }
      break;
   case FileFormats::kNtuple:
      if (use_rdf) {
         auto df = ROOT::Experimental::MakeNTupleDataFrame("DecayTree", input_path);
         Dataframe(df, 1);
      } else {
         NTupleDirect(input_path);
      }
      break;
   default:
      std::cerr << "Invalid file format: " << suffix << std::endl;
      return 1;
   }

   return 0;
}
