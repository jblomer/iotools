#include <ROOT/RDataFrame.hxx>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDS.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RNTupleView.hxx>
#include <ROOT/RVec.hxx>

#include <TApplication.h>
#include <TCanvas.h>
#include <TChain.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH1D.h>
#include <TFile.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTreePerfStats.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "util.h"

bool g_perf_stats = false;
bool g_show = false;
unsigned int g_nstreams = 0;
bool g_mmap = false;

static ROOT::Experimental::RNTupleReadOptions GetRNTupleOptions() {
   using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;

   RNTupleReadOptions options;
   if (g_mmap) {
      options.SetClusterCache(RNTupleReadOptions::kMMap);
      std::cout << "{Using MMAP cluster pool}" << std::endl;
   } else {
      options.SetClusterCache(RNTupleReadOptions::kOn);
      std::cout << "{Using async cluster pool}" << std::endl;
      if (g_nstreams > 0)
         options.SetNumStreams(g_nstreams);
      std::cout << "{Using " << options.GetNumStreams() << " streams}" << std::endl;
   }
   return options;
}

static void Show(TH1D *h) {
   new TApplication("", nullptr, nullptr);

   gStyle->SetTextFont(42);
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
   c->Modified();

   std::cout << "press ENTER to exit..." << std::endl;
   auto future = std::async(std::launch::async, getchar);
   while (true) {
      gSystem->ProcessEvents();
      if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
         break;
   }
}

static void TreeDirect(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();

   auto file = TFile::Open(path.c_str());
   auto tree = file->Get<TTree>("Events");
   TTreePerfStats *ps = nullptr;
   if (g_perf_stats)
      ps = new TTreePerfStats("ioperf", tree);

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

   auto hMass = new TH1D("Dimuon_mass", "Dimuon_mass", 2000, 0.25, 300);

   auto nEntries = tree->GetEntries();
   std::chrono::steady_clock::time_point ts_first;
   for (decltype(nEntries) entryId = 0; entryId < nEntries; ++entryId) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;
      if (entryId == 1) {
         ts_first = std::chrono::steady_clock::now();
      }

      tree->LoadTree(entryId);

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
      hMass->Fill(mass);
   }

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ps->Print();

   if (g_show)
      Show(hMass);
   delete hMass;
}


static void NTupleDirect(const std::string &path) {
   using ENTupleInfo = ROOT::Experimental::ENTupleInfo;
   using RNTupleModel = ROOT::Experimental::RNTupleModel;
   using RNTupleReader = ROOT::Experimental::RNTupleReader;

   auto ts_init = std::chrono::steady_clock::now();

   auto model = RNTupleModel::Create();
   auto options = GetRNTupleOptions();
   auto ntuple = RNTupleReader::Open(std::move(model), "Events", path, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();

   auto hMass = new TH1D("Dimuon_mass", "Dimuon_mass", 2000, 0.25, 300);

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
      hMass->Fill(fmass);
   }
   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ntuple->PrintInfo(ENTupleInfo::kMetrics);
   if (g_show)
      Show(hMass);
}


template <typename T>
static T InvariantMassStdVector(std::vector<T>& pt, std::vector<T>& eta, std::vector<T>& phi, std::vector<T>& mass)
{
   //assert(pt.size() == eta.size() == phi.size() == mass.size() == 2);
   // We adopt the memory here, no copy
   ROOT::RVec<float> rvPt(pt);
   ROOT::RVec<float> rvEta(eta);
   ROOT::RVec<float> rvPhi(phi);
   ROOT::RVec<float> rvMass(mass);

   return InvariantMass(rvPt, rvEta, rvPhi, rvMass);
}

