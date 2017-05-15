/**
 * Copyright CERN; jblomer@cern.ch
 */

#define __STDC_FORMAT_MACROS

#include "util.h"

#include <inttypes.h>

#include <cstdio>
#include <cstdlib>

static void SplitPath(
  const std::string &path,
  std::string *basename,
  std::string *suffix)
{
  size_t idx_dot = path.find_last_of(".");
  if (idx_dot == std::string::npos) {
    *basename = path;
    suffix->clear();
  } else {
    *basename = path.substr(0, idx_dot);
    *suffix = path.substr(idx_dot + 1);
  }
}


FileFormats GetFileFormat(const std::string &suffix) {
  if (suffix == "root") return FileFormats::kRoot;
  else if (suffix == "h5row") return FileFormats::kH5Row;
  else if (suffix == "h5column") return FileFormats::kH5Column;
  else if (suffix == "sqlite") return FileFormats::kSqlite;
  else if (suffix == "avro-deflated") return FileFormats::kAvroDeflated;
  else if (suffix == "avro-inflated") return FileFormats::kAvroInflated;
  else if (suffix == "protobuf-deflated") return FileFormats::kProtobufDeflated;
  else if (suffix == "protobuf-inflated") return FileFormats::kProtobufInflated;
  else if (suffix == "root-deflated") return FileFormats::kRootDeflated;
  else if (suffix == "root-inflated") return FileFormats::kRootInflated;
  else if (suffix == "parquet-deflated") return FileFormats::kParquetDeflated;
  else if (suffix == "parquet-inflated") return FileFormats::kParquetInflated;
  else if (suffix == "parquet-snappy") return FileFormats::kParquetSnappy;
  else abort();
}


std::string GetSuffix(const std::string &path) {
  std::string basename;
  std::string suffix;
  SplitPath(path, &basename, &suffix);
  return suffix;
}


std::string JoinStrings(
  const std::vector<std::string> &strings,
  const std::string &joint)
{
  std::string result = "";
  const unsigned size = strings.size();

  if (size > 0) {
    result = strings[0];
    for (unsigned i = 1; i < size; ++i)
      result += joint + strings[i];
  }

  return result;
}


std::vector<std::string> SplitString(
  const std::string &str,
  const char delim,
  const unsigned max_chunks)
{
  std::vector<std::string> result;

  // edge case... one chunk is always the whole string
  if (1 == max_chunks) {
    result.push_back(str);
    return result;
  }

  // split the string
  const unsigned size = str.size();
  unsigned marker = 0;
  unsigned chunks = 1;
  unsigned i;
  for (i = 0; i < size; ++i) {
    if (str[i] == delim) {
      result.push_back(str.substr(marker, i-marker));
      marker = i+1;

      // we got what we want... good bye
      if (++chunks == max_chunks)
        break;
    }
  }

  // push the remainings of the string and return
  result.push_back(str.substr(marker));
  return result;
}


std::string StripSuffix(const std::string &path) {
  std::string basename;
  std::string suffix;
  SplitPath(path, &basename, &suffix);
  return basename;
}

uint64_t String2Uint64(const std::string &value) {
  uint64_t result;
  sscanf(value.c_str(), "%" PRIu64, &result);
  return result;
}


std::string StringifyUint(const uint64_t value) {
  char buffer[48];
  snprintf(buffer, sizeof(buffer), "%" PRIu64, value);
  return std::string(buffer);
}
