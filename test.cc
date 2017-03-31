/**
 * Copyright CERN; jblomer@cern.ch
 */

#define _LARGEFILE64_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>

#include <cassert>
#include <cstdio>

int main() {
  const char *filename{"Makefile"};
  char buf;
  int fd = open(filename, O_RDONLY);

  int retval = read(fd, &buf, 1);
  assert(retval == 1);
  retval = pread(fd, &buf, 1, 1);
  assert(retval == 1);
  retval = pread64(fd, &buf, 1, 2);
  assert(retval == 1);
  retval = lseek64(fd, 0, SEEK_END);
  assert(retval > 0);
  retval = lseek(fd, 0, SEEK_SET);
  assert(retval == 0);
  struct iovec iov;
  iov.iov_base = &buf;
  iov.iov_len = 1;
  retval = readv(fd, &iov, 1);
  assert(retval == 1);


  close(fd);
  return 0;
}
