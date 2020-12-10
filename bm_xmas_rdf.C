R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

void bm_xmas_rdf()
{
  std::string title = "RNTuple and TTree single-threaded read throughput from SSD with RDataFrame and compressed data";
  TString output_path = "graph_xmas.root";

  std::string container;
  std::string sample;

  // sample -> container -> timings
  std::map<std::string, std::map<std::string, std::array<float, 4>>> timings;
  timings["lhcb"]["ntuple"][0] = 4.375601;
  timings["lhcb"]["ntuple"][1] = 4.424526;
  timings["lhcb"]["ntuple"][2] = 4.579205;
  timings["lhcb"]["ntuple"][3] = 4.358658;
  timings["lhcb"]["root"][0] = 7.588285;
  timings["lhcb"]["root"][1] = 7.529001;
  timings["lhcb"]["root"][2] = 7.812029;
  timings["lhcb"]["root"][3] = 7.441627;
  timings["cms"]["ntuple"][0] = 1.526505;
  timings["cms"]["ntuple"][1] = 1.589383;
  timings["cms"]["ntuple"][2] = 1.544522;
  timings["cms"]["ntuple"][3] = 1.548531;
  timings["cms"]["root"][0] = 2.113191;
  timings["cms"]["root"][1] = 1.996466;
  timings["cms"]["root"][2] = 2.077008;
  timings["cms"]["root"][3] = 2.093517;
  timings["h1X10"]["ntuple"][0] = 1.882185;
  timings["h1X10"]["ntuple"][1] = 1.953293;
  timings["h1X10"]["ntuple"][2] = 1.973846;
  timings["h1X10"]["ntuple"][3] = 1.969042;
  timings["h1X10"]["root"][0] = 3.625057;
  timings["h1X10"]["root"][1] = 3.537451;
  timings["h1X10"]["root"][2] = 3.547826;
  timings["h1X10"]["root"][3] = 3.568688;

  // sample -> container -> MB/s disk throughput
  std::map<std::string, std::map<std::string, float>> readMbs;
  readMbs["lhcb"]["ntuple"] = 850;
  readMbs["lhcb"]["root"] = 500;
  readMbs["cms"]["ntuple"] = 410;
  readMbs["cms"]["root"] = 190;
  readMbs["h1X10"]["ntuple"] = 1620;
  readMbs["h1X10"]["root"] = 210;

  std::map<std::string, int> orderContainer{{"root", 0}, {"ntuple", 1}, {"", 2}};
  std::map<std::string, int> orderSample{{"lhcb", 0}, {"h1X10", 1}, {"cms", 2}};
  auto nContainer = orderContainer.size();
  auto nSample = orderSample.size();

  std::map<std::string, int> nEvents{{"lhcb", 8556118}, {"cms", 1620657}, {"h1X10", 2838130}};

  // sample -> container -> TGraphError *
  std::map<std::string, std::map<std::string, TGraphErrors *>> graphs;
  std::map<std::string, std::map<std::string, std::pair<float, float>>> data;
  // sample --> TGraphError *
  std::map<std::string, TGraphErrors *> gratios;

  // sample --> Evs
  std::map<std::string, std::pair<float, float>> ntupleEvs;
  std::map<std::string, std::pair<float, float>> treeEvs;

  float max_throughput = 0.0;
  for (const auto &s : timings) {
    auto sample = s.first;
    for (const auto &c : s.second) {
      auto container = c.first;

      float mean;
      float error;
      GetStats(c.second.data(), c.second.size(), mean, error);
      float n = nEvents[sample];
      auto throughput_val = n / mean;
      auto throughput_max = n / (mean - error);
      auto throughput_min = n / (mean + error);
      auto throughput_err = (throughput_max - throughput_min) / 2;
      max_throughput = std::max(max_throughput, throughput_val + throughput_err);
      data[sample][container] = std::pair<float, float>(throughput_val, throughput_err);

      auto g = new TGraphErrors();
      int x;
      x = orderSample[sample] * nContainer + orderContainer[container];
      g->SetPoint(0, x + 1 + 0, throughput_val);
      g->SetPoint(1, x + 1 + 1.5, -1);
      g->SetPointError(0, 0, throughput_err);
      graphs[sample][container] = g;

      std::cout << sample << " " << container << " " <<
        throughput_val << " +/- " << throughput_err << "  [@ " << x << "]" << std::endl;
    }
  }

  float max_ratio = 0.0;
  for (const auto &samples : data) {
    auto sample = samples.first;

    auto ntuple_val = samples.second.at("ntuple").first;
    auto ntuple_err = samples.second.at("ntuple").second;
    auto tree_val = samples.second.at("root").first;
    auto tree_err = samples.second.at("root").second;
    auto ratio_val = ntuple_val / tree_val;
    auto ratio_err = ratio_val *
      sqrt(tree_err * tree_err / tree_val / tree_val +
           ntuple_err * ntuple_err / ntuple_val / ntuple_val);

    auto g = new TGraphErrors();
    auto x = orderSample[sample];
    g->SetPoint(0, nContainer * x - 1.5 + 1.5, -1);
    g->SetPoint(1, nContainer * x + 0 + 1.5, ratio_val);
    g->SetPoint(2, nContainer * x + 1.5 + 1.5, -1);
    g->SetPointError(1, 0, ratio_err);
    gratios[sample] = g;
    max_ratio = std::max(max_ratio, ratio_val + ratio_err);

    std::cout << "ntpl/tree: " << sample << " "
              << ratio_val << " +/- " << ratio_err << "  [@ " << x << "]" << std::endl;
  }

  int max_x = nSample * nContainer;

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
  helper->GetYaxis()->SetTitle("Throughput (Events/s)");
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
  int style = 1001;

  pad_throughput->cd();
  gPad->SetGridy();
  //gPad->SetFillColor(GetTransparentColor());
  helper->Draw();

  std::vector<std::string> plot_samples{"lhcb", "h1X10", "cms"};
  std::vector<std::string> plot_containers{"root", "ntuple"};

  for (const auto &s : plot_samples) {
    for (const auto &c : plot_containers) {
      auto g = graphs[s][c];
      g->SetLineColor(12);
      g->SetMarkerColor(12);
      g->SetFillColor(colors[c]);
      g->SetFillStyle(style);
      g->SetLineWidth(1);
      g->Draw("B1");
      g->Draw("P");

      double x, y, err;
      g->GetPoint(0, x, y);
      err = g->GetErrorY(0);
      auto mbs_val = readMbs[s][c];
      std::ostringstream val;
      val.precision(0);
      val << "Disk I/O: " << std::fixed << mbs_val << " MB/s";

      TLatex tval;
      tval.SetTextSize(0.04);
      tval.SetTextAlign(21);
      tval.SetTextColor(colors[c]);
      tval.DrawLatex(x, (y + err) * 1.05, val.str().c_str());
    }
  }

  for (unsigned i = 3; i < 8; i += 3) {
    TLine *line = new TLine(i, 0, i, max_throughput);
    line->SetLineColor(kBlack);
    line->SetLineStyle(2);
    line->SetLineWidth(1);
    line->Draw();
  }

  TLegend *leg;
  leg = new TLegend(0.75, 0.69, 0.90, 0.88);
  leg->AddEntry(graphs["lhcb"]["root"],   "TTree",   "f");
  leg->AddEntry(graphs["lhcb"]["ntuple"], "RNTuple", "f");
  leg->SetBorderSize(1);
  leg->SetTextSize(0.05);
  leg->Draw();
  TText l;
  l.SetTextSize(0.04);
  l.SetTextAlign(13);
  l.DrawTextNDC(0.75, 0.69 - 0.0075, "95% CL");

  pad_ratio->cd();
  gPad->SetGridy();
  helper2->Draw();

  for (const auto &s : plot_samples) {
    auto g = gratios[s];
    g->SetLineColor(12);
    g->SetMarkerColor(12);
    g->SetFillColor(kGreen + 2);
    g->SetFillStyle(style);
    g->SetLineWidth(1);
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
    tval.DrawLatex(x, y * 0.9, val.str().c_str());
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
    line->SetLineWidth(1);
    line->Draw();
  }

  TText lLhcb;
  lLhcb.SetTextSize(0.07);
  lLhcb.SetTextAlign(23);
  lLhcb.DrawText(1.5, -0.5, "LHCb (zstd)");
  TText lH1;
  lH1.SetTextSize(0.07);
  lH1.SetTextAlign(23);
  lH1.DrawText(4.5, -0.5, "H1 (zstd)");
  TText lCms;
  lCms.SetTextSize(0.07);
  lCms.SetTextAlign(23);
  lCms.DrawText(7.5, -0.5, "CMS (lzma)");

  auto output = TFile::Open(output_path, "RECREATE");
  output->cd();
  canvas->Write();
  std::string pdf_path = output_path.View().to_string();
  canvas->Print(TString(pdf_path.substr(0, pdf_path.length() - 4) + "pdf"));
  output->Close();
}
