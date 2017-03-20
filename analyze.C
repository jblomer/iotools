#include <iostream>

#include <TFile.h>
#include <TTree.h>

void analyze() {
  TFile file("iotrace.root");
  TTree *events = (TTree *) file.Get("Events");
  std::cout << "Number of Events: " << events->GetEntries() << std::endl;
}
