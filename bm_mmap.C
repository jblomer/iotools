R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

void bm_mmap(TString dataSet="result_mmap",
             TString title = "TITLE",
             TString output_path = "graph_UNKNOWN.root")
{
  std::ifstream file_timing(Form("%s.txt", dataSet.Data()));
  std::string medium;
  std::string method;
  std::string sample;
  std::array<float, 6> timings;

  std::map<std::string, int> orderMedium{{"mem", 0}, {"optane", 1}, {"ssd", 2}};
  std::map<std::string, int> orderSample{{"lhcb", 0}, {"cms", 1}, {"h1X10", 2}, {"empty", 3}};
  std::map<std::string, int> orderMethod{{"direct", 0}, {"mmap", 1}};
  auto nMedium = orderMedium.size();
  auto nSample = orderSample.size();
  auto nMethod = orderMethod.size();

  std::map<std::string, int> nEvents{{"lhcb", 8556118}, {"cms", 1620657}, {"h1X10", 2838130}};

  // sample -> medium -> method -> TGraphError *
  std::map<std::string, std::map<std::string, std::map<std::string, TGraphErrors *>>> graphs;

  float max_throughput = 0.0;
  while (file_timing >> sample >> medium >> method >>
         timings[0] >> timings[1] >> timings[2] >>
         timings[3] >> timings[4] >> timings[5])
  {
    if (method == "mmap")
      continue;
    float mean;
    float error;
    GetStats(timings.data(), 6, mean, error);
    float n = nEvents[sample];
    auto throughput_val = n / mean;
    auto throughput_max = n / (mean - error);
    auto throughput_min = n / (mean + error);
    auto throughput_err = (throughput_max - throughput_min) / 2;

    auto g = new TGraphErrors();
    auto x = orderSample[sample] * nMedium + orderMedium[medium];
    g->SetPoint(0, x + 1, throughput_val);
    g->SetPoint(1, x + 2, -1);
    g->SetPointError(0, 0, throughput_err);
    graphs[sample][medium][method] = g;
    max_throughput = std::max(max_throughput, throughput_val + throughput_err);

    std::cout << sample << " " << medium << " " << method << " " <<
      throughput_val << " +/- " << throughput_err << "  [@ " << x << "]" << std::endl;
  }

  auto max_x = nSample * nMedium + 1;

  SetStyle();  // Has to be at the beginning of painting
  TCanvas *canvas = new TCanvas("MyCanvas", "MyCanvas");
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
  helper->SetMinimum(0);
  helper->SetMaximum(max_throughput * 1.1);
  helper->SetTitle("TITLE");

  std::map<std::string, int> colors{{"mem", kRed}, {"ssd", kMagenta + 3}, {"optane", kCyan + 3}};
  std::map<std::string, int> styles{{"lhcb", 1001}, {"cms", 3001}, {"h1X10", 3008}};

  gPad->SetGridy();
  helper->Draw();
  for (const auto &samples : graphs) {
    for (const auto &mediums : samples.second) {
      auto g = mediums.second.at("direct");
      g->SetLineColor(12);
      g->SetMarkerColor(12);
      g->SetFillColor(colors[mediums.first]);
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
