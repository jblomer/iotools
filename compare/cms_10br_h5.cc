#include "cms_event_h5.h"

#include <h5hep/h5hep.hxx>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <iostream>
#include <string>
#include <unistd.h>

static void Usage(char *progname) {
  printf("Usage: %s -i <input HDF5 file>\n", progname);
}

/// __COLUMN_MODEL__ is defined via -D compiler option (see Makefile) 
using Builder = h5hep::schema::SchemaBuilder<__COLUMN_MODEL__>;

auto InitSchema() {
  return Builder::MakeStructNode<CmsEventH5>("CmsEventH5", {
      Builder::MakeCollectionNode("nMuon", HOFFSET(CmsEventH5, nMuon),
				  Builder::MakeStructNode<Muon>("Muon", {
				      Builder::MakePrimitiveNode<float>("Muon_eta", HOFFSET(Muon, Muon_eta)),
				      Builder::MakePrimitiveNode<float>("Muon_phi", HOFFSET(Muon, Muon_phi)),
				    })),
      Builder::MakePrimitiveNode<float>("MET_pt", HOFFSET(CmsEventH5, MET_pt)),
      Builder::MakeCollectionNode("nJet", HOFFSET(CmsEventH5, nJet),
				  Builder::MakeStructNode<Jet>("Jet", {
				      Builder::MakePrimitiveNode<float>("Jet_pt", HOFFSET(Jet, Jet_pt)),
				      Builder::MakePrimitiveNode<float>("Jet_eta", HOFFSET(Jet, Jet_eta)),
				      Builder::MakePrimitiveNode<float>("Jet_phi", HOFFSET(Jet, Jet_phi)),
				      Builder::MakePrimitiveNode<float>("Jet_mass", HOFFSET(Jet, Jet_mass)),
				      Builder::MakePrimitiveNode<float>("Jet_btag", HOFFSET(Jet, Jet_btag)),
				    })),
    });
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

  auto schema = InitSchema();
  auto file = h5hep::H5File::Open(inputPath);
  // Use the default RNTuple cluster size as the size for HDF5 chunk cache
  std::static_pointer_cast<h5hep::H5File>(file)->SetCache(50 * 1000 * 1000);
  auto reader = Builder::MakeReaderWriter(file, schema);

  auto num_chunks = reader->GetNChunks();
  auto chunk = std::make_unique<CmsEventH5[]>(reader->GetWriteProperties().GetChunkSize());

  std::chrono::steady_clock::time_point ts_first = std::chrono::steady_clock::now();
  size_t count = 0;
  for (size_t chunkIdx = 0; chunkIdx < num_chunks; ++chunkIdx) {
    printf("processed %lu k events\n", count / 1000);

    auto num_rows = reader->ReadChunk(chunkIdx, chunk.get());
    //for (size_t i = 0; i < num_rows; ++i) {
    //  const auto &row = chunk[i];
    //}
    count += num_rows;
  }
  auto ts_end = std::chrono::steady_clock::now();
  auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
  auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

  std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
  std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;

  return 0;
}
