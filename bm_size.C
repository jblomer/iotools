#include "bm_util.C"


void bm_size(TString dataSet="result_size") {
  std::ifstream file(Form("%s.txt", dataSet.Data()));
  TString format;
  float size;
  vector<TString> format_vec;
  vector<float> size_vec;

  std::map<TString, GraphProperties> props_map;
  FillPropsMap(&props_map);

  TCanvas *canvas = new TCanvas();

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
  for (unsigned i = 0; i < format_vec.size(); ++i) {
    TString format = format_vec[i];
    float size = size_vec[i];
    cout << format << " " << size << endl;

    TGraphErrors *graph_size = graph_map[props_map[format].type].graph;
    graph_size->SetPoint(step, kBarSpacing * step, size);
    graph_size->SetPointError(step, 0, 0);
    step++;
  }

  float max_size = *std::max_element(size_vec.begin(), size_vec.end());

  TGraphErrors *graph_size = graph_map[kGraphInflated].graph;
  graph_size->SetTitle("Data size LHCb OpenData");
  graph_size->GetXaxis()->SetTitle("File format");
  graph_size->GetXaxis()->SetTitleSize(0.04);
  graph_size->GetXaxis()->CenterTitle();
  graph_size->GetXaxis()->SetTickSize(0);
  graph_size->GetXaxis()->SetLabelSize(0);
  graph_size->GetXaxis()->SetLimits(-1, kBarSpacing * step);
  graph_size->GetYaxis()->SetTitle("Size per event [B]");
  graph_size->GetYaxis()->SetTitleSize(0.04);
  graph_size->GetYaxis()->SetRangeUser(0, max_size * 1.3);
  graph_size->SetFillColor(graph_map[kGraphInflated].color);
  graph_size->Draw("AB");
  for (auto g : graph_map) {
    if (g.first == kGraphInflated) continue;
    g.second.graph->SetFillColor(graph_map[g.first].color);
    g.second.graph->Draw("B");
  }

  TLegend *leg = new TLegend(0.6, 0.7, 0.89, 0.89);
  leg->AddEntry(graph_map[kGraphInflated].graph, "uncompressed", "F");
  leg->AddEntry(graph_map[kGraphDeflated].graph, "compressed", "F");
  gStyle->SetLegendTextSize(0.04);
  leg->Draw();

  for (unsigned i = 0; i < format_vec.size(); ++i) {
    TText l;
    l.SetTextAlign(12);
    l.SetTextSize(0.04);
    l.SetTextColor(4);  // blue
    l.SetTextAngle(90);
    l.DrawText(kBarSpacing * i, gPad->YtoPad(max_size * 0.1),
               props_map[format_vec[i]].title);
  }

  TFile * output =
    TFile::Open(Form("graph_size.root"), "RECREATE");
  output->cd();
  canvas->Write();
  output->Close();
}
