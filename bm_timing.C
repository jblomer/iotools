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
    //error *= 1.5;  // safety margin
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
  float max_throughput = 0.0;
  bool has_rdf = false;
  std::vector<EnumCompression> ratio_bins;
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
    max_throughput = std::max(max_throughput, throughput_val + throughput_err);
    cout << format << " " << throughput_val << " " << throughput_err << endl;

    if (!graph_map[props_map[format].type].is_direct)
      has_rdf = true;
    TGraphErrors *graph_throughput = graph_map[props_map[format].type].graph;
    graph_throughput->SetPoint(step, step + 0.5, throughput_val);
    graph_throughput->SetPointError(step, 0, throughput_err);
    for (auto g : graph_map) {
      if (g.first == props_map[format].type) continue;
      if (g.second.is_ratio) continue;
      g.second.graph->SetPoint(step, step + 0.5, -1);
      g.second.graph->SetPointError(step, 0, 0);
    }

    // Ratio plots
    if (step % 2 == 1) {
      auto ratio_val = throughput_val / prev_val;
      auto ratio_err = ratio_val *
      sqrt(throughput_err * throughput_err / throughput_val / throughput_val +
           prev_err * prev_err / prev_val / prev_val);
      max_ratio = std::max(max_ratio, ratio_val + ratio_err);
      ratio_bins.push_back(props_map[format].compression);

      TGraphErrors *active_graph = nullptr;
      TGraphErrors *shadow_graph = nullptr;
      if (graph_map[props_map[format].type].is_direct) {
        active_graph = graph_map[kGraphRatioDirect].graph;
        shadow_graph = graph_map[kGraphRatioRdf].graph;
      } else {
        active_graph = graph_map[kGraphRatioRdf].graph;
        shadow_graph = graph_map[kGraphRatioDirect].graph;
      }
      active_graph->SetPoint(step / 2, step / 2 + 0.5, ratio_val);
      active_graph->SetPointError(step / 2, 0, ratio_err);
      shadow_graph->SetPoint(step / 2, step / 2 + 0.5, -1);
      shadow_graph->SetPointError(step / 2, 0, 0);
    }

    step++;
    prev_val = throughput_val;
    prev_err = throughput_err;
  }
  auto nGraphs = step;
  auto nGraphsPerBlock = has_rdf ? 4 : 2;

  if (limit_y < 0)
    limit_y = max_throughput * 1.05;
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
  pad_ratio->SetTopMargin(0.);
  pad_ratio->SetBottomMargin(0.26);
  pad_ratio->SetLeftMargin(0.1);
  pad_ratio->SetRightMargin(0.055);
  pad_ratio->Draw();
  canvas->cd();

  TH1F * helper = new TH1F("", "", nGraphs, 0, nGraphs);
  helper->GetXaxis()->SetTitle("");
  helper->GetXaxis()->SetNdivisions(0);
  helper->GetXaxis()->SetLabelSize(0);
  helper->GetXaxis()->SetTickSize(0);
  helper->GetYaxis()->SetTitle(ytitle);
  helper->GetYaxis()->SetTickSize(0.01);
  helper->GetYaxis()->SetLabelSize(0.07);
  helper->GetYaxis()->SetTitleSize(0.07);
  helper->GetYaxis()->SetTitleOffset(0.58);
  helper->SetMinimum(0);
  helper->SetMaximum(limit_y);
  helper->SetTitle(title);

  TH1F *helper2 = new TH1F("", "", ratio_bins.size(), 0, ratio_bins.size());
  max_ratio *= 1.05;
  helper2->SetMinimum(0);
  helper2->SetMaximum(max_ratio);
  for (unsigned i = 0; i < ratio_bins.size(); ++i) {
    helper2->GetXaxis()->SetBinLabel(i + 1, kCompressionNames[ratio_bins[i]]);
  }
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

  pad_throughput->cd();
  gPad->SetGridy();

  helper->Draw();
  for (auto g : graph_map) {
    if (g.second.is_ratio) continue;
    g.second.graph->SetLineColor(12);
    g.second.graph->SetMarkerColor(12);
    g.second.graph->SetFillColor(graph_map[g.first].color);
    g.second.graph->SetFillStyle(graph_map[g.first].shade);
    g.second.graph->SetLineWidth(2);
    g.second.graph->Draw("B");
    g.second.graph->Draw("P");
  }

  for (unsigned i = nGraphsPerBlock; i < nGraphs; i += nGraphsPerBlock) {
    TLine *line = new TLine(i, 0, i, limit_y);
    line->SetLineColor(kBlack);
    line->SetLineStyle(2);
    line->Draw();
  }

  TLegend *leg;
  if (has_rdf) {
    leg = new TLegend(0.6, 0.65, 0.9, 0.9);
    leg->SetNColumns(2);
    leg->SetHeader("Direct                      RDataFrame");
    leg->AddEntry(graph_map[kGraphTreeDirect].graph,   "TTree",   "f");
    leg->AddEntry(graph_map[kGraphTreeRdf].graph,      "TTree",   "f");
    leg->AddEntry(graph_map[kGraphNtupleDirect].graph, "RNTuple", "f");
    leg->AddEntry(graph_map[kGraphNtupleRdf].graph,    "RNTuple", "f");
  } else {
    leg = new TLegend(0.8, 0.7, 0.9, 0.9);
    leg->AddEntry(graph_map[kGraphTreeDirect].graph,   "TTree",   "f");
    leg->AddEntry(graph_map[kGraphNtupleDirect].graph, "RNTuple", "f");
  }
  leg->SetBorderSize(1);
  leg->SetTextSize(0.05);
  leg->Draw();
  TText l;
  l.SetTextSize(0.05);
  l.SetTextAlign(33);
  l.DrawTextNDC(0.9, 0.70 - 0.01, "95% CL");

  pad_ratio->cd();
  gPad->SetGridy();

  helper2->Draw();
  for (auto g : graph_map) {
    if (!g.second.is_ratio) continue;
    g.second.graph->SetLineColor(12);
    g.second.graph->SetMarkerColor(12);
    g.second.graph->SetFillColor(graph_map[g.first].color);
    g.second.graph->SetFillStyle(graph_map[g.first].shade);
    g.second.graph->SetLineWidth(2);
    g.second.graph->Draw("B");
    g.second.graph->Draw("P");  // show error bars within bars
  }

  for (unsigned i = nGraphsPerBlock / 2; i < nGraphs / 2; i += nGraphsPerBlock / 2) {
    TLine *line = new TLine(i, 0, i, max_ratio);
    line->SetLineColor(kBlack);
    line->SetLineStyle(2);
    line->Draw();
  }

  TLegend *legr = new TLegend(0.675, 0.8, 0.9, 0.95);
  legr->SetNColumns(2);
  legr->SetBorderSize(1);
  legr->AddEntry(graph_map[kGraphRatioDirect].graph, "Direct",     "f");
  legr->AddEntry(graph_map[kGraphRatioRdf].graph,    "RDataFrame", "f");
  legr->SetTextSize(0.075);
  if (has_rdf)
    legr->Draw();

  //for (unsigned i = 0; i < ratio_bins.size(); i += nGraphsPerBlock / 2) {
  //  TText l;
  //  l.SetTextAlign(22);
  //  l.SetTextSize(0.075);
  //  l.DrawText(float(i) + float(nGraphsPerBlock) / 4., gPad->YtoPad(-(max_ratio / 10)), kCompressionNames[ratio_bins[i]]);
  //}

  //canvas->SetLogy();

  auto output = TFile::Open(output_path, "RECREATE");
  output->cd();
  canvas->Write();
  std::string pdf_path = output_path.View().to_string();
  canvas->Print(TString(pdf_path.substr(0, pdf_path.length() - 4) + "pdf"));
  output->Close();
}
