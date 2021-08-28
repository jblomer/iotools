#include "util_h5.h"

#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <string>


static void Usage(char *progname) {
  printf("Usage: %s -i <input HDF5 file>\n", progname);
}

int main(int argc, char **argv) {
  std::string inputPath;

  int c;
  while ((c = getopt(argc, argv, "hvi:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'i':
        inputPath = optarg;
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }
  assert(!inputPath.empty());

  H5Row h5row;
  auto fileId = H5Fopen(inputPath.c_str(), H5P_DEFAULT, H5F_ACC_RDONLY);
  assert(fileId >= 0);
  auto setId = H5Dopen(fileId, "/DecayTree", H5P_DEFAULT);
  assert(setId >= 0);
  auto spaceId = H5Screate_simple(1, &H5Row::kDefaultDimension, NULL);
  assert(spaceId >= 0);
  auto memSpaceId = H5Screate(H5S_SCALAR);
  assert(memSpaceId >= 0);

  H5Row::DataSet dataset;
  hsize_t count = 1;
  for (hsize_t i = 0; i < H5Row::kDefaultDimension; ++i) {
    if (i && (i % 100000 == 0))
      printf("  ... processed %llu events\n", i);

    auto retval = H5Sselect_hyperslab(spaceId, H5S_SELECT_SET, &i, NULL, &count, NULL);
    assert(retval >= 0);
    retval = H5Dread(setId, h5row.fTypeId, memSpaceId, spaceId, H5P_DEFAULT, &dataset);
    assert(retval >= 0);
  }

  return 0;
}
