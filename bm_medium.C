R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

void bm_medium(TString dataSet="result_medium",
               TString title = "TITLE",
               TString output_path = "graph_UNKNOWN.root")
{
  std::ifstream file_timing(Form("%s.txt", dataSet.Data()));
  std::string sample;
  std::string method;
  std::string container;
  std::array<float, 6> timings;
  int max_streams = 0;

  std::map<std::string, int> orderMethod{{"sdd", 0}, {"hdd", 1}, {"http", 2}};
  std::map<std::string, int> orderSample{{"lhcb", 0}, {"cms", 1}, {"h1X10", 2}, {"empty", 3}};
  std::map<std::string, int> orderContainer{{"root", 0}, {"ntuple", 1}};
  auto nMethod = orderMethod.size();
  auto nSample = orderSample.size();
  auto nContainer = orderContainer.size();

  std::map<std::string, int> nEvents{{"lhcb", 8556118}, {"cms", 1620657}, {"h1X10", 2838130}};

  // method -> sample -> container -> (val, error)
  std::map<std::string, std::map<std::string, std::map<std::string, std::pair<float, float>>>> data;
  std::map<std::string, std::map<std::string, std::map<std::string, TGraphErrors *>>> graphs;
  // method -> sample -> graph
  std::map<std::string, std::map<std::string, TGraphErrors *>> gratios;

  float max_throughput = 0.0;
  while (file_timing >> sample >> method >> container >>
         timings[0] >> timings[1] >> timings[2] >>
         timings[3] >> timings[4] >> timings[5])
  {
    float mean;
    float error;
    GetStats(timings.data(), 6, mean, error);
    float n = nEvents[sample];
    auto throughput_val = n / mean;
    auto throughput_max = n / (mean - error);
    auto throughput_min = n / (mean + error);
    auto throughput_err = (throughput_max - throughput_min) / 2;

    auto g = new TGraphErrors();
    auto x = orderMethod[method] * nSample * nContainer + orderSample[sample] * nContainer
             + orderContainer[container];
    g->SetPoint(0, x + 1, -1);
    g->SetPoint(1, x + 1 + 1, throughput_val);
    g->SetPoint(2, x + 2 + 1, -1);
    g->SetPointError(1, 0, throughput_err);
    graphs[method][sample][container] = g;
    data[method][sample][container] = std::pair<float, float>(throughput_val, throughput_err);
    max_throughput = std::max(max_throughput, throughput_val + throughput_err);

    std::cout << sample << " " << method << " " << container << " " <<
      throughput_val << " +/- " << throughput_err << "  [@ " << x << "]" << std::endl;
  }

  float max_ratio = 0.0;
  for (const auto &methods : data) {
    for (const auto &samples : methods.second) {
      auto ntuple_val = samples.second.at("ntuple").first;
      auto ntuple_err = samples.second.at("ntuple").second;
      auto tree_val = samples.second.at("root").first;
      auto tree_err = samples.second.at("root").second;
      auto ratio_val = ntuple_val / tree_val;
      auto ratio_err = ratio_val *
        sqrt(tree_err * tree_err / tree_val / tree_val +
             ntuple_err * ntuple_err / ntuple_val / ntuple_val);

      auto g = new TGraphErrors();
      auto x = orderMethod[methods.first] * nSample + orderSample[samples.first];
      g->SetPoint(0, 2*x - 0.5 + 1, -1);
      g->SetPoint(1, 2*x + 1.5 + 1, ratio_val);
      g->SetPoint(2, 2*x + 3.5 + 1, -1);
      g->SetPointError(1, 0, ratio_err);
      gratios[methods.first][samples.first] = g;
      max_ratio = std::max(max_ratio, ratio_val + ratio_err);

      std::cout << "ntpl/tree: " << methods.first << " " << samples.first << " "
                << ratio_val << " +/- " << ratio_err << "  [@ " << x << "]" << std::endl;
    }
  }

  auto max_x = nMethod * nSample * nContainer + 1;

  SetStyle();  // Has to be at the beginning of painting
  TCanvas *canvas = new TCanvas("MyCanvas", "MyCanvas");
  canvas->cd();

  auto pad_throughput = new TPad("pad_throughput", "pad_throughput",
                                 0.0, 0.39, 1.0, 0.95);
  pad_throughput->SetTopMargin(0.08);
  pad_throughput->SetBottomMargin(0.03);
  pad_throughput->SetLeftMargin(0.1);
  pad_throughput->SetRightMargin(0.055);
  pad_throughput->Draw();
  canvas->cd();
  auto pad_ratio = new TPad("pad_ratio", "pad_ratio", 0.0, 0.030, 1.0, 0.38);
  pad_ratio->SetTopMargin(0.);
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
  helper->GetYaxis()->SetTitleSize(0.08);
  helper->GetYaxis()->SetTitleOffset(0.58);
  helper->SetMinimum(1);
  helper->SetMaximum(max_throughput * 1.5);
  helper->SetTitle("TITLE");

  TH1F *helper2 = new TH1F("", "", max_x, 0, max_x);
  helper2->SetMinimum(0);
  helper2->SetMaximum(max_ratio * 1.1);
  helper2->GetXaxis()->SetNdivisions(0);
  helper2->GetXaxis()->SetTickSize(0);
  helper2->GetXaxis()->SetLabelSize(0);
  helper2->GetXaxis()->SetTitleSize(0.12);
  helper2->GetYaxis()->SetTitle("RNtuple / TTree");
  helper2->GetYaxis()->SetNdivisions(8);
  helper2->GetYaxis()->SetLabelSize(0.11);
  helper2->GetYaxis()->SetTitleSize(0.12);
  helper2->GetYaxis()->SetTitleOffset(0.35);

  std::map<std::string, int> colors{{"root", kBlue}, {"ntuple", kRed}};
  std::map<std::string, int> styles{{"lhcb", 1001}, {"cms", 3001}, {"h1X10", 3008}};

  pad_throughput->cd();
  gPad->SetGridy();
  pad_throughput->SetLogy(1);
  helper->Draw();
  for (const auto &methods : graphs) {
    for (const auto &samples : methods.second) {
      for (const auto &container : samples.second) {
        auto g = container.second;
        g->SetLineColor(12);
        g->SetMarkerColor(12);
        g->SetFillColor(colors[container.first]);
        g->SetFillStyle(styles[samples.first]);
        g->SetLineWidth(2);
        g->Draw("B1");
        g->Draw("P");
      }
    }
  }


  pad_ratio->cd();
  gPad->SetGridy();
  helper2->Draw();
  for (const auto &methods : gratios) {
    for (const auto &samples : methods.second) {
      auto g = samples.second;
      g->SetLineColor(12);
      g->SetMarkerColor(12);
      g->SetFillColor(kGreen + 2);
      g->SetFillStyle(styles[samples.first]);
      g->SetLineWidth(2);
      g->Draw("B1");
      g->Draw("P");
    }
  }


//  auto output = TFile::Open(output_path, "RECREATE");
//  output->cd();
//  canvas->Write();
//  output->Close();
}
