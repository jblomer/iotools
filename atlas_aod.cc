
#include <unistd.h>

#include <cstdio>
#include <string>

using namespace std;

void Usage(char *progname) {
  printf("%s -i <input file>\n", progname);
}

int main(int argc, char **argv) {
  string input_path;
  int c;
  while ((c = getopt(argc, argv, "hvi:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        input_path = optarg;
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }

  if (input_path.empty()) {
    Usage(argv[0]);
    return 1;
  }



  return 0;
}
