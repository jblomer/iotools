#ifndef _EVENT_H
#define _EVENT_H

#include <Rtypes.h>

struct Track {
   int fFoo;

   ClassDefNV(Track, 2)
};

struct PtrWrapper {
   Track *fPtr = nullptr;
   int fBar = 137;

   ClassDefNV(PtrWrapper, 2)
};

struct Event {
   Track *fTrack = nullptr;

   ClassDefNV(Event, 3)
};

#endif // _EVENT_H
