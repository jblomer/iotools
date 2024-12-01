#ifndef _EVENT_H
#define _EVENT_H

#include <RtypesCore.h>

struct Track {
   float fPt;
   int fId; // identical to event id

   // QUESTION: should track get a new version number?
   ClassDefNV(Track, 3)
};

struct Event {
   int fNewId; // old ID + 1000
   Track fTrack;

   ClassDefNV(Event, 3)
};

#endif // _EVENT_H
