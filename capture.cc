/**
 * Copyright CERN; jblomer@cern.ch
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <string>

#include "wire_format.h"

using namespace std;

int main(int argc, char **argv) {
  string fanout{"iotrace.fanout"};

  unlink(fanout.c_str());
  int retval = mkfifo(fanout.c_str(), 0666);
  assert(retval == 0);
  int fd_fanout = open(fanout.c_str(), O_RDONLY);
  assert(fd_fanout >= 0);

  struct iotrace_frame frame;
  while (read(fd_fanout, &frame, sizeof(frame)) == sizeof(frame)) {
    switch (frame.op) {
      case IOO_OPEN:
        printf("file wass opened (took %ldns)\n", frame.duration_ns);
        break;
      default:
        printf("unknown operation\n");
    }
  }

  unlink(fanout.c_str());
  return 0;
}
