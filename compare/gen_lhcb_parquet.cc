#include "util.h"
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
      field("B_FlightDistance", float64()),
      field("B_VertexChi2", float64()),
      field("H1_PX", float64()),
      field("H1_PY", float64()),
      field("H1_PZ", float64()),
      field("H1_ProbK", float64()),
      field("H1_ProbPi", float64()),
      field("H1_Charge", int32()),
      field("H1_isMuon", int32()),
      field("H1_IpChi2", float64()),
      field("H2_PX", float64()),
      field("H2_PY", float64()),
      field("H2_PZ", float64()),
      field("H2_ProbK", float64()),
      field("H2_ProbPi", float64()),
      field("H2_Charge", int32()),
      field("H2_isMuon", int32()),
      field("H2_IpChi2", float64()),
      field("H3_PX", float64()),
      field("H3_PY", float64()),
      field("H3_PZ", float64()),
      field("H3_ProbK", float64()),
      field("H3_ProbPi", float64()),
      field("H3_Charge", int32()),
      field("H3_isMuon", int32()),
      field("H3_IpChi2", float64()),
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
    /*B_FlightDistance*/arrow::DoubleBuilder,
    /*B_VertexChi2*/arrow::DoubleBuilder,
    /*H1_PX*/arrow::DoubleBuilder,
    /*H1_PY*/arrow::DoubleBuilder,
    /*H1_PZ*/arrow::DoubleBuilder,
    /*H1_ProbK*/arrow::DoubleBuilder,
    /*H1_ProbPi*/arrow::DoubleBuilder,
    /*H1_Charge*/arrow::Int32Builder,
    /*H1_isMuon*/arrow::Int32Builder,
    /*H1_IpChi2*/arrow::DoubleBuilder,
    /*H2_PX*/arrow::DoubleBuilder,
    /*H2_PY*/arrow::DoubleBuilder,
    /*H2_PZ*/arrow::DoubleBuilder,
    /*H2_ProbK*/arrow::DoubleBuilder,
    /*H2_ProbPi*/arrow::DoubleBuilder,
    /*H2_Charge*/arrow::Int32Builder,
    /*H2_isMuon*/arrow::Int32Builder,
    /*H2_IpChi2*/arrow::DoubleBuilder,
    /*H3_PX*/arrow::DoubleBuilder,
    /*H3_PY*/arrow::DoubleBuilder,
    /*H3_PZ*/arrow::DoubleBuilder,
    /*H3_ProbK*/arrow::DoubleBuilder,
    /*H3_ProbPi*/arrow::DoubleBuilder,
    /*H3_Charge*/arrow::Int32Builder,
    /*H3_isMuon*/arrow::Int32Builder,
    /*H3_IpChi2*/arrow::DoubleBuilder
    > tblBuilder(schema,
		 chunkSize,
		 [&writer,chunkSize](std::shared_ptr<arrow::Table> table) { writer->WriteTable(*table, chunkSize); });

  size_t nEvent = 0;
  while (reader.NextEvent()) {
    const auto &row = reader.fEvent;
    tblBuilder << std::make_tuple(row.B_FlightDistance, row.B_VertexChi2,
				  row.H1_PX, row.H1_PY, row.H1_PZ, row.H1_ProbK, row.H1_ProbPi, row.H1_Charge, row.H1_isMuon, row.H1_IpChi2,
				  row.H2_PX, row.H2_PY, row.H2_PZ, row.H2_ProbK, row.H2_ProbPi, row.H2_Charge, row.H2_isMuon, row.H2_IpChi2,
				  row.H3_PX, row.H3_PY, row.H3_PZ, row.H3_ProbK, row.H3_ProbPi, row.H3_Charge, row.H3_isMuon, row.H3_IpChi2);

    if (reader.fPos % 100000 == 0)
      printf(" ... processed %d events\n", reader.fPos);
    nEvent++;
  }
  printf("[done] processed %d events\n", reader.fPos);
  return 0;
}
