#include "Event_v3.hxx"

#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>

#include <iostream>

void read()
{
   gSystem->Load("./libEvent_v3.so");

   auto f = TFile::Open("data.root");
   auto t = f->Get<TTree>("t");

   Event *event = nullptr;
   t->SetBranchAddress("event", &event);

   for (int i = 0; i < t->GetEntries(); ++i) {
      t->GetEntry(i);

      std::cout << event->fTrack << std::endl;
      if (event->fTrack)
         std::cout << event->fTrack->fFoo << std::endl;
   }
}

int main()
{
   read();
   return 0;
}
