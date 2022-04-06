/**
 * Copyright CERN; j.lopez@cern.ch
 */

#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RNTupleSerialize.hxx>
#include <ROOT/RPageStorage.hxx>

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unistd.h>

using RColumnDescriptor = ROOT::Experimental::RColumnDescriptor;
using RFieldDescriptor = ROOT::Experimental::RFieldDescriptor;
using RNTupleDescriptor = ROOT::Experimental::RNTupleDescriptor;
using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;
using RNTupleSerializer = ROOT::Experimental::Internal::RNTupleSerializer;
using RPageSource = ROOT::Experimental::Detail::RPageSource;
using RPageStorage = ROOT::Experimental::Detail::RPageStorage;
using RClusterIndex = ROOT::Experimental::RClusterIndex;

/// \brief Helper class to dump RNTuple pages / metadata to separate files.
///
class RNTupleDumper {
   std::unique_ptr<RPageSource> fSource;

   struct RColumnInfo {
      const RColumnDescriptor &fColumnDesc;
      const RFieldDescriptor &fFieldDesc;
      const std::string fQualName;

      RColumnInfo(const RColumnDescriptor &columnDesc, const RFieldDescriptor &fieldDesc)
        : fColumnDesc(columnDesc), fFieldDesc(fieldDesc),
          fQualName(fieldDesc.GetFieldName() + "-" + std::to_string(columnDesc.GetIndex())) {}
   };

   void AddColumnsFromField(std::vector<RColumnInfo> &vec,
                            const RNTupleDescriptor &desc, const RFieldDescriptor &fieldDesc) {
      for (const auto &F : desc.GetFieldIterable(fieldDesc)) {
         for (const auto &C : desc.GetColumnIterable(F)) {
	   vec.emplace_back(C, F);
         }
         AddColumnsFromField(vec, desc, F);
      }
   }
public:
   RNTupleDumper(std::unique_ptr<RPageSource> source)
     : fSource(std::move(source)) {}

   /// Recursively collect all the columns for all the fields rooted at field zero.
   std::vector<RColumnInfo> CollectColumns() {
      auto desc = fSource->GetSharedDescriptorGuard();
      std::vector<RColumnInfo> columns;
      AddColumnsFromField(columns, desc.GetRef(),
                          desc->GetFieldDescriptor(desc->GetFieldZeroId()));
      return columns;
   }

   /// Iterate over all the clusters and dump the contents of each page for each column.
   /// Generated file names follow the template `filenameTmpl` and are placed in directory `outputPath`.
   /// TODO(jalopezg): format filenames according to the provided template
   void DumpPages(const std::vector<RColumnInfo> &columns,
		  const std::string &outputPath, const std::string &/*filenameTmpl*/) {
      auto desc = fSource->GetSharedDescriptorGuard();
      std::uint64_t count = 0;
      for (const auto &Cluster : desc->GetClusterIterable()) {
         printf("\rDumping pages... [%lu / %lu clusters processed]", count++, desc->GetNClusters());
         for (const auto &Col : columns) {
            auto columnId = Col.fColumnDesc.GetId();
            if (!Cluster.ContainsColumn(columnId))
               continue;

            const auto &pages = Cluster.GetPageRange(columnId);
            size_t idx = 0, page_nr = 0;
            for (auto &PI : pages.fPageInfos) {
               RClusterIndex index(Cluster.GetId(), idx);
               RPageStorage::RSealedPage sealedPage;
               fSource->LoadSealedPage(columnId, index, sealedPage);
               auto buffer = std::make_unique<unsigned char[]>(sealedPage.fSize);
               sealedPage.fBuffer = buffer.get();
               fSource->LoadSealedPage(columnId, index, sealedPage);
               {
                  std::ostringstream oss(outputPath, std::ios_base::ate);
                  oss << "/cluster" << Cluster.GetId()
                      << "_" << Col.fQualName << "_pg" << page_nr++ << ".page";
                  std::ofstream of(oss.str(), std::ios_base::binary);
                  of.write(reinterpret_cast<const char *>(sealedPage.fBuffer), sealedPage.fSize);
               }
               idx += PI.fNElements;
            }
         }
      }
      printf("\nDumped data in %lu clusters!\n", count);
   }

   /// Dump ntuple header and footer to separate files.
   void DumpMetadata(const std::string &outputPath) {
      printf("Dumping ntuple metadata...\n");

      auto desc = fSource->GetSharedDescriptorGuard();
      auto context = RNTupleSerializer::SerializeHeaderV1(nullptr, desc.GetRef());
      auto szHeader = context.GetHeaderSize();
      auto headerBuffer = std::make_unique<unsigned char[]>(szHeader);
      context = RNTupleSerializer::SerializeHeaderV1(headerBuffer.get(), desc.GetRef());
      {
         std::ofstream of(outputPath + "/header", std::ios_base::binary);
         of.write(reinterpret_cast<char *>(headerBuffer.get()), szHeader);
      }

      // TODO(jalopezg): should also call `RNTupleSerializer::SerializePageListV1`
      auto szFooter = RNTupleSerializer::SerializeFooterV1(nullptr, desc.GetRef(), context);
      auto footerBuffer = std::make_unique<unsigned char[]>(szFooter);
      RNTupleSerializer::SerializeFooterV1(footerBuffer.get(), desc.GetRef(), context);
      {
         std::ofstream of(outputPath + "/footer", std::ios_base::binary);
         of.write(reinterpret_cast<char *>(footerBuffer.get()), szFooter);
      }
   }
};

enum EDumpFlags {
   kDumpNone = 0,
   kDumpPages = 0x01,
   kDumpMetadata = 0x02,
};

[[noreturn]]
static void Usage(char *argv0) {
   printf("Usage: %s [-p] [-m] [-o output-path] file-name ntuple-name\n\n", argv0);
   printf("Options:\n");
   printf("  -p\t\tDump pages for all the columns\n");
   printf("  -m\t\tDump ntuple metadata\n");
   printf("  -o output-path\tGenerated files will be written to output-path (defaults to `./`)\n");
   printf("\nAt least one of `-p` or `-m` is required.\n");
   exit(0);
}

int main(int argc, char *argv[]) {
   std::string inputFile;
   std::string ntupleName;
   std::string outputPath{"./"};
   std::string filenameTmpl{"cluster%d_%s_pg%d.page"};
   unsigned dumpFlags = kDumpNone;

   int c;
   while ((c = getopt(argc, argv, "hpmo:")) != -1) {
      switch (c) {
      case 'p':
	 dumpFlags |= kDumpPages;
         break;
      case 'm':
	 dumpFlags |= kDumpMetadata;
         break;
      case 'o':
	 outputPath = optarg;
         break;
      case 'h':
      default:
         Usage(argv[0]);
      }
   }
   if ((argc - optind) != 2 || dumpFlags == kDumpNone)
      Usage(argv[0]);

   auto source = RPageSource::Create(argv[optind + 1], argv[optind], RNTupleReadOptions());
   source->Attach();

   RNTupleDumper dumper(std::move(source));
   if (dumpFlags & kDumpMetadata) {
      dumper.DumpMetadata(outputPath);
   }
   if (dumpFlags & kDumpPages) {
      auto columns = dumper.CollectColumns();
      for (const auto &C : columns) {
         printf("Column %lu: %s[%lu]\n", (unsigned long)C.fColumnDesc.GetId(),
                                         C.fFieldDesc.GetFieldName().c_str(),
                                         (unsigned long)C.fColumnDesc.GetIndex());
      }
      dumper.DumpPages(columns, outputPath, filenameTmpl);
   }

   return 0;
}