static void NTupleRdf(const std::string &path) {
   using RNTupleDS = ROOT::Experimental::RNTupleDS;

   auto ts_init = std::chrono::steady_clock::now();
   std::chrono::steady_clock::time_point ts_first;
   bool ts_first_set = false;

   auto options = GetRNTupleOptions();
   auto pageSource = ROOT::Experimental::Detail::RPageSource::Create("Events", path, options);
   ROOT::RDataFrame df(std::make_unique<RNTupleDS>(std::move(pageSource)));

   auto df_timing = df.Define("TIMING", [&ts_first, &ts_first_set]() {
      if (!ts_first_set)
         ts_first = std::chrono::steady_clock::now();
      ts_first_set = true;
      return ts_first_set;}).Filter([](bool b){ return b; }, {"TIMING"});
   //auto df_2mu = df.Define("muon_size", [](const std::vector<int> &v) { return v.size(); }, {"nMuon_nMuon_Muon_charge"})
   //   .Filter([](size_t s) { return s == 2; }, {"muon_size"});
   auto df_2mu = df_timing.Filter([](std::uint32_t s) { return s == 2; }, {"nMuon_"});
   auto df_os = df_2mu.Filter([](const std::vector<int> &c) {return c[0] != c[1];}, {"nMuon_nMuon_Muon_charge"});
   auto df_mass = df_os.Define("Dimuon_mass", InvariantMassStdVector<float>,
      {"nMuon_nMuon_Muon_pt", "nMuon_nMuon_Muon_eta", "nMuon_nMuon_Muon_phi", "nMuon_nMuon_Muon_mass"});
   auto hMass = df_mass.Histo1D({"Dimuon_mass", "Dimuon_mass", 2000, 0.25, 300}, "Dimuon_mass");

   *hMass;
   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_show)
      Show(hMass.GetPtr());
}


static void TreeRdf(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();
   std::chrono::steady_clock::time_point ts_first;
   bool ts_first_set = false;

   ROOT::RDataFrame df("Events", path);
   auto df_timing = df.Define("TIMING", [&ts_first, &ts_first_set]() {
      if (!ts_first_set)
         ts_first = std::chrono::steady_clock::now();
      ts_first_set = true;
      return ts_first_set;}).Filter([](bool b){ return b; }, {"TIMING"});
   auto df_2mu = df_timing.Filter([](unsigned int s) { return s == 2; }, {"nMuon"});
   auto df_os = df_2mu.Filter([](const ROOT::VecOps::RVec<int> &c) {return c[0] != c[1];}, {"Muon_charge"});
   //auto df_os = df_2mu.Filter("Muon_charge[0] != Muon_charge[1]");
   auto df_mass = df_os.Define("Dimuon_mass", ROOT::VecOps::InvariantMass<float>,
                               {"Muon_pt", "Muon_eta", "Muon_phi", "Muon_mass"});
   auto hMass = df_mass.Histo1D({"Dimuon_mass", "Dimuon_mass", 2000, 0.25, 300}, "Dimuon_mass");

   *hMass;
   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_show)
      Show(hMass.GetPtr());
}


static void Usage(const char *progname) {
  printf("%s [-i input.root/ntuple] [-r(df)] [-s(show)] [-p(erformance stats)] \n"
         "    [-c #streams] [-m(map)]\n", progname);
}

int main(int argc, char **argv) {
   bool use_rdf = false;
   std::string path;
   int c;
   while ((c = getopt(argc, argv, "hvsrpi:c:m")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'i':
         path = optarg;
         break;
      case 'r':
         use_rdf = true;
         break;
      case 'p':
         g_perf_stats = true;
         break;
      case 's':
         g_show = true;
         break;
      case 'c':
         g_nstreams = std::stoi(optarg);
         break;
      case 'm':
         g_mmap = true;
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
      if (use_rdf) {
         TreeRdf(path);
      } else {
         TreeDirect(path);
      }
      break;
   case FileFormats::kNtuple:
      if (use_rdf) {
         NTupleRdf(path);
      } else {
         NTupleDirect(path);
      }
      break;
   default:
      std::cerr << "Invalid file format: " << suffix << std::endl;
      return 1;
   }

   return 0;
}
