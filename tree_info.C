#include <TFile.h>
#include <TTree.h>

#include <iostream>
#include <memory>
#include <string>

void tree_info(std::string fileName, std::string treeName)
{
   std::unique_ptr<TFile> f(TFile::Open(fileName.c_str()));
   assert(f && ! f->IsZombie());
   auto tree = f->Get<TTree>(treeName.c_str());
   tree->Print();
}

void Usage(char *progname) {
   std::cout << "Usage: " << progname << " file-name tree-name" << std::endl;
}

int main(int argc, char **argv) {
   if (argc < 3) {
      Usage(argv[0]);
      return 1;
   }
   tree_info(argv[1], argv[2]);
}
