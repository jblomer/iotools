#include "util_arrow.h"

#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>

#include <cstdio>
#include <memory>
#include <string>
#include <type_traits>

using FloatArray = arrow::FloatArray;

static constexpr size_t kDefaultWriteTableChunkSize = 64000;

std::shared_ptr<arrow::Schema> InitSchema() {
   using namespace arrow;
   return schema({
       field("field_0", float32()),
       field("field_1", float32()),
       field("field_2", float32()),
     });
}

void Hugefile_Read(const std::string &filename) {
   std::shared_ptr<arrow::io::ReadableFile> infile;
   PARQUET_ASSIGN_OR_THROW(infile,
                           arrow::io::ReadableFile::Open(filename));
   std::unique_ptr<parquet::arrow::FileReader> reader;
   PARQUET_THROW_NOT_OK(parquet::arrow::OpenFile(infile, arrow::default_memory_pool(),
                                                 &reader));
   reader->set_use_threads(true);

   std::shared_ptr<arrow::Schema> schema;
   reader->GetSchema(&schema);
   std::vector<int> columns{0, 1, 2};
   std::shared_ptr<arrow::Table> table;

   size_t count = 0;
   for (size_t row_group = 0, num_row_groups = reader->num_row_groups();
        row_group < num_row_groups; ++row_group) {
      reader->ReadRowGroup(row_group, columns, &table);
      auto field0 = std::static_pointer_cast<FloatArray>(table->GetColumnByName("field_0")->chunk(0));
      auto field1 = std::static_pointer_cast<FloatArray>(table->GetColumnByName("field_1")->chunk(0));
      auto field2 = std::static_pointer_cast<FloatArray>(table->GetColumnByName("field_2")->chunk(0));
      for (int64_t i = 0; i < table->num_rows(); ++i) {
         (void)field0->Value(i);
         (void)field1->Value(i);
         (void)field2->Value(i);
         count++;
      }
      printf("Read %lu entries\r", (unsigned long)count);
   }
}

void Hugefile_Write(const std::string &filename, size_t nEntries) {
   parquet::WriterProperties::Builder props;
   props.data_pagesize(64 * 1024); // match RNTuple defaults for page/cluster size
   props.max_row_group_length(50 * 1000 * 1000);
   props.encoding(parquet::Encoding::PLAIN);
   props.disable_dictionary();
   props.compression(parquet::Compression::UNCOMPRESSED);

   auto schema = InitSchema();

   std::shared_ptr<arrow::io::FileOutputStream> outfile;
   PARQUET_ASSIGN_OR_THROW(outfile,
                           arrow::io::FileOutputStream::Open(filename));
   std::unique_ptr<parquet::arrow::FileWriter> writer;
   PARQUET_THROW_NOT_OK(parquet::arrow::FileWriter::Open(*schema, arrow::default_memory_pool(), outfile, props.build(),
                                                         &writer));
   ArrowTableBuilder<
     /*field_0*/arrow::FloatBuilder,
     /*field_1*/arrow::FloatBuilder,
     /*field_2*/arrow::FloatBuilder
     > tblBuilder(schema,
                  kDefaultWriteTableChunkSize,
                  [&writer](std::shared_ptr<arrow::Table> table) { writer->WriteTable(*table, kDefaultWriteTableChunkSize); });
   for (size_t i = 0; i < nEntries; ++i) {
      tblBuilder << std::make_tuple(static_cast<float>(i),
                                    static_cast<float>(i) + 0.1f,
                                    static_cast<float>(i) + 1.1f);
      if (i % 10000 == 0)
         printf("Wrote %lu entries\r", (unsigned long)i);
   }
   printf("Wrote %lu entries\n", (unsigned long)nEntries);
}
