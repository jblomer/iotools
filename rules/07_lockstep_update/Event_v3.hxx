#ifndef _EVENT_H
#define _EVENT_H

#include <RtypesCore.h>

struct Track {
   int fProperties; // changed from string to int

   ClassDefNV(Track, 3)
};

struct Event {
   Track fTrack;
   bool fHasTrackOfPropertyX;

   ClassDefNV(Event, 3)
};

#endif // _EVENT_H
