#ifndef _EVENT_H
#define _EVENT_H

#include <Rtypes.h>

struct Track {
   int fFoo;

   ClassDefNV(Track, 2)
};

struct PtrWrapper {
   Track *fPtr = nullptr;

   ClassDefNV(PtrWrapper, 2)
};

struct Event {
   PtrWrapper fTrack;

   ClassDefNV(Event, 2)
};

#endif // _EVENT_H
