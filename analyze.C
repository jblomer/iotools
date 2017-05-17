#include <iostream>

#include <TFile.h>
#include <TTree.h>

void analyze(TString input = "iotrace.root") {
  TFile file(input);
  TTree *events = (TTree *) file.Get("Events");

  std::cout << "Number of Events: " << events->GetEntries() << std::endl;


}
