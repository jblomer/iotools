#ifndef _EVENT_H
#define _EVENT_H

#include <RtypesCore.h>

struct Base {
   ClassDef(Base, 3)
};

struct Event : public Base {
   int fBetterDerivedName;
   int fFoo;

   ClassDefOverride(Event, 3)
};

#endif // _EVENT_H
