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
#include <TF1.h>
#include <TFile.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TLine.h>
#include <TMath.h>
#include <TPaveStats.h>
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
int g_cluster_bunch_size = 1;

static ROOT::Experimental::RNTupleReadOptions GetRNTupleOptions() {
   using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;

   RNTupleReadOptions options;
   options.SetClusterBunchSize(g_cluster_bunch_size);
   return options;
}

const Double_t dxbin = (0.17-0.13)/40;   // Bin-width
const Double_t sigma = 0.0012;

Double_t fdm5(Double_t *xx, Double_t *par)
{
   Double_t x = xx[0];
   if (x <= 0.13957) return 0;
   Double_t xp3 = (x-par[3])*(x-par[3]);
   Double_t res = dxbin*(par[0]*TMath::Power(x-0.13957, par[1])
       + par[2] / 2.5066/par[4]*TMath::Exp(-xp3/2/par[4]/par[4]));
   return res;
}


Double_t fdm2(Double_t *xx, Double_t *par)
{
   Double_t x = xx[0];
   if (x <= 0.13957) return 0;
   Double_t xp3 = (x-0.1454)*(x-0.1454);
   Double_t res = dxbin*(par[0]*TMath::Power(x-0.13957, 0.25)
       + par[1] / 2.5066/sigma*TMath::Exp(-xp3/2/sigma/sigma));
   return res;
}

