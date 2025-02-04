#include <ROOT/RNTupleExporter.hxx>
#include <ROOT/RPageStorage.hxx>
#include <ROOT/RLogger.hxx>
#include <cstdio>

using namespace ROOT::Experimental;
using namespace ROOT::Experimental::Internal;

int main(int argc, char **argv)
{
  if (argc != 4) {
    printf("Usage: %s <output_dir> <input_file> <ntuple_name>\n", argv[0]);
    return 1;
  }

  const char *output_dir = argv[1];
  const char *input_file = argv[2];
  const char *ntuple_name = argv[3];

  auto source = RPageSource::Create(ntuple_name, input_file);

  ROOT::RLogScopedVerbosity verbosity(ROOT::ELogLevel::kInfo);
  
  RNTupleExporter::RPagesOptions opts {};
  opts.fOutputPath = output_dir;
  // NOTE: If you want to filter out some specific columns, do something like this:
  // opts.fColumnTypeFilter.fType = RNTupleExporter::EFilterType::kWhitelist;
  // opts.fColumnTypeFilter.fSet.insert(EColumnType::kReal64);
  RNTupleExporter::ExportPages(*source, opts);

  return 0;
}
