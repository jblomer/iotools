#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleOptions.hxx>
#include <ROOT/RNTupleView.hxx>
#include <ROOT/RVec.hxx>

#include <TCanvas.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTreePerfStats.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "util.h"

bool g_perf_stats = false;

static void TreeOptimized(const std::string &path) {
   auto ts_init = std::chrono::steady_clock::now();

   auto file = TFile::Open(path.c_str());
   auto tree = file->Get<TTree>("h42");

   TTreePerfStats *ps = nullptr;
   if (g_perf_stats)
      ps = new TTreePerfStats("ioperf", tree);

   auto nEntries = tree->GetEntries();
   std::chrono::steady_clock::time_point ts_first;
   for (decltype(nEntries) entryId = 0; entryId < nEntries; ++entryId) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;
      if (entryId == 1) {
         ts_first = std::chrono::steady_clock::now();
      }
   }

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   if (g_perf_stats)
      ps->Print();

   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
}


static void NTupleOptimized(const std::string &path) {
   using ENTupleInfo = ROOT::Experimental::ENTupleInfo;
   using RNTupleModel = ROOT::Experimental::RNTupleModel;
   using RNTupleReader = ROOT::Experimental::RNTupleReader;
   using RNTupleReadOptions = ROOT::Experimental::RNTupleReadOptions;

   auto ts_init = std::chrono::steady_clock::now();

   auto model = RNTupleModel::Create();
   RNTupleReadOptions options;
   options.SetClusterCache(RNTupleReadOptions::EClusterCache::kOn);
   auto ntuple = RNTupleReader::Open(std::move(model), "h42", path, options);
   if (g_perf_stats)
      ntuple->EnableMetrics();

   std::chrono::steady_clock::time_point ts_first;
   for (auto entryId : ntuple->GetViewRange()) {
      if (entryId % 1000 == 0)
         std::cout << "Processed " << entryId << " entries" << std::endl;
      if (entryId == 1) {
         ts_first = std::chrono::steady_clock::now();
      }
   }

   auto ts_end = std::chrono::steady_clock::now();
   auto runtime_init = std::chrono::duration_cast<std::chrono::microseconds>(ts_first - ts_init).count();
   auto runtime_analyze = std::chrono::duration_cast<std::chrono::microseconds>(ts_end - ts_first).count();

   if (g_perf_stats)
      ntuple->PrintInfo(ENTupleInfo::kMetrics);
   std::cout << "Runtime-Initialization: " << runtime_init << "us" << std::endl;
   std::cout << "Runtime-Analysis: " << runtime_analyze << "us" << std::endl;
}


static void Usage(const char *progname) {
  printf("%s [-i input.root/ntuple] [-p(erformance stats)]\n", progname);
}

int main(int argc, char **argv) {
   std::string path;
   int c;
   while ((c = getopt(argc, argv, "hvpi:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'p':
         g_perf_stats = true;
         break;
      case 'i':
         path = optarg;
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   if (path.empty()) {
      Usage(argv[0]);
      return 1;
   }

   auto suffix = GetSuffix(path);
   switch (GetFileFormat(suffix)) {
   case FileFormats::kRoot:
      TreeOptimized(path);
      break;
   case FileFormats::kNtuple:
      NTupleOptimized(path);
      break;
   default:
      std::cerr << "Invalid file format: " << suffix << std::endl;
      return 1;
   }

   return 0;
}