static void Show(TH1D *hdmd, TH2D *h2) {
   new TApplication("", nullptr, nullptr);
   gStyle->SetOptFit();
   TCanvas *c1 = new TCanvas("c1","h1analysis analysis",10,10,800,600);
   c1->SetBottomMargin(0.15);
   hdmd->GetXaxis()->SetTitle("m_{K#pi#pi} - m_{K#pi}[GeV/c^{2}]");
   hdmd->GetXaxis()->SetTitleOffset(1.4);

   //fit histogram hdmd with function f5 using the log-likelihood option
   if (gROOT->GetListOfFunctions()->FindObject("f5"))
      delete gROOT->GetFunction("f5");
   TF1 *f5 = new TF1("f5",fdm5,0.139,0.17,5);
   f5->SetParameters(1000000, .25, 2000, .1454, .001);
   hdmd->Fit("f5","lr");

   //create the canvas for tau d0
   gStyle->SetOptFit(0);
   gStyle->SetOptStat(1100);
   TCanvas *c2 = new TCanvas("c2","tauD0",100,100,800,600);
   c2->SetGrid();
   c2->SetBottomMargin(0.15);

   // Project slices of 2-d histogram h2 along X , then fit each slice
   // with function f2 and make a histogram for each fit parameter
   // Note that the generated histograms are added to the list of objects
   // in the current directory.
   if (gROOT->GetListOfFunctions()->FindObject("f2"))
      delete gROOT->GetFunction("f2");
   TF1 *f2 = new TF1("f2",fdm2,0.139,0.17,2);
   f2->SetParameters(10000, 10);
   h2->FitSlicesX(f2,0,-1,1,"qln");
   TH1D *h2_1 = (TH1D*)gDirectory->Get("h2_1");
   h2_1->GetXaxis()->SetTitle("#tau[ps]");
   h2_1->SetMarkerStyle(21);
   h2_1->Draw();
   c2->Update();
   TLine *line = new TLine(0,0,0,c2->GetUymax());
   line->Draw();

   // Have the number of entries on the first histogram (to cross check when running
   // with entry lists)
   TPaveStats *psdmd = (TPaveStats *)hdmd->GetListOfFunctions()->FindObject("stats");
   psdmd->SetOptStat(1110);
   c1->Modified();
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

   auto file = OpenOrDownload(path);
   auto tree = file->Get<TTree>("h42");

   TTreePerfStats *ps = nullptr;
   if (g_perf_stats)
      ps = new TTreePerfStats("ioperf", tree);

   float md0_d;
   float ptds_d;
   float etads_d;
   float dm_d;
   float rpd0_t;
   float ptd0_d;
   Int_t ik;
   Int_t ipi;
   Int_t ipis;
   Int_t ntracks;
   Int_t njets;
   Int_t nhitrp[200];
   float rend[200];
   float rstart[200];
   float nlhk[200];
   float nlhpi[200];

   TBranch *br_md0_d = nullptr;
   TBranch *br_ptds_d = nullptr;
   TBranch *br_etads_d = nullptr;
   TBranch *br_dm_d = nullptr;
   TBranch *br_rpd0_t = nullptr;
   TBranch *br_ptd0_d = nullptr;
   TBranch *br_ik = nullptr;
   TBranch *br_ipi = nullptr;
   TBranch *br_ipis = nullptr;
   TBranch *br_ntracks = nullptr;
   TBranch *br_njets = nullptr;
   TBranch *br_nhitrp = nullptr;
   TBranch *br_rend = nullptr;
   TBranch *br_rstart = nullptr;
   TBranch *br_nlhk = nullptr;
   TBranch *br_nlhpi = nullptr;

   tree->SetBranchAddress("md0_d", &md0_d, &br_md0_d);
   tree->SetBranchAddress("ptds_d", &ptds_d, &br_ptds_d);
   tree->SetBranchAddress("etads_d", &etads_d, &br_etads_d);
   tree->SetBranchAddress("dm_d", &dm_d, &br_dm_d);
   tree->SetBranchAddress("rpd0_t", &rpd0_t, &br_rpd0_t);
   tree->SetBranchAddress("ptd0_d", &ptd0_d, &br_ptd0_d);
   tree->SetBranchAddress("ik", &ik, &br_ik);
   tree->SetBranchAddress("ipi", &ipi, &br_ipi);
   tree->SetBranchAddress("ipis", &ipis, &br_ipis);
   tree->SetBranchAddress("ntracks", &ntracks, &br_ntracks);
   tree->SetBranchAddress("njets", &njets, &br_njets);
   tree->SetBranchAddress("nhitrp", nhitrp, &br_nhitrp);
   tree->SetBranchAddress("rend", rend, &br_rend);
   tree->SetBranchAddress("rstart", rstart, &br_rstart);
   tree->SetBranchAddress("nlhk", nlhk, &br_nlhk);
   tree->SetBranchAddress("nlhpi", nlhpi, &br_nlhpi);

   auto hdmd = new TH1D("hdmd", "dm_d", 40, 0.13, 0.17);
   auto h2   = new TH2D("h2", "ptD0 vs dm_d", 30, 0.135, 0.165, 30, -3, 6);

   auto nEntries = tree->GetEntries();
   std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
   for (decltype(nEntries) entryId = 0; entryId < nEntries; ++entryId) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;

      tree->LoadTree(entryId);

      br_md0_d->GetEntry(entryId);
      if (TMath::Abs(md0_d - 1.8646) >= 0.04) continue;
      br_ptds_d->GetEntry(entryId);
      if (ptds_d <= 2.5) continue;
      br_etads_d->GetEntry(entryId);
      if (TMath::Abs(etads_d) >= 1.5) continue;

      br_ntracks->GetEntry(entryId);
      br_ik->GetEntry(entryId);  ik--; //original ik used f77 convention starting at 1
      br_ipi->GetEntry(entryId); ipi--;
      br_nhitrp->GetEntry(entryId);
      if (nhitrp[ik] * nhitrp[ipi] <= 1) continue;

      br_rend->GetEntry(entryId);
      br_rstart->GetEntry(entryId);
      if (rend[ik] - rstart[ik] <= 22) continue;
      if (rend[ipi] - rstart[ipi] <= 22) continue;

      br_nlhk->GetEntry(entryId);
      if (nlhk[ik] <= 0.1) continue;
      br_nlhpi->GetEntry(entryId);
      if (nlhpi[ipi] <= 0.1) continue;
      br_ipis->GetEntry(entryId); ipis--;
      if (nlhpi[ipis] <= 0.1) continue;

      br_njets->GetEntry(entryId);
      if (njets < 1) continue;

      br_dm_d->GetEntry(entryId);
      br_rpd0_t->GetEntry(entryId);
      br_ptd0_d->GetEntry(entryId);
      hdmd->Fill(dm_d);
      h2->Fill(dm_d, rpd0_t / 0.029979 * 1.8646 / ptd0_d);
   }

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   if (g_perf_stats)
      ps->Print();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;

   if (g_show)
      Show(hdmd, h2);
   delete hdmd;
   delete h2;
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
   auto ntuple = RNTupleReader::Open(std::move(model), "h42", path, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();

   auto hdmd = new TH1D("hdmd", "dm_d", 40, 0.13, 0.17);
   auto h2   = new TH2D("h2", "ptD0 vs dm_d", 30, 0.135, 0.165, 30, -3, 6);

   auto dm_dView = ntuple->GetView<float>("event.dm_d");
   auto rpd0_tView = ntuple->GetView<float>("event.rpd0_t");
   auto ptd0_dView = ntuple->GetView<float>("event.ptd0_d");

   auto ptds_dView = ntuple->GetView<float>("event.ptds_d");
   auto etads_dView = ntuple->GetView<float>("event.etads_d");
   auto ikView = ntuple->GetView<std::int32_t>("event.ik");
   auto ipiView = ntuple->GetView<std::int32_t>("event.ipi");
   auto ipisView = ntuple->GetView<std::int32_t>("event.ipis");
   auto md0_dView = ntuple->GetView<float>("event.md0_d");

   auto trackView = ntuple->GetViewCollection("event.tracks");
   auto nhitrpView = ntuple->GetView<std::int32_t>("event.tracks._0.nhitrp");
   auto rstartView = ntuple->GetView<float>("event.tracks._0.rstart");
   auto rendView = ntuple->GetView<float>("event.tracks._0.rend");
   auto nlhkView = ntuple->GetView<float>("event.tracks._0.nlhk");
   auto nlhpiView = ntuple->GetView<float>("event.tracks._0.nlhpi");
   auto njetsView = ntuple->GetViewCollection("event.jets");

   std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
   for (auto i : ntuple->GetEntryRange()) {
      if (i % 1000 == 0)
         std::cout << "Processed " << i << " entries" << std::endl;

      auto ik = ikView(i) - 1;
      auto ipi = ipiView(i) - 1;
      auto ipis = ipisView(i) - 1;

      if (TMath::Abs(md0_dView(i) - 1.8646) >= 0.04) continue;
      if (ptds_dView(i) <= 2.5) continue;
      if (TMath::Abs(etads_dView(i)) >= 1.5) continue;

      if (nhitrpView(*trackView.GetCollectionRange(i).begin()+ik) *
          nhitrpView(*trackView.GetCollectionRange(i).begin()+ipi) <= 1)
      {
         continue;
      }
      if (rendView(*trackView.GetCollectionRange(i).begin()+ik) -
          rstartView(*trackView.GetCollectionRange(i).begin()+ik) <= 22)
      {
         continue;
      }
      if (rendView(*trackView.GetCollectionRange(i).begin()+ipi) -
          rstartView(*trackView.GetCollectionRange(i).begin()+ipi) <= 22)
      {
         continue;
      }
      if (nlhkView(*trackView.GetCollectionRange(i).begin()+ik) <= 0.1) continue;
      if (nlhpiView(*trackView.GetCollectionRange(i).begin()+ipi) <= 0.1) continue;
      if (nlhpiView(*trackView.GetCollectionRange(i).begin()+ipis) <= 0.1) continue;
      if (njetsView(i) < 1) continue;

      hdmd->Fill(dm_dView(i));
      h2->Fill(dm_dView(i),rpd0_tView(i)/0.029979*1.8646/ptd0_dView(i));
   }

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   if (g_perf_stats)
      ntuple->PrintInfo(ENTupleInfo::kMetrics);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;

   if (g_show)
      Show(hdmd, h2);

   delete hdmd;
   delete h2;
}

