R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

void bm_streams(TString dataSet="result_streams",
               TString title = "TITLE",
               TString output_path = "graph_UNKNOWN.root")
{
  std::ifstream file_timing(Form("%s.txt", dataSet.Data()));
  std::string sample;
  std::string compression;
  Int_t nstreams;
  std::array<float, 6> timings;
  int max_streams = 0;

  // sample -> compression -> nstreams -> mean/error
  std::map<std::string, std::map<std::string, std::map<int, std::pair<float, float>>>> data;
  // sample -> compression -> nstreams -> mean/error
  std::map<std::string, std::map<std::string, std::map<int, std::pair<float, float>>>> speedup;
  // sample -> compression -> graph
  std::map<std::string, std::map<std::string, TGraphErrors *>> graphs;

  while (file_timing >> sample >> compression >> nstreams >>
         timings[0] >> timings[1] >> timings[2] >>
         timings[3] >> timings[4] >> timings[5])
  {
    float mean;
    float error;
    GetStats(timings.data(), 6, mean, error);
    std::cout << sample << " " << compression << " " << nstreams << " " <<
      mean << " +/- " << error << std::endl;

    data[sample][compression][nstreams] = std::pair<float, float>(mean, error);

    if (graphs.count(sample) == 0) {
      graphs[sample] = std::map<std::string, TGraphErrors *>();
    }
    if (graphs[sample].count(compression) == 0) {
      graphs[sample][compression] = new TGraphErrors();
    }

    max_streams = std::max(max_streams, nstreams);
  }

  float min_ratio = 1.0;
  float max_ratio = 1.0;
  for (const auto &samples : data) {
    for (const auto &compressions : samples.second) {
      for (const auto &streams : compressions.second) {
        auto ref_val = data[samples.first][compressions.first][1].first;
        auto ref_err = data[samples.first][compressions.first][1].second;
        auto this_val = streams.second.first;
        auto this_err = streams.second.second;
        auto ratio_val = ref_val / this_val;
        auto ratio_err = this_err * this_err / this_val / this_val +
                         ref_err * ref_err / ref_val / ref_val;
        speedup[samples.first][compressions.first][streams.first] =
          std::pair<float, float>(ratio_val, ratio_err);

        std::cout << samples.first << " " << compressions.first << " " << streams.first << " " <<
          ratio_val << " +/- " << ratio_err << std::endl;

        min_ratio = std::min(min_ratio, ratio_val - ratio_err);
        max_ratio = std::max(max_ratio, ratio_val + ratio_err);

        auto step = graphs[samples.first][compressions.first]->GetN();
        graphs[samples.first][compressions.first]->SetPoint(step, streams.first, ratio_val);
        graphs[samples.first][compressions.first]->SetPointError(step, 0, ratio_err);
      }
    }
  }

  SetStyle();  // Has to be at the beginning of painting
  TCanvas *canvas = new TCanvas("MyCanvas", "MyCanvas");
  canvas->cd();

  TH1F * helper = new TH1F("", "", max_streams, 0, max_streams);
  helper->GetXaxis()->SetTitle("# Streams");
  helper->GetXaxis()->SetTitleOffset(0.7);
  helper->GetXaxis()->SetLabelSize(0.05);
  helper->GetXaxis()->SetTitleSize(0.06);
  helper->GetYaxis()->SetTitle("Speed-Up");
  helper->GetYaxis()->SetLabelSize(0.05);
  helper->GetYaxis()->SetTitleSize(0.06);
  helper->GetYaxis()->SetTitleOffset(0.7);
  helper->SetMinimum(min_ratio * 0.9);
  helper->SetMaximum(max_ratio * 1.1);
  helper->SetTitle("title");
  helper->Draw();

  std::map<std::string, int> sample_colors;
  sample_colors["cms"] = kRed;
  sample_colors["lhcb"] = kBlue;
  sample_colors["h1X10"] = kMagenta;
  std::map<std::string, int> compression_styles;
  compression_styles["none"] = 1;
  compression_styles["zstd"] = 7;

  for (const auto &samples : graphs) {
    for (const auto &compressions : samples.second) {
      auto g = compressions.second;
      g->SetMarkerStyle(kStar);
      g->SetMarkerColor(sample_colors[samples.first]);
      g->SetLineColor(sample_colors[samples.first]);
      g->SetLineStyle(compression_styles[compressions.first]);
      g->SetLineWidth(2);
      g->Draw("LP");
    }
  }


//  auto output = TFile::Open(output_path, "RECREATE");
//  output->cd();
//  canvas->Write();
//  output->Close();
}
