
void bm_convert_to_pdf(TString file_name = "UNKNOWN") {
  TFile *file = TFile::Open(Form("%s.root", file_name.Data()));
  TCanvas *canvas = (TCanvas *)file->Get("MyCanvas");
  canvas->Print(Form("%s.pdf", file_name.Data()));
}
