#include "Event_v2.hxx"

void writeKey()
{
   gSystem->Load("./libEvent_v2.so");

   auto f = TFile::Open("dataKey.root", "RECREATE");

   Event event;
   event.fTrack.fPtr = new Track{7};

   f->WriteObject(&event, "event");
   f->Close();

   f = TFile::Open("dataKey.root");
   Event *evPtr = f->Get<Event>("event");

   std::cout << evPtr->fTrack.fPtr << std::endl;
   if (evPtr->fTrack.fPtr)
      std::cout << evPtr->fTrack.fPtr->fFoo << std::endl;
}
