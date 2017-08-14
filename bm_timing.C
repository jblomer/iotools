R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

TString GetPhysicalFormat(TString format) {
  TObjArray *tokens = format.Tokenize("~");
  TString physical_format =
    reinterpret_cast<TObjString *>(tokens->At(0))->CopyString();
  delete tokens;
  return physical_format;
}

float GetBloatFactor(TString format) {
  if (format.EndsWith("times10"))
    return 10.0;
  return 1.0;
}

void bm_timing(TString dataSet="result_read_mem",
               TString title = "TITLE",
               TString output_path = "graph_UNKNOWN.root",
               float limit_y = -1.0)
{
  std::ifstream file_timing(Form("%s.txt", dataSet.Data()));
  std::ifstream file_size("result_size.txt");
  TString format;
  float size;
  std::array<float, 6> timings;
  vector<TString> format_vec;
  vector<float> throughput_mbsval_vec;
  vector<float> throughput_mbserr_vec;
  vector<float> throughput_evsval_vec;
  vector<float> throughput_evserr_vec;
  bool show_events_per_second = true;

  const float nevent = 8556118.;

  std::map<TString, GraphProperties> props_map;
  FillPropsMap(&props_map);

  SetStyle();
  TCanvas *canvas = new TCanvas();

  std::map<EnumGraphTypes, TypeProperties> graph_map;
  FillGraphMap(&graph_map);

  while (file_size >> format >> size) {
    cout << format << "(size) " << size << endl;
    props_map[format].size = size;
  }

  while (file_timing >> format >>
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

    float event_size = props_map[GetPhysicalFormat(format)].size;
    float throughput_event_val = (nevent * GetBloatFactor(format)) / mean;
    throughput_evsval_vec.push_back(throughput_event_val);
    float throughput_byte_val = (throughput_event_val * event_size);
    float throughput_mb_val = throughput_byte_val / (1024 * 1024);
    throughput_mbsval_vec.push_back(throughput_mb_val);
    float thoughput_event_max = (nevent * GetBloatFactor(format)) / min;
    float thoughput_event_min = (nevent * GetBloatFactor(format)) / max;
    float throughput_ev_err =
      (thoughput_event_max - thoughput_event_min) / 2.;
    float throughput_byte_max = thoughput_event_max * event_size;
    float throughput_byte_min = thoughput_event_min * event_size;
    float throughput_mb_err = (throughput_byte_max - throughput_byte_min) / 2.;
    throughput_mb_err /= (1024 * 1024);
    throughput_mbserr_vec.push_back(throughput_mb_err);
    throughput_evserr_vec.push_back(throughput_ev_err);

    cout << format << "(time) " << throughput_mb_val << " " << throughput_mb_err
         << " " << throughput_event_val << " " << throughput_ev_err
         << endl;
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
      std::swap(throughput_mbsval_vec[i], throughput_mbsval_vec[idx_min]);
      std::swap(throughput_mbserr_vec[i], throughput_mbserr_vec[idx_min]);
      std::swap(throughput_evsval_vec[i], throughput_evsval_vec[idx_min]);
      std::swap(throughput_evserr_vec[i], throughput_evserr_vec[idx_min]);
    }
  }

  std::cout << "Sorted results: " << std::endl;
  int step = 0;
  for (unsigned i = 0; i < format_vec.size(); ++i) {
    TString format = format_vec[i];
    float throughput_val;
    float throughput_err;
    if (show_events_per_second) {
      throughput_val = throughput_evsval_vec[i];
      throughput_err = throughput_evserr_vec[i];
    } else {
      throughput_val = throughput_mbsval_vec[i];
      throughput_err = throughput_mbserr_vec[i];
    }
    cout << format << " " << throughput_val << " " << throughput_err << endl;

    TGraphErrors *graph_throughput = graph_map[props_map[format].type].graph;
    graph_throughput->SetPoint(step, kBarSpacing * step, throughput_val);
    graph_throughput->SetPointError(step, 0, throughput_err);
    for (auto g : graph_map) {
      if (g.first == props_map[format].type) continue;
      g.second.graph->SetPoint(step, kBarSpacing * step, 0);
      g.second.graph->SetPointError(step, 0, 0);
    }
    step++;
  }

  float max_throughput;
  if (show_events_per_second) {
    max_throughput = *std::max_element(throughput_evsval_vec.begin(),
                                       throughput_evsval_vec.end());
  } else {
    max_throughput = *std::max_element(throughput_mbsval_vec.begin(),
                                       throughput_mbsval_vec.end());
  }

  TGraphErrors *graph_throughput = graph_map[kGraphInflated].graph;
  graph_throughput->SetTitle(title);
  graph_throughput->GetXaxis()->SetTitle("File format");
  graph_throughput->GetXaxis()->SetTitleSize(0.04);
  graph_throughput->GetXaxis()->CenterTitle();
  graph_throughput->GetXaxis()->SetTickSize(0);
  graph_throughput->GetXaxis()->SetLabelSize(0);
  graph_throughput->GetXaxis()->SetLimits(-1, kBarSpacing * step);

  if (show_events_per_second) {
    graph_throughput->GetYaxis()->SetTitle("Events / s");
  } else {
    graph_throughput->GetYaxis()->SetTitle("Event Size x Events/s [MB/s]");
  }
  graph_throughput->GetYaxis()->SetTitleSize(0.04);
  graph_throughput->GetYaxis()->SetTitleOffset(1.25);
  if (limit_y < 0)
    limit_y = max_throughput;
  else
    limit_y = limit_y / 1.125;
  graph_throughput->GetYaxis()->SetRangeUser(0, limit_y * 1.125);
  graph_throughput->SetFillColor(graph_map[kGraphInflated].color);
  graph_throughput->Draw("AB");
  graph_throughput->Draw("P");
  for (auto g : graph_map) {
    if (g.first == kGraphInflated) continue;
    g.second.graph->SetFillColor(graph_map[g.first].color);
    g.second.graph->Draw("B");
    g.second.graph->Draw("P");
  }

  //TLegend *leg = new TLegend(0.6, 0.7, 0.89, 0.89);
  TLegend *leg = new TLegend(0.95, 0.95, 0.7, 0.8);
  leg->AddEntry(graph_map[kGraphInflated].graph, "uncompressed", "F");
  leg->AddEntry(graph_map[kGraphDeflated].graph, "compressed", "F");
  leg->SetTextSize(0.03);
  leg->Draw();

  for (unsigned i = 0; i < format_vec.size(); ++i) {
    TText l;
    l.SetTextAlign(12);
    l.SetTextSize(0.03);
    l.SetTextColor(4);  // blue
    l.SetTextAngle(90);
    l.DrawText(kBarSpacing * i, gPad->YtoPad(limit_y * 0.1),
               props_map[format_vec[i]].title);
  }

  //canvas->SetLogy();

  TFile * output =
    TFile::Open(output_path, "RECREATE");
  output->cd();
  canvas->Write();
  output->Close();
}
