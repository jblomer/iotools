R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

void bm_streams(TString dataSet="result_streams",
               std::string title = "TITLE",
               TString output_path = "graph_streams.root")
{
  std::ifstream file_timing(Form("%s.txt", dataSet.Data()));
  std::string sample;
  std::string compression;
  std::string media;
  Int_t nstreams;
  std::array<float, 6> timings;
  int max_streams = 0;

  // sample --> compresseion --> mean/error
  std::map<std::string, std::map<string, std::pair<float, float>>> data_mem;
  // sample -> compression -> nstreams -> mean/error
  std::map<std::string, std::map<std::string, std::map<int, std::pair<float, float>>>> data;
  // sample -> compression -> nstreams -> mean/error
  std::map<std::string, std::map<std::string, std::map<int, std::pair<float, float>>>> speedup;
  // sample -> compression -> mean/error
  std::map<std::string, std::map<std::string, std::pair<float, float>>> speedup_mem;
  // sample -> compression -> graph
  std::map<std::string, std::map<std::string, TGraphErrors *>> graphs;
  // sample -> compression -> graph
  std::map<std::string, std::map<std::string, TGraphErrors *>> graphs_mem;

  while (file_timing >> media >> sample >> compression >> nstreams >>
         timings[0] >> timings[1] >> timings[2] >>
         timings[3] >> timings[4] >> timings[5])
  {
    float mean;
    float error;
    GetStats(timings.data(), 6, mean, error);
    std::cout << media << " " << sample << " " << compression << " " << nstreams << " " <<
      mean << " +/- " << error << std::endl;

    if (media == "mem") {
      data_mem[sample][compression] = std::pair<float, float>(mean, error);
      if (graphs.count(sample) == 0) {
        graphs_mem[sample] = std::map<std::string, TGraphErrors *>();
      }
      graphs_mem[sample][compression] = new TGraphErrors();
    } else {
      data[sample][compression][nstreams] = std::pair<float, float>(mean, error);

      if (graphs.count(sample) == 0) {
        graphs[sample] = std::map<std::string, TGraphErrors *>();
      }
      if (graphs[sample].count(compression) == 0) {
        graphs[sample][compression] = new TGraphErrors();
      }

      max_streams = std::max(max_streams, nstreams);
    }
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
        auto ratio_err = ratio_val *
                    sqrt(this_err * this_err / this_val / this_val +
                         ref_err * ref_err / ref_val / ref_val);
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
  std::map<std::string, int> x_offsets{{"lhcb", 32}, {"h1X10", 20}, {"cms", 8}};
  for (const auto &samples : data_mem) {
    for (const auto &compressions : samples.second) {
      auto ref_val = data[samples.first][compressions.first][1].first;
      auto ref_err = data[samples.first][compressions.first][1].second;
      auto this_val = compressions.second.first;
      auto this_err = compressions.second.second;
      auto ratio_val = ref_val / this_val;
      auto ratio_err = ratio_val *
                  sqrt(this_err * this_err / this_val / this_val +
                       ref_err * ref_err / ref_val / ref_val);
      speedup_mem[samples.first][compressions.first] =
        std::pair<float, float>(ratio_val, ratio_err);

      std::cout << "MEM REF: " << samples.first << " " << compressions.first << " " <<
        ratio_val << " +/- " << ratio_err << " @" << 2 * max_streams << std::endl;

      min_ratio = std::min(min_ratio, ratio_val - ratio_err);
      max_ratio = std::max(max_ratio, ratio_val + ratio_err);

      graphs_mem[samples.first][compressions.first]->SetPoint(
        0, 2 * max_streams - x_offsets[samples.first], ratio_val);
      graphs_mem[samples.first][compressions.first]->SetPointError(0, 0, ratio_err);
    }
  }

  SetStyle();  // Has to be at the beginning of painting
  gStyle->SetTitleSize(0.03, "T");

  TCanvas *canvas = new TCanvas("MyCanvas", "MyCanvas");
  canvas->cd();
  canvas->SetCanvasSize(1600, 850);
  canvas->SetFillColor(GetTransparentColor());
  gPad->SetLogx(1);
  gPad->SetFillColor(GetTransparentColor());
  gPad->SetGridy();

  auto ymax = 8.5;
  TH1F * helper = new TH1F("", "", 2 * max_streams + 8, 0.9, 2 * max_streams + 8);
  helper->SetMinimum(0);
  helper->SetMaximum(/*max_ratio * 1.25*/ymax);
  helper->GetXaxis()->SetTitle("# Streams                ");
  helper->GetXaxis()->SetTitleOffset(1);
  helper->GetXaxis()->SetLabelSize(0.06);
  helper->GetXaxis()->SetTitleSize(0.04);
  //helper->GetXaxis()->SetNdivisions(1);
  //helper->GetXaxis()->SetBinLabel(1, "1");
  helper->GetXaxis()->SetTickSize(0);
  helper->GetXaxis()->SetLabelSize(0);
  //helper->GetXaxis()->SetBinLabel(2,  "  2");
  //helper->GetXaxis()->SetBinLabel(4,  "  4");
  //helper->GetXaxis()->SetBinLabel(8,  "  8");
  //helper->GetXaxis()->SetBinLabel(16, " 16");
  //helper->GetXaxis()->SetBinLabel(32, " 32");
  //helper->GetXaxis()->SetBinLabel(64, " 64");
  //helper->GetXaxis()->LabelsOption("h");
  //gPad->SetLogx(1);
  helper->GetYaxis()->SetTitle("Speed-up wrt. single stream");
  helper->GetYaxis()->SetLabelSize(0.04);
  helper->GetYaxis()->SetTitleSize(0.04);
  helper->GetYaxis()->SetTitleOffset(0.7);
  //helper->SetTitle(title);
  helper->Draw();

  std::map<std::string, int> sample_colors;
  sample_colors["cms"] = kGreen + 2;
  sample_colors["lhcb"] = kCyan + 2;
  sample_colors["h1X10"] = kMagenta + 2;
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
  for (const auto &samples : graphs_mem) {
    for (const auto &compressions : samples.second) {
      auto g = compressions.second;
      g->SetMarkerStyle(kStar);
      g->SetMarkerColor(sample_colors[samples.first]);
      g->SetLineColor(sample_colors[samples.first]);
      g->SetLineStyle(compression_styles[compressions.first]);
      g->SetLineWidth(2);
      g->Draw("P");
    }
  }

  TLegend *leg = new TLegend(0.15, 0.71, 0.6, 0.86);
  leg->SetNColumns(3);
  leg->SetHeader("\"LHCb\"                         \"H1\"                            \"CMS\"");
  leg->AddEntry(graphs["lhcb"]["none"],  "uncompressed", "l");
  leg->AddEntry(graphs["h1X10"]["none"], "uncompressed", "l");
  leg->AddEntry(graphs["cms"]["none"],   "uncompressed", "l");
  leg->AddEntry(graphs["lhcb"]["zstd"],  "zstd",         "l");
  leg->AddEntry(graphs["h1X10"]["zstd"], "zstd",         "l");
  leg->AddEntry(graphs["cms"]["zstd"],   "zstd",         "l");
  leg->SetBorderSize(1);
  leg->SetTextSize(0.03);
  leg->Draw();
  TText l;
  l.SetTextSize(0.025);
  l.SetTextAlign(13);
  l.DrawTextNDC(0.15, 0.71 - 0.01, "95% CL");

  for (unsigned i = 1; i <= 64; i *= 2) {
    auto tlabel = new TText;
    tlabel->SetTextFont(helper->GetXaxis()->GetTitleFont());
    tlabel->SetTextSize(0.04);
    tlabel->SetTextAlign(22);
    tlabel->DrawText(i, -0.25, std::to_string(i).c_str());
    TLine *line = new TLine(i, 0., i, 0.2);
    line->SetLineColor(kBlack);
    line->Draw();
  }
  auto tlabel = new TText;
  tlabel->SetTextFont(helper->GetXaxis()->GetTitleFont());
  tlabel->SetTextSize(0.04);
  tlabel->SetTextAlign(22);
  tlabel->DrawText(128 - 16, -0.25, "warm cache");
  TLine *line = new TLine(75, 0, 75, ymax);
  line->SetLineColor(kBlack);
  //line->SetLineStyle(2);
  line->Draw();

  TText mbs_cms;
  mbs_cms.SetTextSize(0.03);
  mbs_cms.SetTextColor(sample_colors["cms"]);
  mbs_cms.SetTextAlign(31);
  mbs_cms.DrawText(64, 3.2, "700 MB/s");
  TText mbs_h1;
  mbs_h1.SetTextSize(0.03);
  mbs_h1.SetTextColor(sample_colors["h1X10"]);
  mbs_h1.SetTextAlign(31);
  mbs_h1.DrawText(64, 2.2, "1.2 GB/s");
  TText mbs_lhcb;
  mbs_lhcb.SetTextSize(0.03);
  mbs_lhcb.SetTextColor(sample_colors["lhcb"]);
  mbs_lhcb.SetTextAlign(31);
  mbs_lhcb.DrawText(64, 0.5, "680 MB/s");

  TText ttitle;
  ttitle.SetTextFont(helper->GetXaxis()->GetTitleFont());
  ttitle.SetTextSize(0.03);
  ttitle.SetTextAlign(22);
  ttitle.DrawText(10, 8.75, title.c_str());

  auto output = TFile::Open(output_path, "RECREATE");
  output->cd();
  canvas->Write();
  std::string pdf_path = output_path.View().to_string();
  canvas->Print(TString(pdf_path.substr(0, pdf_path.length() - 4) + "pdf"));
  output->Close();
}
