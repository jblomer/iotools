#include "Event_v3.hxx"

#include <TFile.h>
#include <TSystem.h>
#include <TTree.h>

#include <iostream>

void readKey()
{
   gSystem->Load("./libEvent_v3.so");

   auto f = TFile::Open("dataKey.root");
   Event *event = f->Get<Event>("event");

   std::cout << event->fTrack << std::endl;
   if (event->fTrack)
      std::cout << event->fTrack->fFoo << std::endl;
}

int main()
{
   readKey();
   return 0;
}
