#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleReader.hxx>
#include <ROOT/RNTupleReadOptions.hxx>
#include <ROOT/RNTupleWriter.hxx>
#include <ROOT/RNTupleWriteOptions.hxx>

#include <sys/resource.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <fstream>

constexpr char const *kFileName = "huge.root";

static double GetRSS()
{
   std::size_t resident_pages = 0;
   if (std::ifstream f("/proc/self/statm"); f) {
      std::size_t total;
      f >> total >> resident_pages;
   }
   const double bytes = resident_pages * static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
   return bytes / (1024. * 1024.);
}

static double GetPeakRSS()
{
   struct rusage ru;
   getrusage(RUSAGE_SELF, &ru);
   return static_cast<double>(ru.ru_maxrss) / 1024.;
}

static void Usage(const char *progname) {
   printf("%s [-s(howInfo)] [-w(rite)] [-r(ead) <eager|ondemand>]\n", progname);
}

int main(int argc, char **argv)
{
   bool showInfo = false;
   int writeClusterGroups = 0;
   bool read = false;
   ROOT::RNTupleReadOptions::EMetadataMode mode = ROOT::RNTupleReadOptions::EMetadataMode::kEager;
   int c;
   while ((c = getopt(argc, argv, "hvsw:r:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 's':
         showInfo = true;
         break;
      case 'w':
         writeClusterGroups = atoi(optarg);
         break;
      case 'r':
         if (std::string(optarg) == "eager") {
            mode = ROOT::RNTupleReadOptions::EMetadataMode::kEager;
         } else if (std::string(optarg) == "ondemand") {
            mode = ROOT::RNTupleReadOptions::EMetadataMode::kOnDemand;
         } else {
            fprintf(stderr, "Unknown metadata mode: %s\n", optarg);
            Usage(argv[0]);
            return 1;
         }
         read = true;
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }

   if (writeClusterGroups > 0) {
      printf("   ... writing file with %d cluster groups\n", writeClusterGroups);
      auto model = ROOT::RNTupleModel::Create();
      auto f = model->MakeField<double>("f");
      ROOT::RNTupleWriteOptions wopts;
      wopts.SetMaxUnzippedPageSize(16);
      auto writer = ROOT::RNTupleWriter::Recreate(std::move(model), "ntpl", kFileName, wopts);

      for (int c = 0; c < 10 * writeClusterGroups; ++c) {
         for (int i = 0; i < 200000; ++i) {
            writer->Fill();
         }
         const auto newGroup = (c > 0) && ((c % 10) == 0);
         writer->CommitCluster(newGroup);
      }
      writer.reset();
      printf("   ... done\n");
   }

   if (read) {
      switch (mode) {
      case ROOT::RNTupleReadOptions::EMetadataMode::kEager:
         printf("   ... using eager mode\n");
         break;
      case ROOT::RNTupleReadOptions::EMetadataMode::kOnDemand:
         printf("   ... using on-demand mode\n");
         break;
      default:
         printf("   ... UNKNOWN MODE\n");
         return 1;
      }

      ROOT::RNTupleReadOptions ropts;
      ropts.SetMetadataMode(mode);
      auto reader = ROOT::RNTupleReader::Open("ntpl", kFileName, ropts);
      printf("Number of clusters: %lu\n", reader->GetDescriptor().GetNClusters());
      printf("Number of cluster groups: %lu\n", reader->GetDescriptor().GetNClusterGroups());

      std::cout << "ON-OPEN RSS: " << GetRSS() << " MiB\n";
      std::cout << "ON-OPEN Peak RSS was " << GetPeakRSS() << " MiB\n";

      for (auto idx : *reader) {
         reader->LoadEntry(idx);
      }

      printf("Number of active clusters: %lu\n", reader->GetDescriptor().GetNActiveClusters());
   }

   std::cout << "RSS: " << GetRSS() << " MiB\n";
   std::cout << "Peak RSS was " << GetPeakRSS() << " MiB\n";

   if (showInfo) {
      ROOT::RNTupleReader::Open("ntpl", kFileName)->PrintInfo(ROOT::ENTupleInfo::kStorageDetails);
   }

   return 0;
}
