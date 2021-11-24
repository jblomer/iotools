#include "cms_ttree.h"
#include "util_arrow.h"

#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>

#include <cassert>
#include <cstdio>
#include <memory>
#include <unistd.h>
#include <unordered_map>

static const std::unordered_map<std::string, parquet::Compression::type> g_name2codec{
  {"none", parquet::Compression::UNCOMPRESSED},
  {"zstd", parquet::Compression::ZSTD},
};

/// Chunk size for `parquet::arrow::FileWriter::WriteTable()` defaults to the old RNTuple entries per cluster
constexpr size_t kDefaultWriteTableChunkSize = 64000;

static void Usage(char *progname) {
  printf("Usage: %s -i <input ROOT file> -o <output parquet file> [-c none|zstd] [-s <WriteTable() chunk size>]\n", progname);
}

std::shared_ptr<arrow::Schema> InitSchema() {
  using namespace arrow;
  return schema({
      field("run", int32()),
    });
}

int main(int argc, char **argv) {
  std::string inputPath;
  std::string outputPath;
  std::string compression{"none"};
  std::size_t chunkSize = kDefaultWriteTableChunkSize;

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
        compression = optarg;
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

  parquet::WriterProperties::Builder props;
  props.data_pagesize(64 * 1024); // match RNTuple defaults for page/cluster size 
  props.max_row_group_length(50 * 1000 * 1000);
  props.encoding(parquet::Encoding::PLAIN);
  props.disable_dictionary();
  props.compression(g_name2codec.at(compression));

  auto schema = InitSchema();

  std::shared_ptr<arrow::io::FileOutputStream> outfile;
  PARQUET_ASSIGN_OR_THROW(outfile,
			  arrow::io::FileOutputStream::Open(outputPath));
  std::unique_ptr<parquet::arrow::FileWriter> writer;
  PARQUET_THROW_NOT_OK(parquet::arrow::FileWriter::Open(*schema, arrow::default_memory_pool(), outfile, props.build(),
							&writer));

  ArrowTableBuilder<
    /**/arrow::Int32Builder
    > tblBuilder(schema,
		 chunkSize,
		 [&writer,chunkSize](std::shared_ptr<arrow::Table> table) { writer->WriteTable(*table, chunkSize); });

  size_t nEvent = 0;
  while (reader.NextEvent()) {
    const auto &row = reader.fEvent;
    //tblBuilder << std::make_tuple();

    if (reader.fPos % 100000 == 0)
      printf(" ... processed %d events\n", reader.fPos);
    nEvent++;
  }
  printf("[done] processed %d events\n", reader.fPos);
  return 0;
}
