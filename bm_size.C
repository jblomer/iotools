
void bm_size(TString dataSet="result_size") {
  std::ifstream file(Form("%s.txt", dataSet.Data()));
  TString format;
  float size;
  vector<TString> format_vec;
  vector<float> size_vec;

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
  TGraph *graph_size = new TGraph();

  int step = 0;
  while (file >> format >> size) {
    cout << format << " " << size << endl;
    format_vec.push_back(format);
    size_vec.push_back(size);
    graph_size->SetPoint(step, step + 1, size);
    step++;
  }

  float max_size = *std::max_element(size_vec.begin(), size_vec.end());

  graph_size->SetTitle("Data size for LHCb OpenData");
  graph_size->GetXaxis()->SetTitle("File format");
  graph_size->GetXaxis()->CenterTitle();
  graph_size->GetXaxis()->SetTickSize(0);
  graph_size->GetXaxis()->SetLabelSize(0);
  graph_size->GetYaxis()->SetTitle("Size per event [B]");
  graph_size->GetYaxis()->SetRangeUser(0, max_size * 1.1);
  graph_size->SetFillColor(40);
  graph_size->Draw("AB");

  for (unsigned i = 0; i < format_vec.size(); ++i) {
    TText l;
    l.SetTextAlign(12);
    l.SetTextSize(0.04);
    l.SetTextAngle(90);
    l.DrawText(i + 1, 10, labels_map[format_vec[i]]);
  }

  TFile * output =
    TFile::Open(Form("graph_size.root"), "RECREATE");
  output->cd();
  canvas->Write();
  output->Close();
}
