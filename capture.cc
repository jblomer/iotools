/**
 * Copyright CERN; jblomer@cern.ch
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <string>

#include "wire_format.h"

using namespace std;


int g_pipe_ctrl[2];


static void MakePipe(int pipe_fd[2]) {
  int retval = pipe(pipe_fd);
  assert(retval == 0);
}

/**
 * Writes to a pipe should always succeed.
 */
static void WritePipe(int fd, const void *buf, size_t nbyte) {
  int num_bytes;
  do {
    num_bytes = write(fd, buf, nbyte);
  } while ((num_bytes < 0) && (errno == EINTR));
  assert((num_bytes >= 0) && (static_cast<size_t>(num_bytes) == nbyte));
}


static void SignalExit(int signal) {
  char c = 'T';
  WritePipe(g_pipe_ctrl[1], &c, 1);
}

static void SignalKill(int signal) {
  exit(0);
}


int main(int argc, char **argv) {
  string fanout{"iotrace.fanout"};

  unlink(fanout.c_str());
  int retval = mkfifo(fanout.c_str(), 0666);
  assert(retval == 0);
  int fd_fanout = open(fanout.c_str(), O_RDONLY);
  assert(fd_fanout >= 0);

  MakePipe(g_pipe_ctrl);
  signal(SIGTERM, SignalExit);
  signal(SIGINT, SignalExit);
  signal(SIGQUIT, SignalKill);

  struct pollfd watch_fds[2];
  watch_fds[0].fd = g_pipe_ctrl[0];
  watch_fds[1].fd = fd_fanout;
  watch_fds[0].revents = watch_fds[1].revents = 0;
  watch_fds[0].events = watch_fds[1].events = POLLIN | POLLPRI;

  int64_t seqno = 0;
  struct iotrace_frame frame;

  while (true) {
    retval = poll(watch_fds, 2, -1);
    if (retval < 0)
      continue;

    if (watch_fds[0].revents)
      break;
    if (!watch_fds[1].revents)
      continue;

    watch_fds[1].revents = 0;
    ssize_t nbytes = read(fd_fanout, &frame, sizeof(frame));
    if (nbytes == 0) {
      printf("pipe closed, reopening\n");
      fd_fanout = open(fanout.c_str(), O_RDONLY);
      assert(fd_fanout >= 0);
      watch_fds[1].fd = fd_fanout;
      continue;
    }
    if (nbytes != sizeof(frame)) {
      fprintf(stderr, "invalid frame\n");
      continue;
    }
    switch (frame.op) {
      case IOO_OPEN:
        printf("file wass opened (took %ldns)\n", frame.duration_ns);
        break;
      case IOO_READ:
        printf("read %ld bytes in (took %ldns)\n",
               frame.info.read.size, frame.duration_ns);
        break;
      case IOO_SEEK:
        printf("seek %ld bytes (took %ldns)\n",
               frame.info.seek.offset, frame.duration_ns);
        break;
      default:
        printf("unknown operation\n");
    }
    seqno++;
  }

  printf("quitting...\n");
  unlink(fanout.c_str());
  return 0;
}
