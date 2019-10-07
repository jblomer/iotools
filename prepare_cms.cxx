#include <TFile.h>
#include <TTree.h>

#include <iostream>
#include <string>

#include <unistd.h>

void Usage(char *progname) {
   std::cout << "Usage: " << progname << " -i <input file> -o <clustered output file>"
             << std::endl;
}


int main(int argc, char **argv) {
   std::string inputPath;
   std::string outputPath;

   int c;
   while ((c = getopt(argc, argv, "hvo:i:")) != -1) {
      switch (c) {
      case 'h':
      case 'v':
         Usage(argv[0]);
         return 0;
      case 'o':
         outputPath = optarg;
         break;
      case 'i':
         inputPath = optarg;
         break;
      default:
         fprintf(stderr, "Unknown option: -%c\n", c);
         Usage(argv[0]);
         return 1;
      }
   }
   if (inputPath.empty() || outputPath.empty()) {
      Usage(argv[0]);
      return 1;
   }
   std::cout << "Converting " << inputPath << " - [clustered] -> " << outputPath << std::endl;

   auto inputFile = TFile::Open(inputPath.c_str());
   auto inputTree = inputFile->Get<TTree>("Events");
   inputTree->SetAutoFlush();
   auto outputFile = new TFile(outputPath.c_str(), "RECREATE");
   inputTree->CloneTree();
   outputFile->Write();
   outputFile->Close();
   delete outputFile;
}
