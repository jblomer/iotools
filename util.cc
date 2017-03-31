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
  else if (suffix == "h5") return FileFormats::kHdf5;
  else if (suffix == "sqlite") return FileFormats::kSqlite;
  else abort();
}


std::string StripSuffix(const std::string &path) {
  std::string basename;
  std::string suffix;
  SplitPath(path, &basename, &suffix);
  return basename;
}
