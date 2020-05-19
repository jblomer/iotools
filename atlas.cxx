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
#include <TF1.h>
#include <TFile.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLorentzVector.h>
#include <TMath.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreePerfStats.h>

#include <Math/Vector4D.h>

#include "util.h"

bool g_perf_stats = false;
bool g_show = false;

static ROOT::Experimental::RNTupleReadOptions GetRNTupleOptions() {
   using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;

   RNTupleReadOptions options;
   options.SetClusterCache(RNTupleReadOptions::kOn);
   std::cout << "{Using async cluster pool}" << std::endl;
   return options;
}


static void Show(TH1D *data, TH1D *ggH, TH1D *VBF) {
   new TApplication("", nullptr, nullptr);

   gROOT->SetStyle("ATLAS");
   auto c = new TCanvas("c", "", 700, 750);
   auto upper_pad = new TPad("upper_pad", "", 0, 0.35, 1, 1);
   auto lower_pad = new TPad("upper_pad", "", 0, 0, 1, 0.35);
   upper_pad->SetLeftMargin(0.14);
   upper_pad->SetRightMargin(0.05);
   upper_pad->SetTickx(false);
   upper_pad->SetTicky(false);
   lower_pad->SetLeftMargin(0.14);
   lower_pad->SetRightMargin(0.05);
   lower_pad->SetTickx(false);
   lower_pad->SetTicky(false);

   upper_pad->SetBottomMargin(0);
   lower_pad->SetTopMargin(0);
   lower_pad->SetBottomMargin(0.3);

   upper_pad->Draw();
   lower_pad->Draw();

   // Fit signal + background model to data
   upper_pad->cd();
   auto fit = new TF1("fit", "([0]+[1]*x+[2]*x^2+[3]*x^3)+[4]*exp(-0.5*((x-[5])/[6])^2)", 105, 160);
   fit->FixParameter(5, 125.0);
   fit->FixParameter(4, 119.1);
   fit->FixParameter(6, 2.39);
   fit->SetLineColor(2);
   fit->SetLineStyle(1);
   fit->SetLineWidth(2);
   data->Fit("fit", "", "E SAME", 105, 160);
   fit->Draw("SAME");

   // Draw background
   auto bkg = new TF1("bkg", "([0]+[1]*x+[2]*x^2+[3]*x^3)", 105, 160);
   for (int i = 0; i < 4; ++i)
      bkg->SetParameter(i, fit->GetParameter(i));;
   bkg->SetLineColor(4);
   bkg->SetLineStyle(2);
   bkg->SetLineWidth(2);
   bkg->Draw("SAME");

   // Draw data
   data->SetMarkerStyle(20);
   data->SetMarkerSize(1.2);
   data->SetLineWidth(2);
   data->SetLineColor(kBlack);
   data->Draw("E SAME");
   data->SetMinimum(1e-3);
   data->SetMaximum(8e3);
   data->GetYaxis()->SetLabelSize(0.045);
   data->GetYaxis()->SetTitleSize(0.05);
   data->SetStats(0);
   data->SetTitle("");

   // Scale simulated events with luminosity * cross-section / sum of weights
   // and merge to single Higgs signal
   auto lumi = 10064.0;
   ggH->Scale(lumi * 0.102 / 55922617.6297);
   VBF->Scale(lumi * 0.008518764 / 3441426.13711);
   auto higgs = reinterpret_cast<TH1D *>(ggH->Clone());
   higgs->Add(VBF);
   higgs->Draw("HIST SAME");

   // Draw ratio
   lower_pad->cd();

   auto ratiobkg = new TF1("zero", "0", 105, 160);
   ratiobkg->SetLineColor(4);
   ratiobkg->SetLineStyle(2);
   ratiobkg->SetLineWidth(2);
   ratiobkg->SetMinimum(-125);
   ratiobkg->SetMaximum(250);
   ratiobkg->GetXaxis()->SetLabelSize(0.08);
   ratiobkg->GetXaxis()->SetTitleSize(0.12);
   ratiobkg->GetXaxis()->SetTitleOffset(1.0);
   ratiobkg->GetYaxis()->SetLabelSize(0.08);
   ratiobkg->GetYaxis()->SetTitleSize(0.09);
   ratiobkg->GetYaxis()->SetTitle("Data - Bkg.");
   ratiobkg->GetYaxis()->CenterTitle();
   ratiobkg->GetYaxis()->SetTitleOffset(0.7);
   ratiobkg->GetYaxis()->SetNdivisions(503, false);
   ratiobkg->GetYaxis()->ChangeLabel(-1, -1, 0);
   ratiobkg->GetXaxis()->SetTitle("m_{#gamma#gamma} [GeV]");
   ratiobkg->Draw();

   auto ratiosig = new TH1D("ratiosig", "ratiosig", 5500, 105, 160);
   ratiosig->Eval(fit);
   ratiosig->SetLineColor(2);
   ratiosig->SetLineStyle(1);
   ratiosig->SetLineWidth(2);
   ratiosig->Add(bkg, -1);
   ratiosig->Draw("SAME");

   auto ratiodata = reinterpret_cast<TH1F *>(data->Clone());
   ratiodata->Add(bkg, -1);
   ratiodata->Draw("E SAME");
   for (int i = 1; i < data->GetNbinsX(); ++i)
      ratiodata->SetBinError(i, data->GetBinError(i));

   // Add legend
   upper_pad->cd();
   auto legend = new TLegend(0.55, 0.55, 0.89, 0.85);
   legend->SetTextFont(42);
   legend->SetFillStyle(0);
   legend->SetBorderSize(0);
   legend->SetTextSize(0.05);
   legend->SetTextAlign(32);
   legend->AddEntry(data, "Data" ,"lep");
   legend->AddEntry(bkg, "Background", "l");
   legend->AddEntry(fit, "Signal + Bkg.", "l");
   legend->AddEntry(higgs, "Signal", "l");
   legend->Draw("SAME");

   // Add ATLAS label
   auto text = new TLatex();
   text->SetNDC();
   text->SetTextFont(72);
   text->SetTextSize(0.05);
   text->DrawLatex(0.18, 0.84, "ATLAS");
   text->SetTextFont(42);
   text->DrawLatex(0.18 + 0.13, 0.84, "Open Data");
   text->SetTextSize(0.04);
   text->DrawLatex(0.18, 0.78, "#sqrt{s} = 13 TeV, 10 fb^{-1}");

   c->Modified();

   std::cout << "press ENTER to exit..." << std::endl;
   auto future = std::async(std::launch::async, getchar);
   while (true) {
      gSystem->ProcessEvents();
      if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
         break;
   }
}


