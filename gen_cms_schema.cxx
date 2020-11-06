#include <TFile.h>
#include <TTree.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include <unistd.h>

#include "util.h"


static void Usage(char *progname)
{
   std::cout << "Usage: " << progname << " -i <ttjet_13tev_june2019.root> -o <path for minimal ttjet file>"
             << std::endl;
}

int main(int argc, char **argv)
{
   std::string inputFile = "ttjet_13tev_june2019.root";
   std::string outputPath = ".";

   int c;
   while ((c = getopt(argc, argv, "hvi:o:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'i':
         inputFile = optarg;
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

   std::string schameName = "ttjet_13tev_june2019_schema.root";
   std::string outputFile = outputPath + "/" + schameName;
   std::cout << "Converting " << inputFile << " --> " << outputFile << std::endl;

   std::unique_ptr<TFile> f(TFile::Open(inputFile.c_str()));
   assert(f && ! f->IsZombie());

   auto tree = f->Get<TTree>("Events");

   std::unique_ptr<TFile> fout(TFile::Open(outputFile.c_str(), "RECREATE"));
   auto treeOut = tree->CopyTree("event == 0");
   treeOut->Write();
   fout->Close();
}
