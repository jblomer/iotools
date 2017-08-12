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

#include <aio.h>
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
  int (*ptr_openat)(int fd, const char *pathname, int flags, ...);
  int (*ptr_openat64)(int fd, const char *pathname, int flags, ...);
  int (*ptr_fcntl)(int fd, int cmd, ...);
  int (*ptr_fcntl64)(int fd, int cmd, ...);
  ssize_t (*ptr_read)(int fd, void *buf, size_t count);
  ssize_t (*ptr_readv)(int fd, const struct iovec *iov, int iovcnt);
  int (*ptr_close)(int fd);
  off_t (*ptr_lseek)(int fd, off_t offset, int whence);
  off64_t (*ptr_lseek64)(int fd, off64_t offset, int whence);
  ssize_t (*ptr_pread)(int fd, void *buf, size_t nbyte, off_t offset);
  ssize_t (*ptr_pread64)(int fd, void *buf, size_t nbyte, off64_t offset);
  ssize_t (*ptr_preadv)(int fd, const struct iovec *iov, int iovcnt, off_t off);
  ssize_t (*ptr_preadv64)(int fd, const struct iovec *iov, int c, off64_t off);
  ssize_t (*ptr_preadv2)(int fd, const struct iovec *iov, int c, off_t off,
                         int flags);
  void *(*ptr_mmap)(void *a, size_t l, int prot, int flags, int fd, off_t off);
  void *(*ptr_mmap64)(void *a, size_t l, int p, int flags, int fd, off64_t off);
  int (*ptr_dup)(int fd);
  int (*ptr_dup2)(int fd, int fd2);
  int (*ptr_aio_read)(struct aiocb *aiocbp);

  // TODO:  aio_read, aio_read64
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
  iotrace_state->ptr_openat = dlsym(RTLD_NEXT, "openat");
  if (iotrace_state->ptr_openat == NULL) { abort(); }
  iotrace_state->ptr_openat64 = dlsym(RTLD_NEXT, "openat64");
  if (iotrace_state->ptr_openat64 == NULL) { abort(); }
  iotrace_state->ptr_fcntl = dlsym(RTLD_NEXT, "fcntl");
  if (iotrace_state->ptr_fcntl == NULL) { abort(); }
  //iotrace_state->ptr_fcntl64 = dlsym(RTLD_NEXT, "fcntl64");
  //if (iotrace_state->ptr_fcntl64 == NULL) { abort(); }
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
  iotrace_state->ptr_preadv = dlsym(RTLD_NEXT, "preadv");
  if (iotrace_state->ptr_preadv == NULL) { abort(); }
  iotrace_state->ptr_preadv64 = dlsym(RTLD_NEXT, "preadv64");
  if (iotrace_state->ptr_preadv64 == NULL) { abort(); }
  //iotrace_state->ptr_preadv2 = dlsym(RTLD_NEXT, "preadv2");
  //if (iotrace_state->ptr_preadv2 == NULL) { abort(); }
  iotrace_state->ptr_mmap = dlsym(RTLD_NEXT, "mmap");
  if (iotrace_state->ptr_mmap == NULL) { abort(); }
  iotrace_state->ptr_mmap64 = dlsym(RTLD_NEXT, "mmap64");
  if (iotrace_state->ptr_mmap64 == NULL) { abort(); }
  iotrace_state->ptr_dup = dlsym(RTLD_NEXT, "dup");
  if (iotrace_state->ptr_dup == NULL) { abort(); }
  iotrace_state->ptr_dup2 = dlsym(RTLD_NEXT, "dup2");
  if (iotrace_state->ptr_dup2 == NULL) { abort(); }
  iotrace_state->ptr_aio_read = dlsym(RTLD_NEXT, "aio_read");
  if (iotrace_state->ptr_aio_read == NULL) { abort(); }

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


int fcntl(int fd, int cmd, ...) {
  iotrace_init_state();

  int arg_int = 0;
  int use_arg_int = 0;
  void *arg_ptr = NULL;
  int use_arg_ptr = 0;
  if ((cmd == F_DUPFD) || (cmd == F_SETFD) || (cmd == F_SETFL) ||
      (cmd == F_SETOWN))
  {
    va_list ap;
    va_start(ap, cmd);
    arg_int = va_arg(ap, int);
    va_end(ap);
    use_arg_int = 1;
  }
  if ((cmd == F_GETLK) || (cmd == F_SETLK)) {
    va_list ap;
    va_start(ap, cmd);
    arg_ptr = va_arg(ap, void *);
    va_end(ap);
    use_arg_ptr = 1;
  }

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if ((idx_trace_fds < 0) || (cmd == F_GETLK) || (cmd == F_SETLK)) {
    if (use_arg_int)
      return iotrace_state->ptr_fcntl(fd, cmd, arg_int);
    if (use_arg_ptr)
      return iotrace_state->ptr_fcntl(fd, cmd, arg_ptr);
    return iotrace_state->ptr_fcntl(fd, cmd);
  }

  printf("strange fcntl\n");
  abort();
}

