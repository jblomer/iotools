#ifndef _EVENT_H
#define _EVENT_H

#include <RtypesCore.h>

struct Base {
   int fBadBaseName;

   ClassDefNV(Base, 2)
};

struct Event : public Base {
   int fFoo;

   ClassDefNV(Event, 2)
};

#endif // _EVENT_H
