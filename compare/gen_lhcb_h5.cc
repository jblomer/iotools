#include "util.h"

#include <h5hep/h5hep.hxx>

#include <cassert>
#include <cstdio>
#include <memory>
#include <unistd.h>

/// Chunk size defaults to RNTuple default page size divided by sizeof(double)
constexpr size_t kDefaultChunkSize = (64 * 1024) / sizeof(double);

static void Usage(char *progname) {
  printf("Usage: %s -i <input ROOT file> -o <output hdf5 file> [-c <compression level>] [-s <chunk size>]\n", progname);
}

/// __COLUMN_MODEL__ is defined via -D compiler option (see Makefile) 
using Builder = h5hep::schema::SchemaBuilder<__COLUMN_MODEL__>;

auto InitSchema() {
  return Builder::MakeStructNode<LhcbEvent>("LhcbEvent", {
      Builder::MakePrimitiveNode<double>("B_FlightDistance", HOFFSET(LhcbEvent, B_FlightDistance)),
      Builder::MakePrimitiveNode<double>("B_VertexChi2", HOFFSET(LhcbEvent, B_VertexChi2)),
      Builder::MakePrimitiveNode<double>("H1_PX", HOFFSET(LhcbEvent, H1_PX)),
      Builder::MakePrimitiveNode<double>("H1_PY", HOFFSET(LhcbEvent, H1_PY)),
      Builder::MakePrimitiveNode<double>("H1_PZ", HOFFSET(LhcbEvent, H1_PZ)),
      Builder::MakePrimitiveNode<double>("H1_ProbK", HOFFSET(LhcbEvent, H1_ProbK)),
      Builder::MakePrimitiveNode<double>("H1_ProbPi", HOFFSET(LhcbEvent, H1_ProbPi)),
      Builder::MakePrimitiveNode<int>("H1_Charge", HOFFSET(LhcbEvent, H1_Charge)),
      Builder::MakePrimitiveNode<int>("H1_isMuon", HOFFSET(LhcbEvent, H1_isMuon)),
      Builder::MakePrimitiveNode<double>("H1_IpChi2", HOFFSET(LhcbEvent, H1_IpChi2)),
      Builder::MakePrimitiveNode<double>("H2_PX", HOFFSET(LhcbEvent, H2_PX)),
      Builder::MakePrimitiveNode<double>("H2_PY", HOFFSET(LhcbEvent, H2_PY)),
      Builder::MakePrimitiveNode<double>("H2_PZ", HOFFSET(LhcbEvent, H2_PZ)),
      Builder::MakePrimitiveNode<double>("H2_ProbK", HOFFSET(LhcbEvent, H2_ProbK)),
      Builder::MakePrimitiveNode<double>("H2_ProbPi", HOFFSET(LhcbEvent, H2_ProbPi)),
      Builder::MakePrimitiveNode<int>("H2_Charge", HOFFSET(LhcbEvent, H2_Charge)),
      Builder::MakePrimitiveNode<int>("H2_isMuon", HOFFSET(LhcbEvent, H2_isMuon)),
      Builder::MakePrimitiveNode<double>("H2_IpChi2", HOFFSET(LhcbEvent, H2_IpChi2)),
      Builder::MakePrimitiveNode<double>("H3_PX", HOFFSET(LhcbEvent, H3_PX)),
      Builder::MakePrimitiveNode<double>("H3_PY", HOFFSET(LhcbEvent, H3_PY)),
      Builder::MakePrimitiveNode<double>("H3_PZ", HOFFSET(LhcbEvent, H3_PZ)),
      Builder::MakePrimitiveNode<double>("H3_ProbK", HOFFSET(LhcbEvent, H3_ProbK)),
      Builder::MakePrimitiveNode<double>("H3_ProbPi", HOFFSET(LhcbEvent, H3_ProbPi)),
      Builder::MakePrimitiveNode<int>("H3_Charge", HOFFSET(LhcbEvent, H3_Charge)),
      Builder::MakePrimitiveNode<int>("H3_isMuon", HOFFSET(LhcbEvent, H3_isMuon)),
      Builder::MakePrimitiveNode<double>("H3_IpChi2", HOFFSET(LhcbEvent, H3_IpChi2)),
    });
}

int main(int argc, char **argv) {
  std::string inputPath;
  std::string outputPath;
  unsigned compressionLevel = 0;
  std::size_t chunkSize = kDefaultChunkSize;

  int c;
  while ((c = getopt(argc, argv, "hvi:o:c:s:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        inputPath = optarg;
        break;
      case 'o':
        outputPath = optarg;
        break;
      case 'c':
        compressionLevel = std::atoi(optarg);
        break;
      case 's':
        chunkSize = std::atoi(optarg);
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }
  assert(!inputPath.empty() && !outputPath.empty());
  printf("Converting %s --> %s\n", inputPath.c_str(), outputPath.c_str());

  EventReaderRoot reader;
  reader.Open(inputPath);

  h5hep::WriteProperties props;
  props.SetChunkSize(chunkSize);
  props.SetCompressionLevel(compressionLevel);

  auto schema = InitSchema();
  auto file = h5hep::H5File::Create(outputPath);
  // Use the default RNTuple cluster size as the size for HDF5 chunk cache
  std::static_pointer_cast<h5hep::H5File>(file)->SetCache(50 * 1000 * 1000);
  auto writer = Builder::MakeReaderWriter(file, schema, props);

  h5hep::BufferedWriter<LhcbEvent> bw(writer);
  size_t nEvent = 0;
  while (reader.NextEvent()) {
    const auto &row = reader.fEvent;
    bw.Write(row);
    
    if (reader.fPos % 100000 == 0)
      printf(" ... processed %d events\n", reader.fPos);
    nEvent++;
  }
  printf("[done] processed %d events\n", reader.fPos);
  return 0;
}
