#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LEN 1000

void Usage(char *progname) {
  printf("Usage: %s <dev> <milliseconds>\n", progname);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    Usage(argv[0]);
    return 1;
  }

  char *device = argv[1];
  int ms = atoi(argv[2]);

  char cmd_del[LEN];
  char cmd_add[LEN];
  char cmd_show[LEN];
  snprintf(cmd_del, LEN, "/usr/sbin/tc qdisc del dev %s root netem", device);
  snprintf(cmd_add, LEN, "/usr/sbin/tc qdisc add dev %s root netem delay %dms", device, ms);
  snprintf(cmd_show, LEN, "/usr/sbin/tc -s qdisc");

  int retval;
  setuid(geteuid());

  printf("CALL: %s\n", cmd_del);
  retval = system(cmd_del);
  //if (retval != 0) {
  //  fprintf(stderr, "... fail, return code %d\n", retval);
  //  return retval;
  //}

  if (ms > 0) {
    printf("Adding artificial latency of %dms\n", ms);
    printf("CALL: %s\n", cmd_add);
    retval = system(cmd_add);
    if (retval != 0) {
      fprintf(stderr, "... fail, return code %d\n", retval);
      return retval;
    }
  }

  return 0;
}
