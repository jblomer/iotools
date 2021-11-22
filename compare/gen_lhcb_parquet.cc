#include "util.h"
#include "parquet_schema.hh"

#include <arrow/io/file.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <string>

static void Usage(char *progname) {
  printf("Usage: %s -i <input ROOT file> -o <output parquet file>\n", progname);
}

int main(int argc, char **argv) {
  std::string inputPath;
  std::string outputPath;

  int c;
  while ((c = getopt(argc, argv, "hvi:o:")) != -1) {
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

  std::shared_ptr<arrow::io::FileOutputStream> outfile;
  PARQUET_ASSIGN_OR_THROW(outfile,
			  arrow::io::FileOutputStream::Open(outputPath));

  auto schema = InitParquetSchema();

  parquet::WriterProperties::Builder builder;
  builder.compression(parquet::Compression::UNCOMPRESSED);
  //builder.compression(parquet::Compression::ZSTD);

  parquet::StreamWriter os{
    parquet::ParquetFileWriter::Open(outfile, schema, builder.build())};

  size_t nEvent = 0;
  while (reader.NextEvent()) {
    if (reader.fPos % 100000 == 0)
      printf(" ... processed %d events\n", reader.fPos);

    os << reader.fEvent;
    nEvent++;
  }
  printf("[done] processed %d events\n", reader.fPos);

  return 0;
}
