R__LOAD_LIBRARY(libMathMore)

#include "bm_util.C"

void bm_timing(TString dataSet="result_read_mem",
               TString pathSize="result_size",
               TString title = "TITLE",
               TString output_path = "graph_UNKNOWN.root",
               float nevent = 0.0,
               float limit_y = -1.0,
               bool show_events_per_second = true,
               int aspect_ratio = 0)
{
  std::ifstream file_timing(Form("%s.txt", dataSet.Data()));
  std::ifstream file_size(Form("%s.txt", pathSize.Data()));
  TString format;
  float size;
  std::array<float, 6> timings;
  vector<TString> format_vec;
  vector<float> throughput_mbsval_vec;
  vector<float> throughput_mbserr_vec;
  vector<float> throughput_evsval_vec;
  vector<float> throughput_evserr_vec;
  vector<float> ratio_vec;
  vector<float> ratio_err;

  std::map<TString, GraphProperties> props_map;
  FillPropsMap(&props_map);

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
  float prev_val = 0.0;
  float prev_err = 0.0;
  float max_ratio = 0.0;
  for (unsigned i = 0; i < format_vec.size(); ++i) {
    TString format = format_vec[i];
    float throughput_val = 0.0;
    float throughput_err = 0.0;
    if (show_events_per_second) {
      throughput_val = throughput_evsval_vec[i];
      throughput_err = throughput_evserr_vec[i];
    } else {
      throughput_val = throughput_mbsval_vec[i];
      throughput_err = throughput_mbserr_vec[i];
    }
    cout << format << " " << throughput_val << " " << throughput_err << endl;

    TGraphErrors *graph_throughput = graph_map[props_map[format].type].graph;
    graph_throughput->SetPoint(step, step + 0.5, throughput_val);
    graph_throughput->SetPointError(step, 0, throughput_err);
    for (auto g : graph_map) {
      if (g.first == props_map[format].type) continue;

      if (g.first == kGraphRatio) {
        if (step % 2 == 1) {
          auto ratio_val = throughput_val / prev_val;
          auto ratio_err = ratio_val *
            sqrt(throughput_err * throughput_err / throughput_val / throughput_val +
                 prev_err * prev_err / prev_val / prev_val);
          g.second.graph->SetPoint(step / 2, step / 2 + 0.5, ratio_val);
          g.second.graph->SetPointError(step / 2, 0, ratio_err);
          max_ratio = std::max(max_ratio, ratio_val + ratio_err);
        }
      } else {
        g.second.graph->SetPoint(step, step + 0.5, -1);
        g.second.graph->SetPointError(step, 0, 0);
      }
    }
    step++;
    prev_val = throughput_val;
    prev_err = throughput_err;
  }

  float max_throughput;
  if (show_events_per_second) {
    max_throughput = *std::max_element(throughput_evsval_vec.begin(),
                                       throughput_evsval_vec.end());
  } else {
    max_throughput = *std::max_element(throughput_mbsval_vec.begin(),
                                       throughput_mbsval_vec.end());
  }
  if (limit_y < 0)
    limit_y = max_throughput;
  else
    limit_y = limit_y / 1.125;
  TString ytitle;
  if (show_events_per_second) {
    ytitle = "Events / s";
  } else {
    ytitle = "Event Size x Events/s [MB/s]";
  }
  if (aspect_ratio == 1) {
    ytitle = "Ev / s";
  }


  ////////////////////////// PAINTING

  SetStyle();  // Has to be at the beginning of painting

  TCanvas *canvas = new TCanvas("MyCanvas", "MyCanvas");
  if (aspect_ratio == 1)
    canvas->SetCanvasSize(394, 535);
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

  TH1F * helper = new TH1F("", "", 8, 0, 8);
  helper->GetXaxis()->SetTitle("");
  helper->GetXaxis()->SetNdivisions(4);
  helper->GetXaxis()->SetLabelSize(0);
  helper->GetXaxis()->SetTickSize(0);
  helper->GetYaxis()->SetTitle(ytitle);
  helper->GetYaxis()->SetLabelSize(0.07);
  helper->GetYaxis()->SetTitleSize(0.08);
  helper->GetYaxis()->SetTitleOffset(0.58);
  helper->SetMinimum(0);
  helper->SetMaximum(limit_y);
  helper->SetTitle(title);

  TH1F *helper2 = new TH1F("", "", 4, 0, 4);
  helper2->SetMinimum(0);
  helper2->SetMaximum(max_ratio * 1.05);
  helper2->GetXaxis()->SetBinLabel(1,"uncompressed");
  helper2->GetXaxis()->SetBinLabel(2,"lz4");
  helper2->GetXaxis()->SetBinLabel(3,"zlib");
  helper2->GetXaxis()->SetBinLabel(4,"lzma");
  helper2->GetXaxis()->SetTitle("Compression");
  helper2->GetXaxis()->CenterTitle();
  helper2->GetXaxis()->SetTickSize(0);
  helper2->GetXaxis()->SetLabelSize(0.13);
  helper2->GetXaxis()->SetTitleSize(0.12);
  helper2->GetYaxis()->SetTitle("RNtuple / TTree ");
  helper2->GetYaxis()->SetNdivisions(5);
  helper2->GetYaxis()->SetLabelSize(0.11);
  helper2->GetYaxis()->SetTitleSize(0.12);
  helper2->GetYaxis()->SetTitleOffset(0.35);

  pad_throughput->cd();
  gPad->SetGridy();
  gPad->SetGridx();

  TGraphErrors *graph_throughput = graph_map[kGraphTreeOpt].graph;
  graph_throughput->SetLineColor(12);
  graph_throughput->SetMarkerColor(12);
  graph_throughput->SetFillColor(graph_map[kGraphTreeOpt].color);
  helper->Draw();

  graph_throughput->Draw("B");
  graph_throughput->Draw("P");
  for (auto g : graph_map) {
    if (g.first == kGraphTreeOpt) continue;
    if (g.first == kGraphRatio) continue;
    g.second.graph->SetLineColor(12);
    g.second.graph->SetMarkerColor(12);
    g.second.graph->SetFillColor(graph_map[g.first].color);
    g.second.graph->Draw("B");
    g.second.graph->Draw("P");
  }

  TLegend *leg = new TLegend(0.8, 0.6, 0.925, 0.9);
  //TLegend *leg = new TLegend(0.95, 0.95, 0.7, 0.8);
  leg->SetHeader("Optimised");
  leg->AddEntry(graph_map[kGraphTreeOpt].graph, "TTree", "F");
  leg->AddEntry(graph_map[kGraphNtupleOpt].graph, "RNTuple", "F");
  leg->SetTextSize(0.05);
  leg->Draw();

  pad_ratio->cd();
  gPad->SetGridy();
  gPad->SetGridx();

  TGraphErrors *graph_ratio = graph_map[kGraphRatio].graph;
  graph_ratio->SetLineColor(12);
  graph_ratio->SetMarkerColor(12);
  graph_ratio->SetFillColor(graph_map[kGraphRatio].color);
  helper2->Draw();
  graph_ratio->Draw("B");
  graph_ratio->Draw("P");  // show error bars within bars


  //canvas->SetLogy();

  auto output = TFile::Open(output_path, "RECREATE");
  output->cd();
  canvas->Write();
  output->Close();
}
