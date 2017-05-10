
void bm_timing(TString dataSet="result_timing") {
  std::ifstream file(Form("%s.txt", dataSet.Data()));
  TString format;
  std::array<float, 3> timings;
  vector<TString> format_vec;
  vector<float> throughput_val_vec;
  vector<float> throughput_err_vec;

  const float nevent = 8556118.;

  std::map<TString, TString> labels_map;
  labels_map["root-inflated"] = "ROOT (inflated)";
  labels_map["root-deflated"] = "ROOT (compressed)";
  labels_map["avro-inflated"] = "Avro (inflated)";
  labels_map["avro-deflated"] = "Avro (compressed)";
  labels_map["h5row"] = "HDF5 (row-wise)";
  labels_map["h5column"] = "HDF5 (column-wise)";
  labels_map["protobuf-inflated"] = "Protobuf (inflated)";
  labels_map["protobuf-deflated"] = "Protobuf (compressed)";
  labels_map["parquet-inflated"] = "Parquet (inflated)";
  labels_map["parquet-deflated"] = "Parquet (compressed)";
  labels_map["sqlite"] = "SQlite";

  TCanvas *canvas = new TCanvas();
  TGraphErrors *graph_throughput = new TGraphErrors();
  //TGraph *graph_throughput = new TGraph();

  int step = 0;
  while (file >> format >> timings[0] >> timings[1] >> timings[2]) {
    format_vec.push_back(format);

    float mean = 0.0;
    for (auto t : timings)
      mean += t;
    mean /= timings.size();
    float max = *std::max_element(timings.begin(), timings.end());
    float min = *std::min_element(timings.begin(), timings.end());

    float throughput_val = nevent/mean;
    throughput_val_vec.push_back(throughput_val);
    float max_throughput = nevent/min;
    float min_throughput = nevent/max;
    float throughput_err = (max_throughput - min_throughput) / 2.;
    throughput_err_vec.push_back(throughput_err);

    cout << format << " " << throughput_val << " " << throughput_err << endl;
    graph_throughput->SetPoint(step, step + 1, throughput_val);
    graph_throughput->SetPointError(step, 0, throughput_err);
    step++;
  }

  float max_throughput =
    *std::max_element(throughput_val_vec.begin(), throughput_val_vec.end());

  graph_throughput->SetTitle("Throughput LHCb OpenData ntuple, warm cache");
  graph_throughput->GetXaxis()->SetTitle("File format");
  graph_throughput->GetXaxis()->CenterTitle();
  graph_throughput->GetXaxis()->SetTickSize(0);
  graph_throughput->GetXaxis()->SetLabelSize(0);
  graph_throughput->GetYaxis()->SetTitle("# events per second");
  graph_throughput->GetYaxis()->SetRangeUser(1, max_throughput * 1.1);
  graph_throughput->SetFillColor(40);
  graph_throughput->Draw("AB");

  for (unsigned i = 0; i < format_vec.size(); ++i) {
    TText l;
    l.SetTextAlign(12);
    l.SetTextSize(0.04);
    l.SetTextColor(4);  // blue
    l.SetTextAngle(90);
    l.DrawText(i + 1, 500000, labels_map[format_vec[i]]);
  }

  //canvas->SetLogy();

  TFile * output =
    TFile::Open(Form("%s.graph.root", dataSet.Data()), "RECREATE");
  output->cd();
  canvas->Write();
  output->Close();
}
