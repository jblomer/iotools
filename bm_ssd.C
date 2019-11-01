R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

void bm_ssd(TString dataSet="result_ssd",
            std::string title = "Read throuput from SSD TTree vs. RNTuple (zstd compressed)",
            TString output_path = "graph_ssd.root",
            bool only_direct = false)
{
  std::ifstream file_timing(Form("%s.txt", dataSet.Data()));
  std::string container;
  std::string sample;
  std::string compression;
  std::array<float, 6> timings;

  std::map<std::string, int> orderContainer{{"root", 0}, {"ntuple", 1}, {"", 2}};
  std::map<std::string, int> orderCompression{{"none", 0}, {"zstd", 1}};
  std::map<std::string, int> orderSample{{"lhcb", 0}, {"h1X10", 1}, {"cms", 2}};
  auto nContainer = orderContainer.size();
  auto nSample = orderSample.size();
  auto nCompression = orderCompression.size();

  std::map<std::string, int> nEvents{{"lhcb", 8556118}, {"cms", 1620657}, {"h1X10", 2838130}};
  std::map<std::string, float> ntupleVolume{{"lhcb", 845660.}, {"cms", 18895.}, {"h1X10", 203348.}};
  std::map<std::string, float> treeVolume{{"lhcb", 768091.}, {"cms", 80072.}, {"h1X10", 210529.}};

  // sample -> compression -> container -> TGraphError *
  std::map<std::string, std::map<std::string, std::map<std::string, TGraphErrors *>>> graphs;
  std::map<std::string, std::map<std::string, std::map<std::string, std::pair<float, float>>>> data;
  // sample --> compression --> TGraphError *
  std::map<std::string, std::map<std::string, TGraphErrors *>> gratios;

  // sample --> compression --> Mbs
  std::map<std::string, std::map<std::string, std::pair<float, float>>> ntupleMbs;
  std::map<std::string, std::map<std::string, std::pair<float, float>>> treeMbs;

  float max_throughput = 0.0;
  while (file_timing >> sample >> container >> compression >>
         timings[0] >> timings[1] >> timings[2] >>
         timings[3] >> timings[4] >> timings[5])
  {
    if (compression == "none")
      continue;

    float mean;
    float error;
    GetStats(timings.data(), 6, mean, error);
    float n = nEvents[sample];
    auto throughput_val = n / mean;
    auto throughput_max = n / (mean - error);
    auto throughput_min = n / (mean + error);
    auto throughput_err = (throughput_max - throughput_min) / 2;
    max_throughput = std::max(max_throughput, throughput_val + throughput_err);
    data[sample][compression][container] = std::pair<float, float>(throughput_val, throughput_err);

    if (container == "ntuple") {
      auto mbs_val = (ntupleVolume[sample] / 1024) / mean;
      auto mbs_max = (ntupleVolume[sample] / 1024) / (mean - error);
      auto mbs_min = (ntupleVolume[sample] / 1024) / (mean + error);
      auto mbs_err = (mbs_max - mbs_min) / 2;
      ntupleMbs[sample][compression] = std::pair<float, float>(mbs_val, mbs_err);
    }
    if (container == "root") {
      auto mbs_val = (treeVolume[sample] / 1024) / mean;
      auto mbs_max = (treeVolume[sample] / 1024) / (mean - error);
      auto mbs_min = (treeVolume[sample] / 1024) / (mean + error);
      auto mbs_err = (mbs_max - mbs_min) / 2;
      treeMbs[sample][compression] = std::pair<float, float>(mbs_val, mbs_err);
    }

    auto g = new TGraphErrors();
    int x;
    x = orderSample[sample] * nContainer /* * nCompression*/ +
      //orderCompression[compression] * nContainer +
      orderContainer[container];
    g->SetPoint(0, x + 1 + 0, throughput_val);
    g->SetPoint(1, x + 1 + 1.5, -1);
    g->SetPointError(0, 0, throughput_err);
    graphs[sample][compression][container] = g;

    std::cout << sample << " " << container << " " << " " << compression << " " <<
      throughput_val << " +/- " << throughput_err << "  [@ " << x << "]" << std::endl;
  }

  float max_ratio = 0.0;
  for (const auto &samples : data) {
    for (const auto &compressions : samples.second) {
      auto ntuple_val = compressions.second.at("ntuple").first;
      auto ntuple_err = compressions.second.at("ntuple").second;
      auto tree_val = compressions.second.at("root").first;
      auto tree_err = compressions.second.at("root").second;
      auto ratio_val = ntuple_val / tree_val;
      auto ratio_err = ratio_val *
        sqrt(tree_err * tree_err / tree_val / tree_val +
             ntuple_err * ntuple_err / ntuple_val / ntuple_val);

      auto g = new TGraphErrors();
      auto x = orderSample[samples.first] /* * nCompression + orderCompression[copmressions.first] */;
      g->SetPoint(0, nContainer * x - 1.5 + 1.5, -1);
      g->SetPoint(1, nContainer * x + 0 + 1.5, ratio_val);
      g->SetPoint(2, nContainer * x + 1.5 + 1.5, -1);
      g->SetPointError(1, 0, ratio_err);
      gratios[samples.first][compressions.first] = g;
      max_ratio = std::max(max_ratio, ratio_val + ratio_err);

      std::cout << "ntpl/tree: " << samples.first << " " << compressions.first << " "
                << ratio_val << " +/- " << ratio_err << "  [@ " << x << "]" << std::endl;
    }
  }

  int max_x = nSample /* * nCompression */ * nContainer;

  SetStyle();  // Has to be at the beginning of painting
  TCanvas *canvas = new TCanvas("MyCanvas", "MyCanvas");
  canvas->SetCanvasSize(1600, 850);
  //canvas->SetFillColor(GetTransparentColor());
  canvas->cd();

  auto pad_throughput = new TPad("pad_throughput", "pad_throughput",
                                 0.0, 0.39, 1.0, 0.95);
  //pad_throughput->SetFillColor(GetTransparentColor());
  pad_throughput->SetTopMargin(0.08);
  pad_throughput->SetBottomMargin(0.03);
  pad_throughput->SetLeftMargin(0.1);
  pad_throughput->SetRightMargin(0.055);
  pad_throughput->Draw();
  canvas->cd();
  auto pad_ratio = new TPad("pad_ratio", "pad_ratio", 0.0, 0.030, 1.0, 0.38);
  //pad_ratio->SetFillColor(GetTransparentColor());
  pad_ratio->SetTopMargin(0.02);
  pad_ratio->SetBottomMargin(0.26);
  pad_ratio->SetLeftMargin(0.1);
  pad_ratio->SetRightMargin(0.055);
  pad_ratio->Draw();
  canvas->cd();

  TH1F * helper = new TH1F("", "", max_x, 0, max_x);
  helper->GetXaxis()->SetTitle("");
  helper->GetXaxis()->SetNdivisions(0);
  helper->GetXaxis()->SetLabelSize(0);
  helper->GetXaxis()->SetTickSize(0);
  helper->GetYaxis()->SetTitle("Events / s");
  helper->GetYaxis()->SetLabelSize(0.07);
  helper->GetYaxis()->SetTitleSize(0.07);
  helper->GetYaxis()->SetTitleOffset(0.58);
  helper->SetMinimum(0);
  max_throughput *= 1.1;
  helper->SetMaximum(max_throughput);
  helper->SetTitle(title.c_str());

  TH1F *helper2 = new TH1F("", "", max_x, 0, max_x);
  helper2->SetMinimum(0);
  max_ratio *= 1.1;
  helper2->SetMaximum(max_ratio);
  helper2->GetXaxis()->SetNdivisions(0);
  helper2->GetXaxis()->SetTickSize(0);
  helper2->GetXaxis()->SetLabelSize(0.16);
  helper2->GetXaxis()->SetTitleSize(0.12);
  helper2->GetYaxis()->SetTitle("RNTuple / TTree");
  helper2->GetYaxis()->SetTickSize(0.005);
  helper2->GetYaxis()->SetNdivisions(8);
  helper2->GetYaxis()->SetLabelSize(0.11);
  helper2->GetYaxis()->SetTitleSize(0.11);
  helper2->GetYaxis()->SetTitleOffset(0.35);

  std::map<std::string, int> colors{{"root", kBlue}, {"ntuple", kRed}};
  //std::map<std::string, int> styles{{"none", 1001}, {"zstd", 3001}};
  std::map<std::string, int> styles{{"none", 1001}, {"zstd", 1001}};

  pad_throughput->cd();
  gPad->SetGridy();
  //gPad->SetFillColor(GetTransparentColor());
  helper->Draw();

  std::vector<std::string> plot_samples{"lhcb", "h1X10", "cms"};
  std::vector<std::string> plot_compressions{/*"none",*/ "zstd"};
  std::vector<std::string> plot_containers{"root", "ntuple"};

  for (const auto &s : plot_samples) {
    for (const auto &z : plot_compressions) {
      for (const auto &c : plot_containers) {
        auto g = graphs[s][z][c];
        g->SetLineColor(12);
        g->SetMarkerColor(12);
        g->SetFillColor(colors[c]);
        g->SetFillStyle(styles[z]);
        g->SetLineWidth(2);
        g->Draw("B1");
        g->Draw("P");

        if (c == "ntuple") {
          double x, y, err;
          g->GetPoint(0, x, y);
          err = g->GetErrorY(0);
          auto mbs_val = ntupleMbs[s][z].first;
          auto mbs_err = ntupleMbs[s][z].second;
          std::ostringstream val;
          val.precision(0);
          val << std::fixed << mbs_val << " MB/s";
          //val << " #pm ";
          //val << std::fixed << mbs_err;

          TLatex tval;
          tval.SetTextSize(0.04);
          tval.SetTextAlign(21);
          tval.SetTextColor(colors[c]);
          tval.DrawLatex(x, (y + err) * 1.05, val.str().c_str());
        }

        if (c == "root") {
          double x, y, err;
          g->GetPoint(0, x, y);
          err = g->GetErrorY(0);
          auto mbs_val = treeMbs[s][z].first;
          auto mbs_err = treeMbs[s][z].second;
          std::ostringstream val;
          val.precision(0);
          val << std::fixed << mbs_val << " MB/s";
          //val << " #pm ";
          //val << std::fixed << mbs_err;

          TLatex tval;
          tval.SetTextSize(0.04);
          tval.SetTextAlign(21);
          tval.SetTextColor(colors[c]);
          tval.DrawLatex(x, (y + err) * 1.05, val.str().c_str());
        }
      }
    }
  }

  for (unsigned i = 3; i < 8; i += 3) {
    TLine *line = new TLine(i, 0, i, max_throughput);
    line->SetLineColor(kBlack);
    line->SetLineStyle(2);
    line->SetLineWidth(2);
    line->Draw();
  }

  TLegend *leg;
  leg = new TLegend(0.2, 0.69, 0.35, 0.88);
  leg->AddEntry(graphs["lhcb"]["zstd"]["root"],   "TTree",   "f");
  leg->AddEntry(graphs["lhcb"]["zstd"]["ntuple"], "RNTuple", "f");
  leg->SetBorderSize(1);
  leg->SetTextSize(0.05);
  leg->Draw();
  TText l;
  l.SetTextSize(0.04);
  l.SetTextAlign(13);
  l.DrawTextNDC(0.2, 0.69 - 0.0075, "95% CL");

  pad_ratio->cd();
  gPad->SetGridy();
  helper2->Draw();

  for (const auto &s : plot_samples) {
    for (const auto &z : plot_compressions) {
      auto g = gratios[s][z];
      g->SetLineColor(12);
      g->SetMarkerColor(12);
      g->SetFillColor(kGreen + 2);
      g->SetFillStyle(styles[z]);
      g->SetLineWidth(2);
      g->Draw("B");
      g->Draw("P");

      double x, y, err;
      g->GetPoint(1, x, y);
      err = g->GetErrorY(1);
      std::ostringstream val;
      val.precision(1);
      val << "#times" << std::fixed << y;
      val << " #pm ";
      val << std::fixed << err;

      TLatex tval;
      tval.SetTextSize(0.07);
      tval.SetTextAlign(23);
      tval.DrawLatex(x, y * 0.8, val.str().c_str());
    }
  }

  TLine *lineOne = new TLine(0, 1, max_x, 1);
  lineOne->SetLineColor(kRed);
  lineOne->SetLineStyle(0);
  lineOne->SetLineWidth(2);
  lineOne->Draw();


  for (unsigned i = 3; i < 8; i += 3) {
    TLine *line = new TLine(i, 0, i, max_ratio);
    line->SetLineColor(kBlack);
    line->SetLineStyle(2);
    line->SetLineWidth(2);
    line->Draw();
  }

  TText lLhcb;
  lLhcb.SetTextSize(0.07);
  lLhcb.SetTextAlign(23);
  lLhcb.DrawText(1.5, -0.5, "LHCb run 1 open data B2HHH");
  TText lH1;
  lH1.SetTextSize(0.07);
  lH1.SetTextAlign(23);
  lH1.DrawText(4.5, -0.5, "H1 micro DST");
  TText lCms;
  lCms.SetTextSize(0.07);
  lCms.SetTextAlign(23);
  lCms.DrawText(7.5, -0.5, "CMS nanoAOD TTJet 13TeV June 2019");

  auto output = TFile::Open(output_path, "RECREATE");
  output->cd();
  canvas->Write();
  std::string pdf_path = output_path.View().to_string();
  canvas->Print(TString(pdf_path.substr(0, pdf_path.length() - 4) + "pdf"));
  output->Close();
}
