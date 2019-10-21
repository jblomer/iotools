
void bm_convert_to_pdf(TString file_name = "UNKNOWN") {
  TFile *file = TFile::Open(Form("%s.root", file_name.Data()));
  auto canvas = file->Get<TCanvas>("MyCanvas");
  canvas->Print(Form("%s.pdf", file_name.Data()));
}