static float ComputeInvariantMass(
   float pt0, float pt1, float eta0, float eta1, float phi0, float phi1, float e0, float e1)
{
    ROOT::Math::PtEtaPhiEVector p1(pt0, eta0, phi0, e0);
    ROOT::Math::PtEtaPhiEVector p2(pt1, eta1, phi1, e1);
    return (p1 + p2).mass() / 1000.0;
}


static void ProcessNTuple(ROOT::Experimental::RNTupleReader *ntuple, TH1D *hMass, bool isMC,
                          unsigned *runtime_init, unsigned *runtime_analyze)
{
   auto ts_init = std::chrono::steady_clock::now();

   auto viewTrigP           = ntuple->GetView<bool>("trigP");
   auto viewPhotonN         = ntuple->GetView<std::uint32_t>("photon_n");
   auto viewPhotonIsTightId = ntuple->GetView<std::vector<bool>>("photon_isTightID");
   auto viewPhotonPt        = ntuple->GetView<std::vector<float>>("photon_pt");
   auto viewPhotonEta       = ntuple->GetView<std::vector<float>>("photon_eta");
   auto viewPhotonPhi       = ntuple->GetView<std::vector<float>>("photon_phi");
   auto viewPhotonE         = ntuple->GetView<std::vector<float>>("photon_E");
   auto viewPhotonPtCone30  = ntuple->GetView<std::vector<float>>("photon_ptcone30");
   auto viewPhotonEtCone20  = ntuple->GetView<std::vector<float>>("photon_etcone20");

   auto viewScaleFactorPhoton        = ntuple->GetView<float>("scaleFactor_PHOTON");
   auto viewScaleFactorPhotonTrigger = ntuple->GetView<float>("scaleFactor_PhotonTRIGGER");
   auto viewScaleFactorPileUp        = ntuple->GetView<float>("scaleFactor_PILEUP");
   auto viewMcWeight                 = ntuple->GetView<float>("mcWeight");

   unsigned nevents = 0;
   std::chrono::steady_clock::time_point ts_first;
   for (auto e : ntuple->GetEntryRange()) {
      nevents++;
      if ((nevents % 100000) == 0) {
         printf("processed %u k events\n", nevents / 1000);
         //printf("dummy is %lf\n", dummy); abort();
      }
      if (nevents == 1) {
         ts_first = std::chrono::steady_clock::now();
      }

      if (!viewTrigP(e)) continue;

      std::vector<size_t> idxGood;
      auto isTightId = viewPhotonIsTightId(e);
      auto pt = viewPhotonPt(e);
      auto eta = viewPhotonEta(e);

      for (size_t i = 0; i < viewPhotonN(e); ++i) {
         if (!isTightId[i]) continue;
         if (pt[i] <= 25000.) continue;
         if (abs(eta[i]) >= 2.37) continue;
         if (abs(eta[i]) >= 1.37 && abs(eta[i]) <= 1.52) continue;
         idxGood.push_back(i);
      }
      if (idxGood.size() != 2) continue;

      auto ptCone30 = viewPhotonPtCone30(e);
      auto etCone20 = viewPhotonEtCone20(e);

      bool isIsolatedPhotons = true;
      for (int i = 0; i < 2; ++i) {
         if ((ptCone30[idxGood[i]] / pt[idxGood[i]] >= 0.065) ||
             (etCone20[idxGood[i]] / pt[idxGood[i]] >= 0.065))
         {
           isIsolatedPhotons = false;
           break;
         }
      }
      if (!isIsolatedPhotons) continue;

      auto phi = viewPhotonPhi(e);
      auto E = viewPhotonE(e);

      float myy = ComputeInvariantMass(
         pt[idxGood[0]], pt[idxGood[1]],
         eta[idxGood[0]], eta[idxGood[1]],
         phi[idxGood[0]], phi[idxGood[1]],
         E[idxGood[0]], E[idxGood[1]]);

      if (pt[idxGood[0]] / 1000. / myy <= 0.35) continue;
      if (pt[idxGood[1]] / 1000. / myy <= 0.25) continue;
      if (myy <= 105) continue;
      if (myy >= 160) continue;

      if (isMC) {
         auto weight = viewScaleFactorPhoton(e) * viewScaleFactorPhotonTrigger(e) *
                       viewScaleFactorPileUp(e) * viewMcWeight(e);
         hMass->Fill(myy, weight);
      } else {
         hMass->Fill(myy);
      }

   }

   auto ts_end = std::chrono::steady_clock::now();
   *runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   *runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();
}


