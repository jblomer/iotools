#ifndef _EVENT_H
#define _EVENT_H

#include <RtypesCore.h>

struct Track {
   float fPt;
   int fId; // identical to event id

   ClassDefNV(Track, 2)
};

struct Event {
   int fId;
   Track fTrack;

   ClassDefNV(Event, 2)
};

#endif // _EVENT_H
