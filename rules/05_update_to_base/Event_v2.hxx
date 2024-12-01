#ifndef _EVENT_H
#define _EVENT_H

#include <RtypesCore.h>

struct Base {
};

struct Event : public Base {
   int fFoo;
   int fBadDerivedName;

   ClassDefNV(Event, 2)
};

#endif // _EVENT_H
