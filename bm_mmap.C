R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

void bm_mmap(TString dataSet="result_mmap",
             std::string title = "TITLE",
             TString output_path = "graph_UNKNOWN.root",
             bool only_direct = false)
{
  std::ifstream file_timing(Form("%s.txt", dataSet.Data()));
  std::string medium;
  std::string method;
  std::string sample;
  std::array<float, 6> timings;

  std::map<std::string, int> orderMedium{{"mem", 0}, {"optane", 1}, {"ssd", 2}, {"empty", 3}};
  std::map<std::string, int> orderSample{{"lhcb", 0}, {"h1X10", 1}, {"cms", 2}};
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
    if (only_direct && (method != "direct"))
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
    int x;
    if (only_direct)
      x = orderSample[sample] * nMedium + orderMedium[medium];
    else
      x = orderSample[sample] * nMedium * nMethod + orderMedium[medium] * nMethod + orderMethod[method];
    g->SetPoint(0, x + 2, throughput_val);
    g->SetPoint(1, x + 3, -1);
    g->SetPointError(0, 0, throughput_err);
    graphs[sample][medium][method] = g;
    max_throughput = std::max(max_throughput, throughput_val + throughput_err);

    std::cout << sample << " " << medium << " " << method << " " <<
      throughput_val << " +/- " << throughput_err << "  [@ " << x << "]" << std::endl;
  }

  int max_x;
  if (only_direct)
    max_x = nSample * nMedium + 1;
  else
    max_x = nSample * nMedium * nMethod + 1;

  SetStyle();  // Has to be at the beginning of painting
  TCanvas *canvas = new TCanvas("MyCanvas", "MyCanvas");
  canvas->SetCanvasSize(1600, 850);
  canvas->SetFillColor(GetTransparentColor());
  canvas->cd();

  TH1F * helper = new TH1F("", "", max_x, 0, max_x);
  helper->GetXaxis()->SetTitle("");
  helper->GetXaxis()->SetNdivisions(0);
  helper->GetXaxis()->SetLabelSize(0);
  helper->GetXaxis()->SetTickSize(0);
  helper->GetYaxis()->SetTitle("Events / s");
  helper->GetYaxis()->SetLabelSize(0.04);
  helper->GetYaxis()->SetTitleSize(0.04);
  helper->GetYaxis()->SetTitleOffset(0.58);
  helper->SetMinimum(0);
  max_throughput *= 1.1;
  helper->SetMaximum(max_throughput);
  //helper->SetTitle(title);

  std::map<std::string, int> colors{{"mem", kGreen + 2}, {"optane", kCyan + 2}, {"ssd", kMagenta + 2}};
  std::map<std::string, int> styles{{"direct", 1001}, {"mmap", 3001}};

  gPad->SetGridy();
  gPad->SetFillColor(GetTransparentColor());
  helper->Draw();

  std::vector<std::string> plot_samples{"lhcb", "h1X10", "cms"};
  std::vector<std::string> plot_media{"mem", "optane", "ssd"};
  std::vector<std::string> plot_methods{"direct", "mmap"};
  for (const auto &s : plot_samples) {
    for (const auto &m : plot_media) {
      for (const auto &a : plot_methods) {
        auto g = graphs[s][m][a];
        g->SetLineColor(12);
        g->SetMarkerColor(12);
        g->SetFillColor(colors[m]);
        g->SetFillStyle(styles[a]);
        g->SetLineWidth(2);
        g->Draw("B1");
        g->Draw("P");
      }
    }
  }

  for (unsigned i = 8; i < 20; i += 8) {
    TLine *line = new TLine(i + 0.5, 0, i + 0.5, max_throughput);
    line->SetLineColor(kBlack);
    line->SetLineStyle(2);
    line->Draw();
  }

  TLegend *leg;
  leg = new TLegend(0.2, 0.69, 0.5, 0.88);
  leg->SetNColumns(3);
  leg->SetHeader("Mem. cached     Optane              SSD");
  leg->AddEntry(graphs["lhcb"]["mem"]["direct"],    "read()",   "f");
  leg->AddEntry(graphs["lhcb"]["optane"]["direct"], "read()",   "f");
  leg->AddEntry(graphs["lhcb"]["ssd"]["direct"],    "read()",   "f");
  leg->AddEntry(graphs["lhcb"]["mem"]["mmap"],      "mmap()", "f");
  leg->AddEntry(graphs["lhcb"]["optane"]["mmap"],   "mmap()", "f");
  leg->AddEntry(graphs["lhcb"]["ssd"]["mmap"],      "mmap()", "f");
  leg->SetBorderSize(1);
  leg->SetTextSize(0.0275);
  leg->Draw();
  TText l;
  l.SetTextSize(0.025);
  l.SetTextAlign(33);
  l.DrawTextNDC(0.5, 0.69 - 0.005, "95% CL");
  TText ttitle;
  ttitle.SetTextFont(helper->GetXaxis()->GetTitleFont());
  ttitle.SetTextSize(0.03);
  ttitle.SetTextAlign(22);
  ttitle.DrawText(12.5, 50000000, title.c_str());

  TText lLhcb;
  lLhcb.SetTextSize(0.03);
  lLhcb.SetTextAlign(23);
  lLhcb.DrawText(4.5, -500000, "LHCb run 1 open data B2HHH");
  TText lH1;
  lH1.SetTextSize(0.03);
  lH1.SetTextAlign(23);
  lH1.DrawText(12.5, -500000, "H1 micro DST");
  TText lCms;
  lCms.SetTextSize(0.03);
  lCms.SetTextAlign(23);
  lCms.DrawText(20.75, -500000, "CMS nanoAOD TTJet 13TeV June 2019");

  auto output = TFile::Open(output_path, "RECREATE");
  output->cd();
  canvas->Write();
  std::string pdf_path = output_path.View().to_string();
  canvas->Print(TString(pdf_path.substr(0, pdf_path.length() - 4) + "pdf"));
  output->Close();
}
