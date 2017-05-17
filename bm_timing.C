R__LOAD_LIBRARY(libMathMore)

enum EnumGraphTypes { kGraphInflated, kGraphDeflated };

struct TypeProperties {
  TypeProperties() : graph(NULL), color(0) { };
  TypeProperties(TGraphErrors *g, int c) : graph(g), color(c) { }

  TGraphErrors *graph;
  int color;
};

struct GraphProperties {
  GraphProperties() : type(kGraphInflated), title("UNKNOWN") { }
  GraphProperties(EnumGraphTypes ty, TString ti) : type(ty), title(ti) { }

  EnumGraphTypes type;
  TString title;
};

void bm_timing(TString dataSet="result_read_mem",
               TString title = "TITLE",
               TString output_path = "graph_UNKNOWN.root")
{
  std::ifstream file(Form("%s.txt", dataSet.Data()));
  TString format;
  std::array<float, 6> timings;
  vector<TString> format_vec;
  vector<float> throughput_val_vec;
  vector<float> throughput_err_vec;

  const float nevent = 8556118.;
  const float bar_spacing = 1.3;

  std::map<TString, GraphProperties> props_map;
  props_map["root-inflated"] =
   GraphProperties(kGraphInflated, "ROOT (inflated)");
  props_map["root-deflated"] =
    GraphProperties(kGraphDeflated, "ROOT (compressed)");
  props_map["avro-inflated"] =
    GraphProperties(kGraphInflated, "Avro (inflated)");
  props_map["avro-deflated"] =
    GraphProperties(kGraphDeflated, "Avro (compressed)");
  props_map["parquet-inflated"] =
    GraphProperties(kGraphInflated, "Parquet (inflated)");
  props_map["parquet-deflated"] =
    GraphProperties(kGraphDeflated, "Parquet (compressed)");
  props_map["protobuf-inflated"]
    = GraphProperties(kGraphInflated, "Protobuf (inflated)");
  props_map["protobuf-deflated"]
    = GraphProperties(kGraphDeflated, "Protobuf (compressed)");
  props_map["h5row"] = GraphProperties(kGraphInflated, "HDF5 (row-wise)");
  props_map["h5column"] =
    GraphProperties(kGraphInflated, "HDF5 (column-wise)");
  props_map["sqlite"] =
    GraphProperties(kGraphInflated, "SQlite");

  TCanvas *canvas = new TCanvas();

  std::map<EnumGraphTypes, TypeProperties> graph_map;
  graph_map[kGraphInflated] = TypeProperties(new TGraphErrors(), 40);
  graph_map[kGraphDeflated] = TypeProperties(new TGraphErrors(), 46);

  int step = 0;
  while (file >> format >>
         timings[0] >> timings[1] >> timings[2] >>
         timings[3] >> timings[4] >> timings[5])
  {
    format_vec.push_back(format);

    float n = timings.size();
    float mean = 0.0;
    for (auto t : timings)
      mean += t;
    mean /= n;
    float s2 = 0.0;
    for (auto t : timings)
      s2 += (t - mean) * (t - mean);
    s2 /= (n - 1);
    float s = sqrt(s2);
    float t = abs(ROOT::Math::tdistribution_quantile(0.05 / 2., n - 1));
    float error = t * s / sqrt(n);
    error *= 1.5;  // safety margin
    float max = mean + error;
    float min = mean - error;
    //float max = *std::max_element(timings.begin(), timings.end());
    //float min = *std::min_element(timings.begin(), timings.end());

    float throughput_val = nevent/mean;
    throughput_val_vec.push_back(throughput_val);
    float max_throughput = nevent/min;
    float min_throughput = nevent/max;
    float throughput_err = (max_throughput - min_throughput) / 2.;
    throughput_err_vec.push_back(throughput_err);

    cout << format << " " << throughput_val << " " << throughput_err << endl;
    TGraphErrors *graph_throughput = graph_map[props_map[format].type].graph;
    graph_throughput->SetPoint(step, bar_spacing * step, throughput_val);
    graph_throughput->SetPointError(step, 0, throughput_err);
    step++;
  }

  float max_throughput =
    *std::max_element(throughput_val_vec.begin(), throughput_val_vec.end());

  gStyle->SetEndErrorSize(6);

  TGraphErrors *graph_throughput = graph_map[kGraphInflated].graph;
  graph_throughput->SetTitle(title);
  graph_throughput->GetXaxis()->SetTitle("File format");
  graph_throughput->GetXaxis()->CenterTitle();
  graph_throughput->GetXaxis()->SetTickSize(0);
  graph_throughput->GetXaxis()->SetLabelSize(0);
  graph_throughput->GetXaxis()->SetLimits(-1, bar_spacing * step);
  graph_throughput->GetYaxis()->SetTitle("# events per second");
  graph_throughput->GetYaxis()->SetRangeUser(1, max_throughput * 1.125);
  graph_throughput->SetFillColor(graph_map[kGraphInflated].color);
  graph_throughput->Draw("AB");
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
    l.DrawText(bar_spacing * i, gPad->YtoPad(max_throughput * 0.1),
               props_map[format_vec[i]].title);
  }

  //canvas->SetLogy();

  TFile * output =
    TFile::Open(output_path, "RECREATE");
  output->cd();
  canvas->Write();
  output->Close();
}
