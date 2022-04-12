#include <h5hep/h5hep.hxx>

#include <cstdio>
#include <memory>
#include <string>

struct Entry {
   float field0;
   float field1;
   float field2;
};

using Builder = h5hep::schema::SchemaBuilder<__COLUMN_MODEL__>;

/// Chunk size defaults to RNTuple default page size divided by sizeof(float)
static constexpr size_t kDefaultChunkSize = (64 * 1024) / sizeof(float);

auto InitSchema() {
   return Builder::MakeStructNode<Entry>("Entry", {
       Builder::MakePrimitiveNode<float>("field_0", HOFFSET(Entry, field0)),
       Builder::MakePrimitiveNode<float>("field_1", HOFFSET(Entry, field1)),
       Builder::MakePrimitiveNode<float>("field_2", HOFFSET(Entry, field2)),
     });
}

void Hugefile_Read(const std::string &filename) {
   auto schema = InitSchema();
   auto file = h5hep::H5File::Open(filename);
   // Use the default RNTuple cluster size as the size for HDF5 chunk cache
   std::static_pointer_cast<h5hep::H5File>(file)->SetCache(50 * 1000 * 1000);
   auto reader = Builder::MakeReaderWriter(file, schema);

   auto num_chunks = reader->GetNChunks();
   auto chunk = std::make_unique<Entry[]>(reader->GetWriteProperties().GetChunkSize());

   size_t count = 0;
   for (size_t chunkIdx = 0; chunkIdx < num_chunks; ++chunkIdx) {
      auto num_rows = reader->ReadChunk(chunkIdx, chunk.get());
      count += num_rows;
      printf("Read %lu entries\r", (unsigned long)count);
   }
}

void Hugefile_Write(const std::string &filename, size_t nEntries) {
   h5hep::WriteProperties props;
   props.SetChunkSize(kDefaultChunkSize);
   props.SetCompressionLevel(0);

   auto schema = InitSchema();
   auto file = h5hep::H5File::Create(filename);
   // Use the default RNTuple cluster size as the size for HDF5 chunk cache
   std::static_pointer_cast<h5hep::H5File>(file)->SetCache(50 * 1000 * 1000);
   auto writer = Builder::MakeReaderWriter(file, schema, props);

   h5hep::BufferedWriter<Entry> bw(writer);
   Entry row;
   for (size_t i = 0; i < nEntries; ++i) {
      row.field0 = static_cast<float>(i);
      row.field1 = static_cast<float>(i) + 0.1f;
      row.field2 = static_cast<float>(i) + 1.1f;
      bw.Write(row);
      if (i % 10000 == 0)
         printf("Wrote %lu entries\r", (unsigned long)i);
   }
   printf("Wrote %lu entries\n", (unsigned long)nEntries);
}
