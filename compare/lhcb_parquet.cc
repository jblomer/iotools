#include "parquet_schema.hh"

#include <arrow/io/file.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <string>
#include <chrono>
#include <iostream>
#include <future>

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>

constexpr double kKaonMassMeV = 493.677;

static void Show(TH1D *h) {
  new TApplication("", nullptr, nullptr);

  gStyle->SetTextFont(42);
  auto c = new TCanvas("c", "", 800, 700);
  h->GetXaxis()->SetTitle("m_{KKK} [MeV/c^{2}]");
  h->DrawCopy();
  c->Modified();

  std::cout << "press ENTER to exit..." << std::endl;
  auto future = std::async(std::launch::async, getchar);
  while (true) {
    gSystem->ProcessEvents();
    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
      break;
  }
}

static double GetP2(double px, double py, double pz)
{
   return px*px + py*py + pz*pz;
}

static double GetKE(double px, double py, double pz)
{
   double p2 = GetP2(px, py, pz);
   return sqrt(p2 + kKaonMassMeV*kKaonMassMeV);
}

static void Usage(char *progname) {
  printf("Usage: %s -i <input parquet file> [-s]\n", progname);
}

int main(int argc, char **argv) {
  std::string inputPath;
  bool show = false;

  int c;
  while ((c = getopt(argc, argv, "hvi:s")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        inputPath = optarg;
        break;
      case 's':
	show = true;
	break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }
  assert(!inputPath.empty());

  auto ts_init = std::chrono::steady_clock::now();

  std::shared_ptr<arrow::io::ReadableFile> infile;
  PARQUET_ASSIGN_OR_THROW(infile,
			  arrow::io::ReadableFile::Open(inputPath));
  parquet::StreamReader os{parquet::ParquetFileReader::Open(infile)};

  LhcbEvent row;
  auto hMass = new TH1D("B_mass", "", 500, 5050, 5500);
  
  std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
  size_t i = 1;
  while (!os.eof()) {
    if (i && (i++ % 100000 == 0))
      printf("  ... processed %lu events\n", i);

    os >> row;

    if (row.H1_isMuon || row.H2_isMuon || row.H3_isMuon) {
      continue;
    }

    constexpr double prob_k_cut = 0.5;
    if (row.H1_ProbK < prob_k_cut) continue;
    if (row.H2_ProbK < prob_k_cut) continue;
    if (row.H3_ProbK < prob_k_cut) continue;

    constexpr double prob_pi_cut = 0.5;
    if (row.H1_ProbPi > prob_pi_cut) continue;
    if (row.H2_ProbPi > prob_pi_cut) continue;
    if (row.H3_ProbPi > prob_pi_cut) continue;

    double b_px = row.H1_PX + row.H2_PX + row.H3_PX;
    double b_py = row.H1_PY + row.H2_PY + row.H3_PY;
    double b_pz = row.H1_PZ + row.H2_PZ + row.H3_PZ;
    double b_p2 = GetP2(b_px, b_py, b_pz);
    double k1_E = GetKE(row.H1_PX, row.H1_PY, row.H1_PZ);
    double k2_E = GetKE(row.H2_PX, row.H2_PY, row.H2_PZ);
    double k3_E = GetKE(row.H3_PX, row.H3_PY, row.H3_PZ);
    double b_E = k1_E + k2_E + k3_E;
    double b_mass = sqrt(b_E*b_E - b_p2);
    hMass->Fill(b_mass);
  }
  auto ts_end = std::chrono::steady_clock::now();
  auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
  auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

  std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
  std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;

  if (show)
    Show(hMass);
  delete hMass;

  return 0;
}
