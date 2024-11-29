#ifndef _EVENT_H
#define _EVENT_H

#include <RtypesCore.h>

#include <cmath>

struct Event {
   float fR;
   float fPhi;

   void SetFromEuclidian(float x, float y) {
      fR = sqrt(x*x + y*y);
      fPhi = atan2(y, x);
   }

   ClassDefNV(Event, 3)
};

#endif // _EVENT_H
