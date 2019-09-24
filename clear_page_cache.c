#include <fcntl.h>
#include <unistd.h>

int main() {
  sync();
  char signal = '1';
  int fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
  int nbytes = write(fd, &signal, 1);
  close(fd);
  if (nbytes != 1)
    return 1;
  return 0;
}
