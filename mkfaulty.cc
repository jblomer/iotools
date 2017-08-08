#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

#include "prng.h"

using namespace std;

const unsigned kBufSize = 4096;

void Usage(const char *progname) {
  printf("%s -i <input file> -o <output file> -n <number of bit errors>\n"
         "  [-s <seed>]",
         progname);
}

int main(int argc, char **argv) {
  string input_path;
  string output_path;
  unsigned n_bit_err = 0;
  unsigned seed = 137;

  int c;
  while ((c = getopt(argc, argv, "hvi:o:n:s:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        input_path = optarg;
        break;
      case 'o':
        output_path = optarg;
        break;
      case 'n':
        n_bit_err = atoi(optarg);
        break;
      case 's':
        seed = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }

  if (input_path.empty() || output_path.empty()) {
    Usage(argv[0]);
    return 1;
  }

  int fd_input = open(input_path.c_str(), O_RDONLY);
  if (fd_input < 0) {
    fprintf(stderr, "cannot open %s\n", input_path.c_str());
    return 1;
  }
  int fd_output = open(output_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY,
                       0644);
  if (fd_output < 0) {
    fprintf(stderr, "cannot open %s\n", output_path.c_str());
    return 1;
  }

  struct stat64 info;
  int retval = fstat64(fd_input, &info);
  unsigned mbyte = info.st_size / (1024*1024);
  unsigned byte = info.st_size;
  unsigned bit = info.st_size * 8;
  assert(retval == 0);
  printf("Copy %s --> %s\n", input_path.c_str(), output_path.c_str());
  printf("File size: %u MB    %u B    %u Bit\n", mbyte, byte, bit);

  vector<unsigned> err_pos;
  Prng prng;
  prng.InitSeed(seed);
  for (unsigned i = 0; i < n_bit_err; ++i) {
    unsigned pos = prng.Next(bit);
    err_pos.push_back(pos);
  }
  sort(err_pos.begin(), err_pos.end());
  for (unsigned i = 0; i < n_bit_err; ++i)
    printf("  marking bit at position %u\n", err_pos[i]);

  unsigned char buf[kBufSize];
  int nbytes;
  unsigned total_bits = 0;
  unsigned total_bytes = 0;
  while ((nbytes = read(fd_input, buf, kBufSize)) != 0) {
    total_bytes += nbytes;
    total_bits += nbytes * 8;
    while (!err_pos.empty() && (total_bits > err_pos[0])) {
      unsigned bit_pos = err_pos[0];
      err_pos.erase(err_pos.begin());

      unsigned byte_pos = bit_pos / 8;
      unsigned byte_in_buffer = byte_pos - (total_bytes - nbytes);
      printf("  flipping bit at position Bit %u (Byte %u), "
             "Byte %u in current buffer of size %u Byte\n",
             bit_pos, byte_pos, byte_in_buffer, nbytes);

      unsigned char original_byte = buf[byte_in_buffer];
      buf[byte_in_buffer] ^= (1 << (bit_pos % 8));
      printf("    changed byte %#x to %#x\n", original_byte,
             buf[byte_in_buffer]);
    }

    int written = write(fd_output, buf, nbytes);
    assert(written == nbytes);
  }

  close(fd_input);
  close(fd_output);

  return 0;
}
