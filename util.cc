/**
 * Copyright CERN; jblomer@cern.ch
 */

#include "util.h"

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
  else if (suffix == "avro") return FileFormats::kAvro;
  else if (suffix == "avro-inflated") return FileFormats::kAvroInflated;
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
