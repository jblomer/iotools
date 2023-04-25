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
#include <TRootCanvas.h>
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

bool g_perf_stats = false;
bool g_show = false;
unsigned int g_cluster_bunch_size = 1;

static ROOT::Experimental::RNTupleReadOptions GetRNTupleOptions() {
   using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;

   RNTupleReadOptions options;
   options.SetClusterBunchSize(g_cluster_bunch_size);
   return options;
}

static void Show(TH1D *h) {
   auto app = TApplication("", nullptr, nullptr);

   gStyle->SetTextFont(42);
   auto c = TCanvas("c", "", 800, 700);
   c.SetLogx(); c.SetLogy();

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
   label.SetTextSize(0.030); label.DrawLatex(0.50, 0.920, "#sqrt{s} = 8 TeV, L_{int} = 11.6 fb^{-1}");
   c.Modified();
   c.Update();
   static_cast<TRootCanvas*>(c.GetCanvasImp())
      ->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
   app.Run();
}

static void TreeDirect(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();

   auto file = OpenOrDownload(path);
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
   std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
   for (decltype(nEntries) entryId = 0; entryId < nEntries; ++entryId) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;

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

   // Trigger download if needed.
   delete OpenOrDownload(path);

   auto ts_init = std::chrono::steady_clock::now();

   auto model = RNTupleModel::Create();
   auto options = GetRNTupleOptions();
   auto ntuple = RNTupleReader::Open(std::move(model), "Events", path, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();

   auto hMass = new TH1D("Dimuon_mass", "Dimuon_mass", 2000, 0.25, 300);

   const auto &desc = ntuple->GetDescriptor();
   const auto columnId = desc->FindPhysicalColumnId(desc->FindFieldId("nMuon"), 0);
   const auto collectionFieldId = desc->GetColumnDescriptor(columnId).GetFieldId();
   const auto collectionFieldName = desc->GetFieldDescriptor(collectionFieldId).GetFieldName();

   auto viewMuon = ntuple->GetViewCollection(collectionFieldName);
   auto viewMuonCharge = viewMuon.GetView<std::int32_t>("Muon_charge");
   auto viewMuonPt = viewMuon.GetView<float>("Muon_pt");
   auto viewMuonEta = viewMuon.GetView<float>("Muon_eta");
   auto viewMuonPhi = viewMuon.GetView<float>("Muon_phi");
   auto viewMuonMass = viewMuon.GetView<float>("Muon_mass");

   std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
   for (auto entryId : ntuple->GetEntryRange()) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;

      if (viewMuon(entryId) != 2)
         continue;

      std::int32_t charges[2];
      int i = 0;
      for (auto m : viewMuon.GetCollectionRange(entryId)) {
         charges[i++] = viewMuonCharge(m);
      }
      if (charges[0] == charges[1])
         continue;

      float pt[2];
      float eta[2];
      float phi[2];
      float mass[2];
      i = 0;
      for (auto m : viewMuon.GetCollectionRange(entryId)) {
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


static void Rdf(ROOT::RDataFrame &df) {
   auto df_2mu = df.Filter([](unsigned int s) { return s == 2; }, {"nMuon"});
   auto df_os = df_2mu.Filter([](const ROOT::VecOps::RVec<int> &c) {return c[0] != c[1];}, {"Muon_charge"});
   //auto df_os = df_2mu.Filter("Muon_charge[0] != Muon_charge[1]");
   auto df_mass = df_os.Define("Dimuon_mass", ROOT::VecOps::InvariantMass<float>,
                               {"Muon_pt", "Muon_eta", "Muon_phi", "Muon_mass"});
   auto hMass = df_mass.Histo1D<float>({"Dimuon_mass", "Dimuon_mass", 2000, 0.25, 300}, "Dimuon_mass");

   *hMass;

   if (g_show)
      Show(hMass.GetPtr());
}


static void Usage(const char *progname) {
  printf("%s [-i input.root/ntuple] [-r(df)] [-m(t)] [-s(show)] [-p(erformance stats)] [-x cluster bunch size]\n",
         progname);
}

int main(int argc, char **argv) {
   auto ts_init = std::chrono::steady_clock::now();

   bool use_rdf = false;
   std::string path;
   int c;
   while ((c = getopt(argc, argv, "hvsrpmi:x:")) != -1) {
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

   auto verbosity = ROOT::Experimental::RLogScopedVerbosity(
      ROOT::Detail::RDF::RDFLogChannel(), ROOT::Experimental::ELogLevel::kInfo);

   auto suffix = GetSuffix(path);
   switch (GetFileFormat(suffix)) {
   case FileFormats::kRoot:
      if (use_rdf) {
         auto file = OpenOrDownload(path);
         auto tree = file->Get<TTree>("Events");
         TTreePerfStats *ps = nullptr;
         if (g_perf_stats)
            ps = new TTreePerfStats("ioperf", tree);
         ROOT::RDataFrame df(*tree);
         Rdf(df);
         if (g_perf_stats)
            ps->Print();
      } else {
         TreeDirect(path);
      }
      break;
   case FileFormats::kNtuple:
      if (use_rdf) {
         auto options = GetRNTupleOptions();
         auto pageSource = ROOT::Experimental::Detail::RPageSource::Create("Events", path, options);
         auto ds = std::make_unique<ROOT::Experimental::RNTupleDS>(std::move(pageSource));
         if (g_perf_stats)
            ds->EnableMetrics();
         ROOT::RDataFrame df(std::move(ds));
         Rdf(df);
      } else {
         NTupleDirect(path);
      }
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