static void TreeRdf(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();
   std::chrono::steady_clock::time_point ts_first;
   bool ts_first_set = false;

   ROOT::RDataFrame df("h42", path);
   auto df_timing = df.Define("TIMING", [&ts_first, &ts_first_set]() {
      if (!ts_first_set)
         ts_first = std::chrono::steady_clock::now();
      ts_first_set = true;
      return ts_first_set;}).Filter([](bool b){ return b; }, {"TIMING"});

   auto df_md0_d = df_timing.Filter([](float md0_d) {return TMath::Abs(md0_d - 1.8646) < 0.04;}, {"md0_d"});
   auto df_ptds_d = df_md0_d.Filter([](float ptds_d) {return ptds_d > 2.5;}, {"ptds_d"});
   auto df_etads_d = df_ptds_d.Filter([](float etads_d) {return etads_d < 1.5;}, {"etads_d"});

   auto df_ikipi = df_etads_d.Define("IK_C", [](int ik) {return ik - 1;}, {"ik"})
                             .Define("IPI_C", [](int ipi) {return ipi - 1;}, {"ipi"});
   auto df_nhitrp = df_ikipi.Filter([](const ROOT::VecOps::RVec<int> &nhitrp, int ik, int ipi) {
      return nhitrp[ik] * nhitrp[ipi] > 1;}, {"nhitrp", "IK_C", "IPI_C"});
   auto df_r = df_nhitrp.Filter(
      [](const ROOT::VecOps::RVec<float> &rend, const ROOT::VecOps::RVec<float> &rstart, int ik, int ipi)
         {return ((rend[ik] - rstart[ik]) > 22) && ((rend[ipi] - rstart[ipi]) > 22);},
         {"rend", "rstart", "IK_C", "IPI_C"});
   auto df_nlhk = df_r.Filter([](const ROOT::VecOps::RVec<float> &nlhk, int ik){return nlhk[ik] > 0.1;},
                              {"nlhk", "IK_C"});
   auto df_nlhpi = df_nlhk.Filter([](const ROOT::VecOps::RVec<float> &nlhpi, int ipi){return nlhpi[ipi] > 0.1;},
                                  {"nlhpi", "IPI_C"});
   auto df_ipis = df_nlhpi.Define("IPIS_C", [](int ipis) {return ipis - 1;}, {"ipis"});
   auto df_nlhpi_ipis = df_ipis.Filter(
      [](const ROOT::VecOps::RVec<float> &nlhpi, int ipis){return nlhpi[ipis] > 0.1;},
      {"nlhpi", "IPIS_C"});
   auto df_njets = df_nlhpi_ipis.Filter([](int njets){return njets >= 1;}, {"njets"});

   auto hdmd = df_njets.Histo1D({"hdmd", "dm_d", 40, 0.13, 0.17}, "dm_d");
   auto df_ptD0 = df_njets.Define("ptD0", [](float rpd0_t, float ptd0_d){return rpd0_t / 0.029979 * 1.8646 / ptd0_d;},
                                  {"rpd0_t", "ptd0_d"});
   auto h2 = df_ptD0.Histo2D({"h2", "ptD0 vs dm_d", 30, 0.135, 0.165, 30, -3, 6}, "dm_d", "ptD0");

   *hdmd;
   *h2;
   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_show)
      Show(hdmd.GetPtr(), h2.GetPtr());
}


