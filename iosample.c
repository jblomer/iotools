#include <sys/types.h>
#include <stdio.h>
#include <dlfcn.h>

// GNU extension
#ifndef RTLD_NEXT
#define RTLD_NEXT	((void *) -1l)
#endif

ssize_t read(int fd, void *buf, size_t count) {
  printf("reading from a file\n");
  size_t (*fptr)(int, void *, size_t);
  fptr = dlsym(RTLD_NEXT, "read");
  ssize_t result = (*fptr)(fd, buf, count);
  return result;
}
