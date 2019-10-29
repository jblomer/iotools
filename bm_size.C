#include "bm_util.C"


void bm_size(TString dataSet="size", TString title="UNKNOWN TITLE") {
  std::ifstream file(Form("result_size_%s.txt", dataSet.Data()));
  TString format;
  float size;
  vector<TString> format_vec;
  vector<float> size_vec;

  std::map<TString, GraphProperties> props_map;
  FillPropsMap(&props_map);

  std::map<EnumGraphTypes, TypeProperties> graph_map;
  FillGraphMap(&graph_map);

  while (file >> format >> size) {
    cout << format << " " << size << endl;
    format_vec.push_back(format);
    size_vec.push_back(size);
  }

  // sort the vectors in lockstep
  for (unsigned i = 0; i < format_vec.size(); ++i) {
    unsigned idx_min = i;
    for (unsigned j = i + 1; j < format_vec.size(); ++j) {
      if (props_map[format_vec[idx_min]].priority >
          props_map[format_vec[j]].priority)
      {
        idx_min = j;
      }
    }
    if (idx_min != i) {
      std::swap(format_vec[i], format_vec[idx_min]);
      std::swap(size_vec[i], size_vec[idx_min]);
    }
  }

  cout << "Sorted values:" << endl;
  int step = 0;
  float prev_size = 0.0;
  float max_ratio = 0.0;
  std::vector<EnumCompression> ratio_bins;
  for (unsigned i = 0; i < format_vec.size(); ++i) {
    TString format = format_vec[i];
    float size = size_vec[i];
    cout << format << " " << size << endl;

    TGraphErrors *graph_size = graph_map[props_map[format].type].graph;
    graph_size->SetPoint(step, step + 0.5, size);
    graph_size->SetPointError(step, 0, 0);
    for (auto g : graph_map) {
      if (g.first == props_map[format].type) continue;
      if (g.second.is_ratio) continue;
      g.second.graph->SetPoint(step, step + 0.5, -1);
      g.second.graph->SetPointError(step, 0, 0);
    }

    // Ratio plot
    if (step % 2 == 1) {
      auto graph_ratio = graph_map[kGraphRatioDirect].graph;
      auto ratio = size / prev_size;
      graph_ratio->SetPoint(step / 2, step / 2 + 0.5, ratio);
      graph_ratio->SetPointError(step / 2, 0, 0);
      max_ratio = std::max(max_ratio, ratio);
      ratio_bins.push_back(props_map[format].compression);
    }

    step++;
    prev_size = size;
  }
  auto nGraphs = step;
  float max_size = *std::max_element(size_vec.begin(), size_vec.end());


  SetStyle();  // Has to be at the beginning of painting

  TCanvas *canvas = new TCanvas("MyCanvas", "MyCanvas");
  canvas->SetCanvasSize(1600, 850);
  canvas->SetFillColor(GetTransparentColor());
  canvas->cd();

  auto pad_size = new TPad("pad_size", "pad_size", 0.0, 0.39, 1.0, 0.95);
  pad_size->SetFillColor(GetTransparentColor());
  pad_size->SetTopMargin(0.08);
  pad_size->SetBottomMargin(0.03);
  pad_size->SetLeftMargin(0.1);
  pad_size->SetRightMargin(0.055);
  pad_size->Draw();
  canvas->cd();
  auto pad_ratio = new TPad("pad_ratio", "pad_ratio", 0.0, 0.030, 1.0, 0.38);
  pad_ratio->SetFillColor(GetTransparentColor());
  pad_ratio->SetTopMargin(0.05);
  pad_ratio->SetBottomMargin(0.26);
  pad_ratio->SetLeftMargin(0.1);
  pad_ratio->SetRightMargin(0.055);
  pad_ratio->Draw();
  canvas->cd();

  TH1F *helper = new TH1F("", "", nGraphs, 0, nGraphs);
  helper->GetXaxis()->SetNdivisions(5);
  helper->GetXaxis()->SetLabelSize(0);
  helper->GetXaxis()->SetTickSize(0);
  helper->GetYaxis()->SetTitle("Average event size [B]");
  helper->GetYaxis()->SetTickSize(0.01);
  helper->GetYaxis()->SetLabelSize(0.07);
  helper->GetYaxis()->SetTitleSize(0.07);
  helper->GetYaxis()->SetTitleOffset(0.58);
  helper->SetMinimum(0);
  helper->SetMaximum(max_size * 1.05);
  helper->SetTitle(title);

  TH1F *helper2 = new TH1F("", "", ratio_bins.size(), 0, ratio_bins.size());
  helper2->SetMinimum(0);
  helper2->SetMaximum(std::max(max_ratio * 1.05, 1.05));
  for (unsigned i = 0; i < ratio_bins.size(); ++i) {
    helper2->GetXaxis()->SetBinLabel(i + 1, kCompressionNames[ratio_bins[i]]);
  }
  //helper2->GetXaxis()->SetTitle("Compression");
  //helper2->GetXaxis()->CenterTitle();
  helper2->GetXaxis()->SetTickSize(0);
  helper2->GetXaxis()->SetLabelSize(0.16);
  helper2->GetXaxis()->SetTitleSize(0.12);
  helper2->GetYaxis()->SetTitle("RNTuple / TTree");
  helper2->GetYaxis()->SetTickSize(0.005);
  helper2->GetYaxis()->SetNdivisions(6);
  helper2->GetYaxis()->SetLabelSize(0.11);
  helper2->GetYaxis()->SetTitleSize(0.11);
  helper2->GetYaxis()->SetTitleOffset(0.35);

  pad_size->cd();
  gPad->SetGridy();
  gPad->SetGridx();

  helper->Draw();
  TGraphErrors *graph_size = graph_map[kGraphTreeDirect].graph;
  for (auto g : graph_map) {
    if (g.second.is_ratio) continue;
    g.second.graph->SetFillColor(graph_map[g.first].color);
    g.second.graph->SetLineWidth(2);
    g.second.graph->Draw("B");
  }

  TLegend *leg = new TLegend(0.8, 0.7, 0.9, 0.9);
  leg->AddEntry(graph_map[kGraphTreeDirect].graph,   "TTree", "F");
  leg->AddEntry(graph_map[kGraphNtupleDirect].graph, "RNTuple", "F");
  leg->SetTextSize(0.05);
  leg->Draw();

  pad_ratio->cd();
  gPad->SetGridy();
  gPad->SetGridx();

  TGraphErrors *graph_ratio = graph_map[kGraphRatioDirect].graph;
  graph_ratio->SetLineColor(12);
  graph_ratio->SetMarkerColor(12);
  graph_ratio->SetFillColor(graph_map[kGraphRatioDirect].color);
  graph_ratio->SetLineWidth(2);
  helper2->Draw();
  graph_ratio->Draw("B");

  cout << "Writing into " << Form("%s.root", dataSet.Data()) << endl;
  TFile * output =
    TFile::Open(Form("graph_size.%s.root", dataSet.Data()), "RECREATE");
  output->cd();
  canvas->Write();
  canvas->Print(Form("graph_size.%s.pdf", dataSet.Data()));
  output->Close();
}
