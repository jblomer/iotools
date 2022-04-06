/**
 * Copyright CERN; j.lopez@cern.ch
 */

#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RPageStorage.hxx>

#include <fstream>
#include <memory>
#include <unistd.h>

using RColumnDescriptor = ROOT::Experimental::RColumnDescriptor;
using RFieldDescriptor = ROOT::Experimental::RFieldDescriptor;
using RNTupleDescriptor = ROOT::Experimental::RNTupleDescriptor;
using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;
using RPageSource = ROOT::Experimental::Detail::RPageSource;
using RClusterIndex = ROOT::Experimental::RClusterIndex;

/// \brief Helper class to dump RNTuple pages / metadata to separate files.
///
class RNTupleDumper {
   std::unique_ptr<RPageSource> fSource;
   const RPageSource::RSharedDescriptorGuard fDesc;

   struct RColumnInfo {
      const RColumnDescriptor &fColumnDesc;
      const RFieldDescriptor &fFieldDesc;
   };

   void AddColumnsFromField(std::vector<RColumnInfo> &vec, const RFieldDescriptor &fieldDesc) {
   }
public:
   RNTupleDumper(std::unique_ptr<RPageSource> source)
     : fSource(std::move(source)), fDesc(source->GetSharedDescriptorGuard()) {}

   /// Recursively collect all the columns for all the fields rooted at field zero.
   std::vector<RColumnInfo> CollectColumns() {
      std::vector<RColumnInfo> columns;
      return columns;
   }

   /// Iterate over all the clusters and dump the contents of each page for each column.
   /// Generated file names follow the template `filenameTmpl` and are placed in directory `outputPath`.
   void DumpPages(const std::vector<RColumnInfo> &columns,
		  const std::string &outputPath, const std::string &filenameTmpl) {
   }

   /// Dump ntuple header and footer to separate files.
   void DumpMetadata(const std::string &outputPath) {
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
