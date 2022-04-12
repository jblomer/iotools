#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>

extern void Hugefile_Read(const std::string &filename);
extern void Hugefile_Write(const std::string &filename, size_t nEntries);

[[noreturn]]
static void Usage(const char *argv0) {
   printf("Usage: %s [-n n_entries] read|write path/to/file.ext\n", argv0);
   exit(0);
}

int main(int argc, char *argv[]) {
   size_t nEntries = 0;
   int c;
   while ((c = getopt(argc, argv, "hn:")) != -1) {
      switch (c) {
      case 'n':
         nEntries = static_cast<size_t>(std::atol(optarg));
         break;
      case 'h':
      default:
         Usage(argv[0]);
      }
   }
   if ((argc - optind) != 2)
      Usage(argv[0]);

   std::string op{argv[optind]};
   std::string filename{argv[optind + 1]};
   if (op == "read")
      Hugefile_Read(filename);
   else if (op == "write" && (nEntries != 0))
      Hugefile_Write(filename, nEntries);
   else
      Usage(argv[0]);
   return 0;
}
