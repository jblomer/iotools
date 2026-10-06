#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RPageNullSink.hxx>
#include <ROOT/RPageStorage.hxx>
#include <ROOT/RPageStorageFile.hxx>
#include <ROOT/RNTupleWriteOptions.hxx>
#include <ROOT/RNTupleWriter.hxx>

#include <TFile.h>
#include <TROOT.h>

#include <sys/resource.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

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
   printf("%s -o <number of output files> -f <number of fields per file> [-c(ompression) <setting>] "
          "[-d(ry run / null sink)] [-a(ppend)] [-e(entries) per file] [-m(max page size) <bytes>] "
          "[-i(nitial page size) <bytes>] [-u(nbuffered)] [-w(rite buffer size) <bytes>] "
          "[-t(hreading)] [-n(ull data))]\n", progname);
}

int main(int argc, char **argv)
{
   int nFiles = -1;
   int nFieldsPerFile = -1;
   int nEntriesPerFile = 0;
   int compression = 0;
   int initialPageSize = 0;
   int maxPageSize = 0;
   int writeBufferSize = 0;
   bool unbuffered = false;
   bool dryRun = false;
   bool append = false;
   bool imt = false;
   bool nullData = false;
   int c;
   while ((c = getopt(argc, argv, "hvo:f:c:e:m:i:w:tnuda")) != -1) {
      switch (c) {
      case 'h':
        Usage(argv[0]);
        return 0;
      case 'o':
         nFiles = atoi(optarg);
         break;
      case 'f':
         nFieldsPerFile = atoi(optarg);
         break;
      case 'c':
         compression = atoi(optarg);
         break;
      case 'e':
         nEntriesPerFile = atoi(optarg);
         break;
      case 'i':
         initialPageSize = atoi(optarg);
         break;
      case 'm':
         maxPageSize = atoi(optarg);
         break;
      case 'w':
         writeBufferSize = atoi(optarg);
         break;
      case 't':
         imt = true;
         break;
      case 'n':
         nullData = true;
         break;
      case 'u':
         unbuffered = true;
         break;
      case 'd':
         dryRun = true;
         break;
      case 'a':
         append = true;
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   if (nFiles <= 0) {
      fprintf(stderr, "Invalid number of output files: %d\n", nFiles);
      Usage(argv[0]);
      return 1;
   }
   if (nFieldsPerFile <= 0) {
      fprintf(stderr, "Invalid number of fields per file: %d\n", nFieldsPerFile);
      Usage(argv[0]);
      return 1;
   }

   printf("Simulating data derivation with %d output files, having %d fields each\n", nFiles, nFieldsPerFile);
   printf("   --> total number of columns: %d\n", nFiles * nFieldsPerFile);
   printf("Writing %d entries per file\n", nEntriesPerFile);
   printf("   --> total number of rows: %d\n", nFiles * nEntriesPerFile);
   printf("Using compression settings %d\n", compression);
   if (dryRun) {
      printf("   ... using the Null sink\n");
   }

   if (imt) {
      printf("Enabling imlicit multi-threading\n");
      ROOT::EnableImplicitMT();
   }
   if (nullData) {
      printf("Writing only zero bytes, no RNG\n");
   }

   auto ts_begin = std::chrono::steady_clock::now();

   std::vector<std::shared_ptr<double>> values;
   values.reserve(nFiles * nFieldsPerFile);

   std::vector<std::unique_ptr<TFile>> files;
   std::vector<std::unique_ptr<ROOT::RNTupleWriter>> writers;
   files.reserve(nFiles);
   writers.reserve(nFiles);

   ROOT::RNTupleWriteOptions opts;
   opts.SetCompression(compression);
   if (initialPageSize) {
      printf("Setting initial page size to %d B\n", initialPageSize);
      opts.SetInitialUnzippedPageSize(initialPageSize);
   }
   if (maxPageSize) {
      printf("Setting max page size to %d B\n", maxPageSize);
      opts.SetMaxUnzippedPageSize(maxPageSize);
   }
   if (writeBufferSize) {
      printf("Setting write buffer size to %d B\n", writeBufferSize);
      opts.SetWriteBufferSize(writeBufferSize);
   }
   if (unbuffered) {
      printf("Using unbuffered writes\n");
      opts.SetUseBufferedWrite(false);
   }

   printf("\n");

   for (int i = 0; i < nFiles; ++i) {
      auto model = ROOT::RNTupleModel::Create();
      for (int j = 0; j < nFieldsPerFile; ++j) {
         const std::string fieldName = "FEATURE_" + std::to_string(j);
         values.emplace_back(model->MakeField<double>(fieldName));
      }

      std::unique_ptr<ROOT::Internal::RPageSink> sink;
      if (dryRun) {
         sink = std::make_unique<ROOT::Experimental::Internal::RPageNullSink>("Events", opts);
      } else {
         const std::string fileName = "NTPL_" + std::to_string(i) + ".root";
         if (append) {
            files.emplace_back(TFile::Open(fileName.c_str(), "RECREATE"));
            sink = std::make_unique<ROOT::Internal::RPageSinkFile>("Events", *files.back(), opts);
         } else {
            sink = std::make_unique<ROOT::Internal::RPageSinkFile>("Events", fileName, opts);
         }
      }
      writers.emplace_back(ROOT::Internal::CreateRNTupleWriter(std::move(model), std::move(sink)));
   }

   auto ts_schema = std::chrono::steady_clock::now();
   auto duration_schema = std::chrono::duration_cast<std::chrono::milliseconds>(ts_schema - ts_begin).count();
   std::cout << "Creating the schema took " << duration_schema << " milliseconds\n";
   std::cout << "RSS: " << GetRSS() << " MiB\n";

   std::random_device rd;
   std::mt19937 gen(rd());
   std::normal_distribution<double> gauss(2., 3.);

   for (int i = 0; i < nEntriesPerFile; ++i) {
      for (int j = 0; j < nFiles; ++j) {
         if (!nullData) {
            for (int k = 0; k < nFieldsPerFile; ++k) {
               *values[j * nFieldsPerFile + k] = gauss(gen);
            }
         }
         writers[j]->Fill();
      }
   }

   auto ts_written = std::chrono::steady_clock::now();
   auto duration_written = std::chrono::duration_cast<std::chrono::milliseconds>(ts_written - ts_schema).count();
   std::cout << "Writing values took " << duration_written << " milliseconds\n";
   std::cout << "RSS: " << GetRSS() << " MiB\n";

   for (int i = 0; i < nFiles; ++i) {
      writers[i]->CommitDataset();
   }

   auto ts_committed = std::chrono::steady_clock::now();
   auto duration_committed = std::chrono::duration_cast<std::chrono::milliseconds>(ts_committed - ts_written).count();
   std::cout << "Committing the datasets took " << duration_committed << " milliseconds\n";
   std::cout << "RSS: " << GetRSS() << " MiB\n";

   writers.clear();
   files.clear();
   values.clear();

   auto ts_cleanup = std::chrono::steady_clock::now();
   auto duration_cleanup = std::chrono::duration_cast<std::chrono::milliseconds>(ts_cleanup - ts_schema).count();
   std::cout << "Destructing I/O took " << duration_cleanup << " milliseconds\n";

   printf("\n");

   std::cout << "Peak RSS was " << GetPeakRSS() << " MiB\n";
   std::cout << "Total payload is " <<
                static_cast<double>(sizeof(double) * nFiles * nFieldsPerFile * nEntriesPerFile) / (1000. * 1000.)
                << " MB\n";

   if (dryRun)
      return 0;

   std::uintmax_t totalSize = 0;
   for (int i = 0; i < nFiles; ++i) {
      totalSize += std::filesystem::file_size("NTPL_" + std::to_string(i) + ".root");
   }
   std::cout << "Total file size of " << nFiles << " files is " << static_cast<double>(totalSize) / (1000. * 1000.) <<
                " MB\n";

   return 0;
}