static void NTupleRdf(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();
   std::chrono::steady_clock::time_point ts_first;
   bool ts_first_set = false;

   auto df = ROOT::Experimental::MakeNTupleDataFrame("h42", path);
   auto df_timing = df.Define("TIMING", [&ts_first, &ts_first_set]() {
      if (!ts_first_set)
         ts_first = std::chrono::steady_clock::now();
      ts_first_set = true;
      return ts_first_set;}).Filter([](bool b){ return b; }, {"TIMING"});

   auto df_md0_d = df_timing.Filter([](float md0_d) {return TMath::Abs(md0_d - 1.8646) < 0.04;}, {"event.md0_d"});
   auto df_ptds_d = df_md0_d.Filter([](float ptds_d) {return ptds_d > 2.5;}, {"ptds_d"});
   auto df_etads_d = df_ptds_d.Filter([](float etads_d) {return etads_d < 1.5;}, {"etads_d"});

   auto df_ikipi = df_etads_d.Define("IK_C", [](int ik) {return ik - 1;}, {"ik"})
                             .Define("IPI_C", [](int ipi) {return ipi - 1;}, {"ipi"});
   auto df_nhitrp = df_ikipi.Filter([](const ROOT::VecOps::RVec<int> &nhitrp, int ik, int ipi) {
      return nhitrp[ik] * nhitrp[ipi] > 1;}, {"nhitrp", "IK_C", "IPI_C"});
   auto df_r = df_nhitrp.Filter(
      [](const ROOT::VecOps::RVec<float> &rend, const ROOT::VecOps::RVec<float> &rstart, int ik, int ipi)
         {return ((rend[ik] - rstart[ik]) > 22) && ((rend[ipi] - rstart[ipi]) > 22);},
         {"rend", "rstart", "IK_C", "IPI_C"});
   auto df_nlhk = df_r.Filter([](const ROOT::VecOps::RVec<float> &nlhk, int ik){return nlhk[ik] > 0.1;},
                              {"nlhk", "IK_C"});
   auto df_nlhpi = df_nlhk.Filter([](const ROOT::VecOps::RVec<float> &nlhpi, int ipi){return nlhpi[ipi] > 0.1;},
                                  {"nlhpi", "IPI_C"});
   auto df_ipis = df_nlhpi.Define("IPIS_C", [](int ipis) {return ipis - 1;}, {"ipis"});
   auto df_nlhpi_ipis = df_ipis.Filter(
      [](const ROOT::VecOps::RVec<float> &nlhpi, int ipis){return nlhpi[ipis] > 0.1;},
      {"nlhpi", "IPIS_C"});
   auto df_njets = df_nlhpi_ipis.Filter([](int njets){return njets >= 1;}, {"njets"});

   auto hdmd = df_njets.Histo1D({"hdmd", "dm_d", 40, 0.13, 0.17}, "dm_d");
   auto df_ptD0 = df_njets.Define("ptD0", [](float rpd0_t, float ptd0_d){return rpd0_t / 0.029979 * 1.8646 / ptd0_d;},
                                  {"rpd0_t", "ptd0_d"});
   auto h2 = df_ptD0.Histo2D({"h2", "ptD0 vs dm_d", 30, 0.135, 0.165, 30, -3, 6}, "dm_d", "ptD0");

   *hdmd;
   *h2;
   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_show)
      Show(hdmd.GetPtr(), h2.GetPtr());
}




static void Usage(const char *progname) {
  printf("%s [-i input.root/ntuple] [-r(df)] [-m(t)] [-p(erformance stats)] [-x cluster bunch size]\n"
         "   [-s(show)] [-m(t)]\n", progname);
}

int main(int argc, char **argv) {
   auto ts_init = std::chrono::steady_clock::now();

   bool use_rdf = false;
   std::string path;
   int c;
   while ((c = getopt(argc, argv, "hvpsri:mx:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'p':
         g_perf_stats = true;
         break;
      case 's':
         g_show = true;
         break;
      case 'i':
         path = optarg;
         break;
      case 'r':
         use_rdf = true;
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
      if (use_rdf)
         TreeRdf(path);
      else
         TreeDirect(path);
      break;
   case FileFormats::kNtuple:
      if (use_rdf)
         NTupleRdf(path);
      else
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