/*int fcntl64(int fd, int cmd, ...) {
  iotrace_init_state();

  int arg_int = 0;
  int use_arg_int = 0;
  void *arg_ptr = NULL;
  int use_arg_ptr = 0;
  if ((cmd == F_DUPFD) || (cmd == F_SETFD) || (cmd == F_SETFL) ||
      (cmd == F_SETOWN))
  {
    va_list ap;
    va_start(ap, cmd);
    arg_int = va_arg(ap, int);
    va_end(ap);
    use_arg_int = 1;
  }
  if ((cmd == F_GETLK) || (cmd == F_SETLK)) {
    va_list ap;
    va_start(ap, cmd);
    arg_ptr = va_arg(ap, void *);
    va_end(ap);
    use_arg_ptr = 1;
  }

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0) {
    if (use_arg_int)
      return iotrace_state->ptr_fcntl64(fd, cmd, arg_int);
    if (use_arg_ptr)
      return iotrace_state->ptr_fcntl64(fd, cmd, arg_ptr);
    return iotrace_state->ptr_fcntl64(fd, cmd);
  }

  abort();
}*/

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
  if (!do_trace)
    return iotrace_state->ptr_open(pathname, flags, mode);
  printf("*** IOTRACE: following %s\n", pathname);

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
  if (!do_trace)
    return iotrace_state->ptr_open64(pathname, flags, mode);
  printf("*** IOTRACE: following %s\n", pathname);

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


int openat(int fd, const char *pathname, int flags, ...) {
  iotrace_init_state();

  mode_t mode = 0;
  if ((flags & O_CREAT) || (flags & O_TMPFILE)) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, mode_t);
    va_end(ap);
  }
  int do_trace = iotrace_should_trace(pathname);
  if (!do_trace)
    return iotrace_state->ptr_openat(fd, pathname, flags, mode);
  printf("*** IOTRACE: following %s\n", pathname);

  struct timespec ts_start = iotrace_stopwatch();
  int result = iotrace_state->ptr_openat(fd, pathname, flags, mode);
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


int openat64(int fd, const char *pathname, int flags, ...) {
  iotrace_init_state();

  mode_t mode = 0;
  if ((flags & O_CREAT) || (flags & O_TMPFILE)) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, mode_t);
    va_end(ap);
  }
  int do_trace = iotrace_should_trace(pathname);
  if (!do_trace)
    return iotrace_state->ptr_openat64(fd, pathname, flags, mode);
  printf("*** IOTRACE: following %s\n", pathname);

  struct timespec ts_start = iotrace_stopwatch();
  int result = iotrace_state->ptr_openat64(fd, pathname, flags, mode);
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

int aio_read(struct aiocb *aiocbp) {
  iotrace_init_state();

  int fd = aiocbp->aio_fildes;
  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_aio_read(aiocbp);

  printf("aio_read unsupported\n");
  abort();
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


ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t off) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_preadv(fd, iov, iovcnt, off);

  printf("preadv unsupported\n");
  abort();
}


ssize_t preadv64(int fd, const struct iovec *iov, int iovcnt, off64_t off) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_preadv64(fd, iov, iovcnt, off);

  printf("preadv64 unsupported\n");
  abort();
}


/*ssize_t preadv2(int fd, const struct iovec *iov, int c, off_t off, int flags) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_preadv2(fd, iov, c, off, flags);

  abort();
}*/


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

  //printf("*** IOTRACE: pread\n");
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

  //printf("*** IOTRACE: pread64\n");
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

void *mmap(void *a, size_t l, int prot, int flags, int fd, off_t off) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_mmap(a, l, prot, flags, fd, off);

  printf("mmap unsupported\n");
  abort();
}

void *mmap64(void *a, size_t l, int prot, int flags, int fd, off64_t off) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_mmap64(a, l, prot, flags, fd, off);

  printf("mmap64 unsupported\n");
  abort();
}

int dup(int fd) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_dup(fd);

  printf("dup unsupported\n");
  abort();
}

int dup2(int fd, int fd2) {
  iotrace_init_state();

  iotrace_lock();
  int idx_trace_fds = iotrace_get_idx_trace_fds(fd);
  iotrace_unlock();
  if (idx_trace_fds < 0)
    return iotrace_state->ptr_dup2(fd, fd2);

  printf("dup2 unsupported\n");
  abort();
}
