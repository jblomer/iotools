/**
 * Copyright CERN; jblomer@cern.ch
 */

#define _LARGEFILE64_SOURCE
#if __STDC_VERSION__ >= 199901L
// POSIX 2008
#define _XOPEN_SOURCE 700
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <time.h>
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
#define IOTRACE_CLOCK_ID CLOCK_MONOTONIC


struct iotrace_state_t {
  int (*ptr_open)(const char *pathname, int flags, ...);
  int (*ptr_open64)(const char *pathname, int flags, ...);
  ssize_t (*ptr_read)(int fd, void *buf, size_t count);
  ssize_t (*ptr_readv)(int fd, const struct iovec *iov, int iovcnt);
  int (*ptr_close)(int fd);
  off_t (*ptr_lseek)(int fd, off_t offset, int whence);
  off64_t (*ptr_lseek64)(int fd, off64_t offset, int whence);
  ssize_t (*ptr_pread)(int fd, void *buf, size_t nbyte, off_t offset);
  ssize_t (*ptr_pread64)(int fd, void *buf, size_t nbyte, off64_t offset);
  // TODO: mmap, mmap64, openat, openat64, fcntl, fcntl64, dup, dup2, preadv, preadv64, preadv2, aio_read, aio_read64
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
  iotrace_state->ptr_open64 = dlsym(RTLD_NEXT, "open64");
  if (iotrace_state->ptr_open64 == NULL) { abort(); }
  iotrace_state->ptr_read = dlsym(RTLD_NEXT, "read");
  if (iotrace_state->ptr_read == NULL) { abort(); }
  iotrace_state->ptr_readv = dlsym(RTLD_NEXT, "readv");
  if (iotrace_state->ptr_readv == NULL) { abort(); }
  iotrace_state->ptr_close = dlsym(RTLD_NEXT, "close");
  if (iotrace_state->ptr_close == NULL) { abort(); }
  iotrace_state->ptr_lseek = dlsym(RTLD_NEXT, "lseek");
  if (iotrace_state->ptr_lseek == NULL) { abort(); }
  iotrace_state->ptr_lseek64 = dlsym(RTLD_NEXT, "lseek64");
  if (iotrace_state->ptr_lseek64 == NULL) { abort(); }
  iotrace_state->ptr_pread = dlsym(RTLD_NEXT, "pread");
  if (iotrace_state->ptr_pread == NULL) { abort(); }
  iotrace_state->ptr_pread64 = dlsym(RTLD_NEXT, "pread64");
  if (iotrace_state->ptr_pread64 == NULL) { abort(); }

  pthread_mutex_init(&iotrace_state->lock_trace_fds, NULL);
  iotrace_state->pattern = getenv("IOTRACE_FILENAME");
  if (iotrace_state->pattern != NULL)
    printf("*** IOTRACE: tracing %s\n", iotrace_state->pattern);

  char *fanout = getenv("IOTRACE_FANOUT");
  if (fanout == NULL) { fanout = "iotrace.fanout"; }
  iotrace_state->fd_fanout = iotrace_state->ptr_open(fanout, O_WRONLY);
  if (iotrace_state->fd_fanout < 0) {
    fprintf(stderr, "cannot open pipe %s\n", fanout);
    abort();
  }

  struct timespec tp_res;
  int retval = clock_getres(IOTRACE_CLOCK_ID, &tp_res);
  assert(retval == 0);
  printf("*** IOTRACE: clock resolution %lds, %ldns\n",
         tp_res.tv_sec, tp_res.tv_nsec);
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

static inline struct timespec iotrace_stopwatch() {
  struct timespec ts_start;
  int retval = clock_gettime(IOTRACE_CLOCK_ID, &ts_start);
  assert(retval == 0);
  return ts_start;
}

static inline int64_t iotrace_meter_ns(struct timespec *ts_start) {
  struct timespec ts_end;
  int retval = clock_gettime(IOTRACE_CLOCK_ID, &ts_end);
  assert(retval == 0);
  return ((ts_end.tv_sec - ts_start->tv_sec) * 1000000000 +
          (ts_end.tv_nsec - ts_start->tv_nsec));
}


int open(const char *pathname, int flags, ...) {
  iotrace_init_state();

  mode_t mode = 0;
  if ((flags & O_CREAT) || (flags & O_TMPFILE)) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, mode_t);
    va_end(ap);
  }
  int do_trace = iotrace_should_trace(pathname);
  printf("open %s, do_trace is %d\n", pathname, do_trace);
  if (!do_trace)
    return iotrace_state->ptr_open(pathname, flags, mode);

  struct timespec ts_start = iotrace_stopwatch();
  int result = iotrace_state->ptr_open(pathname, flags, mode);
  int64_t duration_ns = iotrace_meter_ns(&ts_start);
  if (result >= 0) {
    iotrace_lock();
    if (iotrace_state->sz_trace_fds >= MAX_FILES_TRACED) { abort(); }
    iotrace_state->trace_fds[iotrace_state->sz_trace_fds++] = result;
    iotrace_unlock();

    struct iotrace_frame frame;
    frame.op = IOO_OPEN;
    frame.fd = result;
    frame.duration_ns = duration_ns;
    iotrace_send(&frame);
  }
  return result;
}


