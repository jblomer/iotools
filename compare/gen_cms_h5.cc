#include "cms_ttree.h"

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
  return Builder::MakeStructNode<CmsEvent>("CmsEvent", {
      Builder::MakePrimitiveNode<int>("run", HOFFSET(CmsEvent, run)),
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

  h5hep::BufferedWriter<CmsEvent> bw(writer);
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
