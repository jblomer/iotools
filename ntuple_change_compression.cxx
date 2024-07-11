/*
  ntuple_change_compression

  A small utility used to test the capability of RNTupleMerger to change compression of its inputs.
  Accepts one or more ROOT input files containing one or more RNTuples and outputs one file with
  all the RNTuples merged with possibly changed compression algorithm and level.

  @author Giacomo Parolini, 2024
*/
#include <ROOT/RLogger.hxx>
#include <ROOT/RNTupleReader.hxx>
#include <ROOT/RNTupleMerger.hxx>
#include <ROOT/RPageStorageFile.hxx>
#include <TFile.h>
#include <TROOT.h>
#include <ROOT/RNTupleWriteOptions.hxx>
#include <iostream>

using namespace ROOT::Experimental;
using namespace ROOT::Experimental::Internal;
using namespace std::chrono;

int main(int argc, char **argv) {
  if (argc < 5) {
    fprintf(stderr, "Usage: %s <compression_settings> <ntuple_file_out> <ntuple_name> <ntuple_file1.root> [ntuple_file2.root ...]\n", argv[0]);
    fprintf(stderr, "Common compression settings:\n\t-1: preserve;\n\t0: uncompressed\n\t505: Zstd\n\t207: LZMA\n");
    return 1;
  }

#ifdef R__USE_IMT
  ROOT::EnableImplicitMT();
#endif

  auto noWarn = RLogScopedVerbosity(NTupleLog(), ELogLevel::kError);

  const int compSettings = std::atoi(argv[1]);
  const char *ntuple_file_out = argv[2];
  const char *ntuple_name = argv[3];
  std::vector<const char *> ntuple_files;
  for (int i = 4; i < argc; ++i)
    ntuple_files.push_back(argv[i]);

  std::vector<std::unique_ptr<RPageSource>> srcs;
  std::vector<RPageSource *> srcsRaw;
  srcs.reserve(ntuple_files.size());
  srcsRaw.reserve(ntuple_files.size());

  for (const char *ntuple_file : ntuple_files) {
    auto file = std::unique_ptr<TFile>(TFile::Open(ntuple_file));
    if (!file)
      return 1;

    auto *anchor = file->Get<RNTuple>(ntuple_name);
    if (!anchor) {
      std::cerr << "Error reading RNTuple " << ntuple_name << " from " << ntuple_file << ": skipping.\n";
      continue;
    }
    auto src = RPageSourceFile::CreateFromAnchor(*anchor);
    auto &s = srcs.emplace_back(std::move(src));
    srcsRaw.push_back(s.get());
  }

  auto dst = RPageSinkFile { ntuple_name, ntuple_file_out, RNTupleWriteOptions {} };

  RNTupleMerger merger;
  RNTupleMergeOptions merge_opts;
  merge_opts.fCompressionSettings = compSettings;
  merger.Merge(srcsRaw, dst, merge_opts);

  std::cout << "Merged " << srcsRaw.size() << " ntuples.";
  if (compSettings != -1)
    std::cout << " Compression changed to " << compSettings << ".";
  std::cout << "\nOut file is " << ntuple_file_out << "\n";

  return 0;
}
