R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

void bm_chep23(TString dataSet="bm_chep23",
               TString title = "RNTuple + RDataFrame benchmarks, zstd compressed input data",
               std::string output_path = "graph_chep23.root")
{
  std::ifstream file_timing(Form("%s.txt", dataSet.Data()));

  std::map<std::string, int> orderMethod{{"tmpfs", 0}, {"ssd", 1}, {"hdd", 2}, {"xrootd", 3}};
  std::map<std::string, int> orderSample{{"lhcb", 0}, {"h1", 1}, {"cms", 2}, {"atlas", 3}, {"empty", 4}};
  std::map<std::string, int> orderContainer{{"tree", 0}, {"ntuple", 1}};
  auto nMethod = orderMethod.size();
  auto nSample = orderSample.size();
  auto nContainer = orderContainer.size();

  std::map<std::string, int> nEvents{{"lhcb", 8556118}, {"cms", 1620657}, {"h1", 2838130}, {"atlas", 7798424}};
  std::map<std::string, int> deliveredBytes{{"lhcb", 150094236}, {"atlas", 267999743}, {"cms", 16545168}, {"h1", 33626120}};

  // method -> sample -> container -> vector(timing, bandwidth)
  std::map<std::string, std::map<std::string, std::map<std::string, std::vector<std::pair<double, double>>>>> measurement;
  // method -> sample -> container -> (val, error)
  std::map<std::string, std::map<std::string, std::map<std::string, std::pair<double, double>>>> data;
  std::map<std::string, std::map<std::string, std::map<std::string, TGraphErrors *>>> graphs;
  // method -> sample -> graph
  std::map<std::string, std::map<std::string, TGraphErrors *>> gratios;

  double max_throughput = 0.0;
  std::string sample;
  std::string method;
  std::string container;
  int run;
  double realtime;
  double bandwidth;
  while (file_timing >> sample >> method >> container >> run >> realtime >> bandwidth)
  {
    //printf("proc: %s %s %s\n", sample.c_str(), method.c_str(), container.c_str());
    auto &c = measurement[method][sample][container];
    c.push_back(std::pair<double, double>(realtime, bandwidth));
  }

  for (const auto &m : measurement) {
    for (const auto &s : m.second) {
        for (const auto &c : s.second) {
            const auto &vals = c.second;
            method = m.first;
            sample = s.first;
            container = c.first;

            double mean;
            double error;
            double median;
            std::vector<double> rt;
            for (const auto &p : vals) {
                rt.push_back(p.first);
            }
            GetStats(rt.data(), rt.size(), mean, error, median);

            if (method == "xrootd") {
                for (unsigned i = 0; i < rt.size(); ) {
                    if (rt[i] > 1.5*median) {
                        rt.erase(rt.begin() + i);
                    } else {
                        i++;
                    }
                }
            }

            printf("%s %s %s (%lf)", method.c_str(), sample.c_str(), container.c_str(), median);
            for (const auto &x : rt) {
                printf(" %lf", x);
            }
            printf("\n");

            GetStats(rt.data(), rt.size(), mean, error, median);

            double n = nEvents[sample];
            double sz = deliveredBytes[sample];
            sz /= 1024;
            sz /= 1024;
            auto throughput_val = sz / mean;
            auto throughput_max = sz / (mean - error);
            auto throughput_min = sz / (mean + error);
            auto throughput_err = (throughput_max - throughput_min) / 2;

            auto g = new TGraphErrors();
            auto x = orderMethod[method] * nSample * nContainer + orderSample[sample] * nContainer
                + orderContainer[container];
            g->SetPoint(0, x + 1, -1);
            g->SetPoint(1, x + 1 + 1, throughput_val);
            g->SetPoint(2, x + 2 + 1, -1);
            g->SetPointError(1, 0, throughput_err);
            graphs[method][sample][container] = g;
            data[method][sample][container] = std::pair<double, double>(throughput_val, throughput_err);
            max_throughput = std::max(max_throughput, throughput_val + throughput_err);

            std::cout << sample << " " << method << " " << container << " " <<
                throughput_val << " +/- " << throughput_err << "  [@ " << x << "]" << std::endl;
        }
    }
  }

  double max_ratio = 0.0;
  for (const auto &methods : data) {
    for (const auto &samples : methods.second) {
      auto ntuple_val = samples.second.at("ntuple").first;
      auto ntuple_err = samples.second.at("ntuple").second;
      auto tree_val = samples.second.at("tree").first;
      auto tree_err = samples.second.at("tree").second;
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
  canvas->SetCanvasSize(1600, 850);
  canvas->SetFillColor(GetTransparentColor());
  canvas->cd();

  auto pad_throughput = new TPad("pad_throughput", "pad_throughput",
                                 0.0, 0.39, 1.0, 0.95);
  pad_throughput->SetFillColor(GetTransparentColor());
  pad_throughput->SetTopMargin(0.08);
  pad_throughput->SetBottomMargin(0.03);
  pad_throughput->SetLeftMargin(0.1);
  pad_throughput->SetRightMargin(0.055);
  pad_throughput->Draw();
  canvas->cd();
  auto pad_ratio = new TPad("pad_ratio", "pad_ratio", 0.0, 0.030, 1.0, 0.38);
  pad_ratio->SetFillColor(GetTransparentColor());
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
  helper->GetYaxis()->SetTitle("Data used in analysis [MiB/s]");
  helper->GetYaxis()->SetTickSize(0.01);
  helper->GetYaxis()->SetLabelSize(0.07);
  helper->GetYaxis()->SetTitleSize(0.07);
  helper->GetYaxis()->SetTitleOffset(0.58);
  helper->SetMinimum(1);
  max_throughput *= 1.8;
  helper->SetMaximum(max_throughput);
  helper->SetTitle(title);

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

  std::map<std::string, int> colors{{"tree", kBlue}, {"ntuple", kRed}};
  std::map<std::string, int> styles{{"atlas", 3011}, {"lhcb", 1001}, {"cms", 3001}, {"h1", 3008}};

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

  for (unsigned i = 10; i < 32; i += 10) {
    TLine *line = new TLine(i + 0.5, 0, i + 0.5, max_throughput);
    line->SetLineColor(kBlack);
    line->SetLineStyle(2);
    line->Draw();
  }

  TLegend *leg;
  leg = new TLegend(0.5, 0.65, 0.68, 0.9);
  leg->SetNColumns(4);
  leg->SetHeader("\"LHCb\"            \"H1\"    \"CMS\" \"ATLAS\"");
  leg->AddEntry(graphs["ssd"]["lhcb"]["tree"],    "TTree",   "f");
  leg->AddEntry(graphs["ssd"]["h1"]["tree"],   "    ",   "f");
  leg->AddEntry(graphs["ssd"]["cms"]["tree"],     "    ",   "f");
  leg->AddEntry(graphs["ssd"]["atlas"]["tree"],     "    ",   "f");
  leg->AddEntry(graphs["ssd"]["lhcb"]["ntuple"],  "RNTuple", "f");
  leg->AddEntry(graphs["ssd"]["h1"]["ntuple"], "    ", "f");
  leg->AddEntry(graphs["ssd"]["cms"]["ntuple"],   "    ", "f");
  leg->AddEntry(graphs["ssd"]["atlas"]["ntuple"],   "    ", "f");
  leg->SetBorderSize(1);
  leg->SetTextSize(0.0375);
  leg->Draw();
  TText l;
  l.SetTextSize(0.04);
  l.SetTextAlign(13);
  l.DrawTextNDC(0.9025, 0.9, "95% CL");

  pad_ratio->cd();
  gPad->SetGridy();
  helper2->Draw();
  TLine *lineOne = new TLine(0, 1, max_x, 1);
  lineOne->SetLineColor(kRed);
  lineOne->SetLineStyle(0);
  lineOne->SetLineWidth(2);
  lineOne->Draw();

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

  for (unsigned i = 10; i < 32; i += 10) {
    TLine *line = new TLine(i + 0.5, 0, i + 0.5, max_ratio);
    line->SetLineColor(kBlack);
    line->SetLineStyle(2);
    line->Draw();
  }

  TLegend *legr = new TLegend(0.125, 0.85, 0.35, 1.);
  legr->SetNColumns(4);
  legr->SetBorderSize(1);
  legr->AddEntry(gratios["ssd"]["lhcb"],  "\"LHCb\"", "f");
  legr->AddEntry(gratios["ssd"]["h1"], "\"H1\"",   "f");
  legr->AddEntry(gratios["ssd"]["cms"],   "\"CMS\"",  "f");
  legr->AddEntry(gratios["ssd"]["atlas"],   "\"ATLAS\"",  "f");
  legr->SetTextSize(0.075);
  legr->Draw();

  TText lTmpfs;
  lTmpfs.SetTextSize(0.085);
  lTmpfs.SetTextAlign(11);
  lTmpfs.DrawText(3.5, -0.5, "RAM Disk");
  TText lSsd;
  lSsd.SetTextSize(0.085);
  lSsd.SetTextAlign(11);
  lSsd.DrawText(12.5, -0.5, "Solid State Disk");
  TText lHdd;
  lHdd.SetTextSize(0.085);
  lHdd.SetTextAlign(11);
  lHdd.DrawText(23, -0.5, "Spinning Disk");
  TText lHttp;
  lHttp.SetTextSize(0.085);
  lHttp.SetTextAlign(11);
  lHttp.DrawText(31.5, -0.5, "XRootD, 100GbE, 0.3ms");

  auto output = TFile::Open(output_path.c_str(), "RECREATE");
  output->cd();
  canvas->Write();
  std::string pdf_path = output_path;
  canvas->Print(TString(pdf_path.substr(0, pdf_path.length() - 4) + "pdf"));
  output->Close();
}