int open64(const char *pathname, int flags, ...) {
  iotrace_init_state();

  mode_t mode = 0;
  if ((flags & O_CREAT) || (flags & O_TMPFILE)) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, mode_t);
    va_end(ap);
  }
  int do_trace = iotrace_should_trace(pathname);
  printf("open %s, do_trace is %d\n", pathname, do_trace);
  if (!do_trace)
    return iotrace_state->ptr_open64(pathname, flags, mode);

  struct timespec ts_start = iotrace_stopwatch();
  int result = iotrace_state->ptr_open64(pathname, flags, mode);
  int64_t duration_ns = iotrace_meter_ns(&ts_start);
  if (result >= 0) {
    iotrace_lock();
    if (iotrace_state->sz_trace_fds >= MAX_FILES_TRACED) { abort(); }
    iotrace_state->trace_fds[iotrace_state->sz_trace_fds++] = result;
    iotrace_unlock();

    struct iotrace_frame frame;
    frame.op = IOO_OPEN;
    frame.fd = result;
    frame.duration_ns = duration_ns;
    iotrace_send(&frame);
  }
  return result;
}


ssize_t read(int fd, void *buf, size_t count) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_read(fd, buf, count);

  struct timespec ts_start = iotrace_stopwatch();
  ssize_t result = iotrace_state->ptr_read(fd, buf, count);
  int64_t duration_ns = iotrace_meter_ns(&ts_start);
  struct iotrace_frame frame;
  frame.op = IOO_READ;
  frame.fd = fd;
  frame.duration_ns = duration_ns;
  if (result >= 0)
    frame.info.read.size = result;
  else
    frame.info.read.size = 0;
  iotrace_send(&frame);
  return result;
}


ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_readv(fd, iov, iovcnt);

  struct timespec ts_start = iotrace_stopwatch();
  ssize_t result = iotrace_state->ptr_readv(fd, iov, iovcnt);
  int64_t duration_ns = iotrace_meter_ns(&ts_start);
  struct iotrace_frame frame;
  frame.op = IOO_READ;
  frame.fd = fd;
  frame.duration_ns = duration_ns;
  if (result >= 0)
    frame.info.read.size = result;
  else
    frame.info.read.size = 0;
  iotrace_send(&frame);
  return result;
}


off_t lseek(int fd, off_t offset, int whence) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_lseek(fd, offset, whence);

  off_t pos_cur = iotrace_state->ptr_lseek(fd, 0, SEEK_CUR);
  struct timespec ts_start = iotrace_stopwatch();
  off_t result = iotrace_state->ptr_lseek(fd, offset, whence);
  int64_t duration_ns = iotrace_meter_ns(&ts_start);
  struct iotrace_frame frame;
  frame.op = IOO_SEEK;
  frame.fd = fd;
  frame.duration_ns = duration_ns;
  if ((result >= 0) && (pos_cur >= 0))
    frame.info.seek.offset = result - pos_cur;
  else
    frame.info.seek.offset = -1;
  iotrace_send(&frame);
  return result;
}


off64_t lseek64(int fd, off64_t offset, int whence) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_lseek64(fd, offset, whence);

  off64_t pos_cur = iotrace_state->ptr_lseek64(fd, 0, SEEK_CUR);
  struct timespec ts_start = iotrace_stopwatch();
  off64_t result = iotrace_state->ptr_lseek64(fd, offset, whence);
  int64_t duration_ns = iotrace_meter_ns(&ts_start);
  struct iotrace_frame frame;
  frame.op = IOO_SEEK;
  frame.fd = fd;
  frame.duration_ns = duration_ns;
  if ((result >= 0) && (pos_cur >= 0))
    frame.info.seek.offset = result - pos_cur;
  else
    frame.info.seek.offset = -1;
  iotrace_send(&frame);
  return result;
}


ssize_t pread(int fd, void *buf, size_t nbyte, off_t offset) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_pread(fd, buf, nbyte, offset);

  off64_t pos_cur = iotrace_state->ptr_lseek(fd, 0, SEEK_CUR);
  struct timespec ts_start = iotrace_stopwatch();
  ssize_t result = iotrace_state->ptr_pread(fd, buf, nbyte, offset);
  int64_t duration_ns = iotrace_meter_ns(&ts_start);
  struct iotrace_frame frame;
  frame.op = IOO_SEEK;
  frame.fd = fd;
  frame.duration_ns = 0;
  if (pos_cur >= 0)
    frame.info.seek.offset = offset - pos_cur;
  else
    frame.info.seek.offset = -1;
  iotrace_send(&frame);
  frame.op = IOO_READ;
  frame.duration_ns = duration_ns;
  if (result >= 0)
    frame.info.read.size = result;
  else
    frame.info.read.size = 0;
  iotrace_send(&frame);
  return result;
}


ssize_t pread64(int fd, void *buf, size_t nbyte, off64_t offset) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_pread64(fd, buf, nbyte, offset);

  off64_t pos_cur = iotrace_state->ptr_lseek64(fd, 0, SEEK_CUR);
  struct timespec ts_start = iotrace_stopwatch();
  ssize_t result = iotrace_state->ptr_pread64(fd, buf, nbyte, offset);
  int64_t duration_ns = iotrace_meter_ns(&ts_start);
  struct iotrace_frame frame;
  frame.op = IOO_SEEK;
  frame.fd = fd;
  frame.duration_ns = 0;
  if (pos_cur >= 0)
    frame.info.seek.offset = offset - pos_cur;
  else
    frame.info.seek.offset = -1;
  iotrace_send(&frame);
  frame.op = IOO_READ;
  frame.duration_ns = duration_ns;
  if (result >= 0)
    frame.info.read.size = result;
  else
    frame.info.read.size = 0;
  iotrace_send(&frame);
  return result;
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
