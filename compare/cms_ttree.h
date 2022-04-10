#ifndef CMS_TTREE_H_
#define CMS_TTREE_H_

#include "cms_event.h"

#include <string>

class TChain;
class TBranch;

class EventReaderRoot {
 public:
  EventReaderRoot() = default;
  void Open(const std::string &path);
  bool NextEvent();

  CmsEvent fEvent;
  int fNEvents = -1;
  int fPos = -1;

 private:
  TChain *fChain = nullptr;
  TBranch *fBranch[84]{};
};

#endif // CMS_TTREE_H_
