#ifndef _EVENT_H
#define _EVENT_H

#include <RtypesCore.h>

#include <string>

struct Track {
   std::string fProperties;

   ClassDefNV(Track, 2)
};

struct Event {
   Track fTrack;

   ClassDefNV(Event, 2)
};

#endif // _EVENT_H
