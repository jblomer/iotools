#ifndef _EVENT_H
#define _EVENT_H

#include <RtypesCore.h>

struct Base {
   int fBetterBaseName;

   ClassDefNV(Base, 3)
};

struct Event : public Base {
   int fFoo;
   ClassDefNV(Event, 3)
};

#endif // _EVENT_H
