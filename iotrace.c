/**
 * Copyright CERN; jblomer@cern.ch
 */

#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "wire_format.h"

// GNU extension
#ifndef RTLD_NEXT
#define RTLD_NEXT	((void *) -1l)
#endif

#ifndef O_TMPFILE
#define O_TMPFILE (020000000)
#endif

#define MAX_FILES_TRACED 64


struct iotrace_state_t {
  int (*ptr_open)(const char *pathname, int flags, ...);
  size_t (*ptr_read)(int fd, void *buf, size_t count);
  int (*ptr_close)(int fd);
  // TODO: seek, pread, mmap, fcntl
  int trace_fds[MAX_FILES_TRACED];
  int sz_trace_fds;
  pthread_mutex_t lock_trace_fds;
  char *pattern;
  int fd_fanout;
};
struct iotrace_state_t *iotrace_state = NULL;


static void iotrace_init_state() {
  if (iotrace_state != NULL)
    return;

  iotrace_state = malloc(sizeof(struct iotrace_state_t));
  memset(iotrace_state, 0, sizeof(struct iotrace_state_t));

  iotrace_state->ptr_open = dlsym(RTLD_NEXT, "open");
  if (iotrace_state->ptr_open == NULL) { abort(); }
  iotrace_state->ptr_read = dlsym(RTLD_NEXT, "read");
  if (iotrace_state->ptr_read == NULL) { abort(); }
  iotrace_state->ptr_close = dlsym(RTLD_NEXT, "close");
  if (iotrace_state->ptr_close == NULL) { abort(); }

  pthread_mutex_init(&iotrace_state->lock_trace_fds, NULL);
  iotrace_state->pattern = getenv("IOTRACE_FILENAME");

  char *fanout = getenv("IOTRACE_FANOUT");
  if (fanout == NULL) { fanout = "iotrace.fanout"; }
  iotrace_state->fd_fanout = iotrace_state->ptr_open(fanout, O_WRONLY);
  if (iotrace_state->fd_fanout < 0) {
    fprintf(stderr, "cannot open pipe %s\n", fanout);
    abort();
  }
}


static int iotrace_should_trace(const char *pathname) {
  if ((pathname == NULL) || (pathname[0] == '\0'))
    return 0;
  if (iotrace_state->pattern == NULL)
    return 1;

  int len = strlen(pathname);
  int i = len - 1;
  for (; i >= 0; --i) {
    if (pathname[i] == '/')
      break;
  }
  ++i;

  return strcmp(pathname + i, iotrace_state->pattern) == 0;
}


static int iotrace_get_idx_trace_fds(int fd) {
  for (int i = 0; i < iotrace_state->sz_trace_fds; ++i) {
    if (iotrace_state->trace_fds[i] == fd)
      return i;
  }
  return -1;
}

static void iotrace_lock() {
  pthread_mutex_lock(&iotrace_state->lock_trace_fds);
}

static void iotrace_unlock() {
  pthread_mutex_unlock(&iotrace_state->lock_trace_fds);
}

static void iotrace_send(struct iotrace_frame *frame) {
  ssize_t written = write(iotrace_state->fd_fanout, frame,
                          sizeof(struct iotrace_frame));
  assert(written == sizeof(struct iotrace_frame));
}


int open(const char *pathname, int flags, ...) {
  iotrace_init_state();
  int do_trace = iotrace_should_trace(pathname);

  mode_t mode = 0;
  if ((flags & O_CREAT) || (flags & O_TMPFILE)) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, mode_t);
    va_end(ap);
  }
  int result = iotrace_state->ptr_open(pathname, flags, mode);
  if (do_trace && (result >= 0)) {
    iotrace_lock();
    if (iotrace_state->sz_trace_fds >= MAX_FILES_TRACED) { abort(); }
    iotrace_state->trace_fds[iotrace_state->sz_trace_fds++] = result;
    iotrace_unlock();

    struct iotrace_frame frame;
    frame.op = IOO_OPEN;
    iotrace_send(&frame);
  }
  return result;
}


ssize_t read(int fd, void *buf, size_t count) {
  iotrace_init_state();

  return iotrace_state->ptr_read(fd, buf, count);
}


int close(int fd) {
  iotrace_init_state();

  int result = iotrace_state->ptr_close(fd);
  if (result != 0)
    return result;

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  if (idx_trace_fds >= 0) {
    iotrace_state->trace_fds[idx_trace_fds] =
      iotrace_state->trace_fds[iotrace_state->sz_trace_fds - 1];
    iotrace_state->sz_trace_fds--;
  }
  iotrace_unlock();

  return result;
}
