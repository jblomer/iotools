#include "util_arrow.h"

#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <iostream>
#include <string>
#include <unistd.h>

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TSystem.h>

using Int32Array = arrow::Int32Array;
using DoubleArray = arrow::DoubleArray;

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
  std::unique_ptr<parquet::arrow::FileReader> reader;
  PARQUET_THROW_NOT_OK(parquet::arrow::OpenFile(infile, arrow::default_memory_pool(),
						&reader));
  reader->set_use_threads(true);

  std::shared_ptr<arrow::Schema> schema;
  reader->GetSchema(&schema);
  auto columns = GetColumnIndices(schema,
				  {"H1_isMuon", "H2_isMuon", "H3_isMuon",
				   "H1_ProbK", "H2_ProbK", "H3_ProbK",
				   "H1_ProbPi", "H2_ProbPi", "H3_ProbPi",
				   "H1_PX", "H1_PY", "H1_PZ",
				   "H2_PX", "H2_PY", "H2_PZ",
				   "H3_PX", "H3_PY", "H3_PZ"});
  std::shared_ptr<arrow::Table> table;

  auto hMass = new TH1D("B_mass", "", 500, 5050, 5500);

  std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
  size_t count = 0;
  for (size_t row_group = 0, num_row_groups = reader->num_row_groups();
       row_group < num_row_groups; ++row_group) {
    printf("processed %lu k events\n", count / 1000);

    reader->ReadRowGroup(row_group, columns, &table);
    auto H1_isMuon = std::static_pointer_cast<Int32Array>(table->GetColumnByName("H1_isMuon")->chunk(0));
    auto H2_isMuon = std::static_pointer_cast<Int32Array>(table->GetColumnByName("H2_isMuon")->chunk(0));
    auto H3_isMuon = std::static_pointer_cast<Int32Array>(table->GetColumnByName("H3_isMuon")->chunk(0));
    auto H1_ProbK = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H1_ProbK")->chunk(0));
    auto H2_ProbK = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H2_ProbK")->chunk(0));
    auto H3_ProbK = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H3_ProbK")->chunk(0));
    auto H1_ProbPi = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H1_ProbPi")->chunk(0));
    auto H2_ProbPi = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H2_ProbPi")->chunk(0));
    auto H3_ProbPi = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H3_ProbPi")->chunk(0));
    auto H1_PX = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H1_PX")->chunk(0));
    auto H1_PY = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H1_PY")->chunk(0));
    auto H1_PZ = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H1_PZ")->chunk(0));
    auto H2_PX = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H2_PX")->chunk(0));
    auto H2_PY = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H2_PY")->chunk(0));
    auto H2_PZ = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H2_PZ")->chunk(0));
    auto H3_PX = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H3_PX")->chunk(0));
    auto H3_PY = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H3_PY")->chunk(0));
    auto H3_PZ = std::static_pointer_cast<DoubleArray>(table->GetColumnByName("H3_PZ")->chunk(0));

    for (int64_t i = 0; i < table->num_rows(); ++i) {
      if (H1_isMuon->Value(i) || H2_isMuon->Value(i) || H3_isMuon->Value(i)) {
	continue;
      }

      constexpr double prob_k_cut = 0.5;
      if (H1_ProbK->Value(i) < prob_k_cut) continue;
      if (H2_ProbK->Value(i) < prob_k_cut) continue;
      if (H3_ProbK->Value(i) < prob_k_cut) continue;

      constexpr double prob_pi_cut = 0.5;
      if (H1_ProbPi->Value(i) > prob_pi_cut) continue;
      if (H2_ProbPi->Value(i) > prob_pi_cut) continue;
      if (H3_ProbPi->Value(i) > prob_pi_cut) continue;

      double b_px = H1_PX->Value(i) + H2_PX->Value(i) + H3_PX->Value(i);
      double b_py = H1_PY->Value(i) + H2_PY->Value(i) + H3_PY->Value(i);
      double b_pz = H1_PZ->Value(i) + H2_PZ->Value(i) + H3_PZ->Value(i);
      double b_p2 = GetP2(b_px, b_py, b_pz);
      double k1_E = GetKE(H1_PX->Value(i), H1_PY->Value(i), H1_PZ->Value(i));
      double k2_E = GetKE(H2_PX->Value(i), H2_PY->Value(i), H2_PZ->Value(i));
      double k3_E = GetKE(H3_PX->Value(i), H3_PY->Value(i), H3_PZ->Value(i));
      double b_E = k1_E + k2_E + k3_E;
      double b_mass = sqrt(b_E*b_E - b_p2);
      hMass->Fill(b_mass);
    }
    count += table->num_rows();
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
