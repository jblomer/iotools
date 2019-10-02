#include <ROOT/RNTuple.hxx>
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

static void Show(TH1F *hdmd, TH2F *h2) {
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

static void TreeOptimized(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();

   auto file = TFile::Open(path.c_str());
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

   auto hdmd = new TH1F("hdmd", "dm_d", 40, 0.13, 0.17);
   auto h2   = new TH2F("h2", "ptD0 vs dm_d", 30, 0.135, 0.165, 30, -3, 6);

   auto nEntries = tree->GetEntries();
   std::chrono::steady_clock::time_point ts_first;
   for (decltype(nEntries) entryId = 0; entryId < nEntries; ++entryId) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;
      if (entryId == 1) {
         ts_first = std::chrono::steady_clock::now();
      }

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


static void NTupleOptimized(const std::string &path) {
   using ENTupleInfo = ROOT::Experimental::ENTupleInfo;
   using RNTupleModel = ROOT::Experimental::RNTupleModel;
   using RNTupleReader = ROOT::Experimental::RNTupleReader;
   using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;

   auto ts_init = std::chrono::steady_clock::now();

   auto model = RNTupleModel::Create();
   RNTupleReadOptions options;
   options.SetClusterCache(RNTupleReadOptions::EClusterCache::kOn);
   auto ntuple = RNTupleReader::Open(std::move(model), "h42", path, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();

   std::chrono::steady_clock::time_point ts_first;
   for (auto entryId : ntuple->GetViewRange()) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;
      if (entryId == 1) {
         ts_first = std::chrono::steady_clock::now();
      }
   }

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   if (g_perf_stats)
      ntuple->PrintInfo(ENTupleInfo::kMetrics);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
}


static void Usage(const char *progname) {
  printf("%s [-i input.root/ntuple] [-p(erformance stats)]\n", progname);
}

int main(int argc, char **argv) {
   std::string path;
   int c;
   while ((c = getopt(argc, argv, "hvpsi:")) != -1) {
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