static void NTupleDirect(const std::string &pathData, const std::string &path_ggH, const std::string &pathVBF)
{
   using RNTupleReader = ROOT::Experimental::RNTupleReader;

   unsigned int runtime_init;
   unsigned int runtime_analyze;
   auto options = GetRNTupleOptions();

   auto hData = new TH1D("", "Diphoton invariant mass; m_{#gamma#gamma} [GeV];Events", 30, 105, 160);
   auto hggH = new TH1D("", "Diphoton invariant mass; m_{#gamma#gamma} [GeV];Events", 30, 105, 160);
   auto hVBF = new TH1D("", "Diphoton invariant mass; m_{#gamma#gamma} [GeV];Events", 30, 105, 160);

   auto ntuple = RNTupleReader::Open("mini", pathData, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();
   ProcessNTuple(ntuple.get(), hData, false /* isMC */, &runtime_init, &runtime_analyze);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ntuple->PrintInfo(ROOT::Experimental::ENTupleInfo::kMetrics);


   ntuple = RNTupleReader::Open("mini", path_ggH, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();
   ProcessNTuple(ntuple.get(), hggH, true /* isMC */, &runtime_init, &runtime_analyze);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ntuple->PrintInfo(ROOT::Experimental::ENTupleInfo::kMetrics);

   ntuple = RNTupleReader::Open("mini", pathVBF, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();
   ProcessNTuple(ntuple.get(), hVBF, true /* isMC */, &runtime_init, &runtime_analyze);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ntuple->PrintInfo(ROOT::Experimental::ENTupleInfo::kMetrics);

   if (g_show)
      Show(hData, hggH, hVBF);

   delete hVBF;
   delete hggH;
   delete hData;
}


static void ProcessTree(TTree *tree, TH1D *hMass, bool isMC,
                        unsigned *runtime_init, unsigned *runtime_analyze)
{
   auto ts_init = std::chrono::steady_clock::now();

   TBranch *brTrigP                    = nullptr;
   TBranch *brPhotonN                  = nullptr;
   TBranch *brPhotonIsTightId          = nullptr;
   TBranch *brPhotonPt                 = nullptr;
   TBranch *brPhotonEta                = nullptr;
   TBranch *brPhotonPhi                = nullptr;
   TBranch *brPhotonE                  = nullptr;
   TBranch *brPhotonPtCone30           = nullptr;
   TBranch *brPhotonEtCone20           = nullptr;
   TBranch *brScaleFactorPhoton        = nullptr;
   TBranch *brScaleFactorPhotonTrigger = nullptr;
   TBranch *brScaleFactorPileUp        = nullptr;
   TBranch *brMcWeight                 = nullptr;

   bool trigP;
   unsigned int photon_n;
   std::vector<bool> *photon_isTightID = nullptr;
   std::vector<float> *photon_pt = nullptr;
   std::vector<float> *photon_eta = nullptr;
   std::vector<float> *photon_phi = nullptr;
   std::vector<float> *photon_E = nullptr;
   std::vector<float> *photon_ptcone30 = nullptr;
   std::vector<float> *photon_etcone20 = nullptr;
   float scaleFactor_PHOTON;
   float scaleFactor_PhotonTRIGGER;
   float scaleFactor_PILEUP;
   float mcWeight;

   tree->SetBranchAddress("trigP", &trigP, &brTrigP);
   tree->SetBranchAddress("photon_n", &photon_n, &brPhotonN);
   tree->SetBranchAddress("photon_isTightID", &photon_isTightID, &brPhotonIsTightId);
   tree->SetBranchAddress("photon_pt", &photon_pt, &brPhotonPt);
   tree->SetBranchAddress("photon_eta", &photon_eta, &brPhotonEta);
   tree->SetBranchAddress("photon_phi", &photon_phi, &brPhotonPhi);
   tree->SetBranchAddress("photon_E", &photon_E, &brPhotonE);
   tree->SetBranchAddress("photon_ptcone30", &photon_ptcone30, &brPhotonPtCone30);
   tree->SetBranchAddress("photon_etcone20", &photon_etcone20, &brPhotonEtCone20);
   tree->SetBranchAddress("scaleFactor_PHOTON", &scaleFactor_PHOTON, &brScaleFactorPhoton);
   tree->SetBranchAddress("scaleFactor_PhotonTRIGGER", &scaleFactor_PhotonTRIGGER, &brScaleFactorPhotonTrigger);
   tree->SetBranchAddress("scaleFactor_PILEUP", &scaleFactor_PILEUP, &brScaleFactorPileUp);
   tree->SetBranchAddress("mcWeight", &mcWeight, &brMcWeight);

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

      tree->LoadTree(entryId);

      brTrigP->GetEntry(entryId);
      if (!brTrigP) continue;

      std::vector<size_t> idxGood;
      brPhotonN->GetEntry(entryId);
      brPhotonIsTightId->GetEntry(entryId);
      brPhotonPt->GetEntry(entryId);
      brPhotonEta->GetEntry(entryId);
      for (size_t i = 0; i < photon_n; ++i) {
         brPhotonIsTightId->GetEntry(entryId);
         if (!(*photon_isTightID)[i]) continue;
         if ((*photon_pt)[i] <= 25000.) continue;
         if (abs((*photon_eta)[i]) >= 2.37) continue;
         if (abs((*photon_eta)[i]) >= 1.37 && abs((*photon_eta)[i]) <= 1.52) continue;
         idxGood.push_back(i);
      }
      if (idxGood.size() != 2) continue;

      brPhotonPtCone30->GetEntry(entryId);
      brPhotonEtCone20->GetEntry(entryId);

      bool isIsolatedPhotons = true;
      for (int i = 0; i < 2; ++i) {
         if (((*photon_ptcone30)[idxGood[i]] / (*photon_pt)[idxGood[i]] >= 0.065) ||
             ((*photon_etcone20)[idxGood[i]] / (*photon_pt)[idxGood[i]] >= 0.065))
         {
           isIsolatedPhotons = false;
           break;
         }
      }
      if (!isIsolatedPhotons) continue;

      brPhotonPhi->GetEntry(entryId);
      brPhotonE->GetEntry(entryId);

      float myy = ComputeInvariantMass(
         (*photon_pt)[idxGood[0]],  (*photon_pt)[idxGood[1]],
         (*photon_eta)[idxGood[0]], (*photon_eta)[idxGood[1]],
         (*photon_phi)[idxGood[0]], (*photon_phi)[idxGood[1]],
         (*photon_E)[idxGood[0]],   (*photon_E)[idxGood[1]]);

      if ((*photon_pt)[idxGood[0]] / 1000. / myy <= 0.35) continue;
      if ((*photon_pt)[idxGood[1]] / 1000. / myy <= 0.25) continue;
      if (myy <= 105) continue;
      if (myy >= 160) continue;

      if (isMC) {
         brScaleFactorPhoton->GetEntry(entryId);
         brScaleFactorPhotonTrigger->GetEntry(entryId);
         brScaleFactorPileUp->GetEntry(entryId);
         brMcWeight->GetEntry(entryId);
         auto weight = scaleFactor_PHOTON * scaleFactor_PhotonTRIGGER *
                       scaleFactor_PILEUP * mcWeight;
         hMass->Fill(myy, weight);
      } else {
         hMass->Fill(myy);
      }

   }

   auto ts_end = std::chrono::steady_clock::now();
   *runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   *runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();
}


static void TreeDirect(const std::string &pathData, const std::string &path_ggH, const std::string &pathVBF)
{
   unsigned int runtime_init;
   unsigned int runtime_analyze;

   auto hData = new TH1D("", "Diphoton invariant mass; m_{#gamma#gamma} [GeV];Events", 30, 105, 160);
   auto hggH = new TH1D("", "Diphoton invariant mass; m_{#gamma#gamma} [GeV];Events", 30, 105, 160);
   auto hVBF = new TH1D("", "Diphoton invariant mass; m_{#gamma#gamma} [GeV];Events", 30, 105, 160);

   auto file = TFile::Open(pathData.c_str());
   auto tree = file->Get<TTree>("mini");
   TTreePerfStats *ps = nullptr;
   if (g_perf_stats)
      ps = new TTreePerfStats("ioperf", tree);
   ProcessTree(tree, hData, false /* isMC */, &runtime_init, &runtime_analyze);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ps->Print();


   file = TFile::Open(path_ggH.c_str());
   tree = file->Get<TTree>("mini");
   if (g_perf_stats)
      ps = new TTreePerfStats("ioperf", tree);
   ProcessTree(tree, hggH, true /* isMC */, &runtime_init, &runtime_analyze);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ps->Print();

   file = TFile::Open(pathVBF.c_str());
   tree = file->Get<TTree>("mini");
   if (g_perf_stats)
      ps = new TTreePerfStats("ioperf", tree);
   ProcessTree(tree, hVBF, true /* isMC */, &runtime_init, &runtime_analyze);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
   if (g_perf_stats)
      ps->Print();

   if (g_show)
      Show(hData, hggH, hVBF);

   delete hVBF;
   delete hggH;
   delete hData;
}


static void Usage(const char *progname) {
  printf("%s [-i gg_data.root] [-r(df)] [-p(erformance stats)] [-s(show)]\n", progname);
}


int main(int argc, char **argv) {
   std::string input_path;
   std::string input_suffix;
   bool use_rdf = false;
   int c;
   while ((c = getopt(argc, argv, "hvi:rps")) != -1) {
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

   std::string suffix = GetSuffix(input_path);
   std::string compression = SplitString(StripSuffix(input_path), '~')[1];
   std::string ggH_path = GetParentPath(input_path) + "/gg_mc_ggH125~" + compression + "." + suffix;
   std::string vbf_path = GetParentPath(input_path) + "/gg_mc_VBFH125~" + compression + "." + suffix;

   switch (GetFileFormat(suffix)) {
   case FileFormats::kRoot:
      if (use_rdf) {
         //ROOT::RDataFrame df("DecayTree", input_path);
         //Dataframe(df, 1);
      } else {
         TreeDirect(input_path, ggH_path, vbf_path);
      }
      break;
   case FileFormats::kNtuple:
      if (use_rdf) {
         //using RNTupleDS = ROOT::Experimental::RNTupleDS;
         //auto options = GetRNTupleOptions();
         //auto pageSource = ROOT::Experimental::Detail::RPageSource::Create("DecayTree", input_path, options);
         //ROOT::RDataFrame df(std::make_unique<RNTupleDS>(std::move(pageSource)));
         //Dataframe(df, 1);
      } else {
         NTupleDirect(input_path, ggH_path, vbf_path);
      }
      break;
   default:
      std::cerr << "Invalid file format: " << suffix << std::endl;
      return 1;
   }

   return 0;
}
