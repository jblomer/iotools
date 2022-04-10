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

using Int32Array = arrow::Int32Array;
using FloatArray = arrow::FloatArray;

static void Usage(char *progname) {
  printf("Usage: %s -i <input parquet file>\n", progname);
}

int main(int argc, char **argv) {
  std::string inputPath;

  int c;
  while ((c = getopt(argc, argv, "hvi:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        inputPath = optarg;
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
  // FIXME: this doesn't work for dot paths; hardcode indices instead. List of
  // column indices is retrieved as:
  // `$ parquet-tools inspect FILE.parquet | grep path | nl -v0`
  //
  //auto columns = GetColumnIndices(schema,
  //				  {/*"nMuon",*/ "Muon_eta.list.item", "Muon_phi.list.item",
  //				   "MET_pt",
  //				   /*"nJet",*/ "Jet_pt.list.item", "Jet_eta.list.item", "Jet_phi.list.item", "Jet_mass.list.item", "Jet_btag.list.item"});
  std::vector<int> columns{11, 12, 60, 67, 68, 69, 70, 72};
  std::shared_ptr<arrow::Table> table;

  std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
  size_t count = 0;
  for (size_t row_group = 0, num_row_groups = reader->num_row_groups();
       row_group < num_row_groups; ++row_group) {
    printf("processed %lu k events\n", count / 1000);

    reader->ReadRowGroup(row_group, columns, &table);
    //std::cout << table->ToString();
    assert(table->GetColumnByName("Muon_eta")->num_chunks() == 1);
    assert(table->GetColumnByName("Muon_phi")->num_chunks() == 1);
    assert(table->GetColumnByName("MET_pt")->num_chunks() == 1);
    assert(table->GetColumnByName("Jet_pt")->num_chunks() == 1);
    assert(table->GetColumnByName("Jet_eta")->num_chunks() == 1);
    assert(table->GetColumnByName("Jet_phi")->num_chunks() == 1);
    assert(table->GetColumnByName("Jet_mass")->num_chunks() == 1);
    assert(table->GetColumnByName("Jet_btag")->num_chunks() == 1);

    auto Muon_eta = std::static_pointer_cast<FloatArray>(table->GetColumnByName("Muon_eta")->chunk(0));
    auto Muon_phi = std::static_pointer_cast<FloatArray>(table->GetColumnByName("Muon_phi")->chunk(0));
    auto MET_pt = std::static_pointer_cast<FloatArray>(table->GetColumnByName("MET_pt")->chunk(0));
    auto Jet_pt = std::static_pointer_cast<FloatArray>(table->GetColumnByName("Jet_pt")->chunk(0));
    auto Jet_eta = std::static_pointer_cast<FloatArray>(table->GetColumnByName("Jet_eta")->chunk(0));
    auto Jet_phi = std::static_pointer_cast<FloatArray>(table->GetColumnByName("Jet_phi")->chunk(0));
    auto Jet_mass = std::static_pointer_cast<FloatArray>(table->GetColumnByName("Jet_mass")->chunk(0));
    auto Jet_btag = std::static_pointer_cast<FloatArray>(table->GetColumnByName("Jet_btag")->chunk(0));

    for (int64_t i = 0; i < table->num_rows(); ++i) {
      (void)Muon_eta->Value(i);
      (void)Muon_phi->Value(i);
      (void)MET_pt->Value(i);
      (void)Jet_pt->Value(i);
      (void)Jet_eta->Value(i);
      (void)Jet_phi->Value(i);
      (void)Jet_mass->Value(i);
      (void)Jet_btag->Value(i);
    }
    count += table->num_rows();
  }
  auto ts_end = std::chrono::steady_clock::now();
  auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
  auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

  std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
  std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;

  return 0;
}
