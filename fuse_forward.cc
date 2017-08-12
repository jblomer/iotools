/**
 * Copyright CERN; jblomer@cern.ch
 */

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

using namespace std;

string *g_phys_path;
string *g_log_path;
map<int, int> *g_fd2log;

static string GetRealPath(const char *path) {
  return (*g_phys_path) + string(path);
}

static string GetLogPath(const string &real_path) {
  string chiffre_path(real_path);
  std::replace(chiffre_path.begin(), chiffre_path.end(), '/', '-');
  return *g_log_path + string("/") + chiffre_path;
}

static int MkFuseRetval(int retval) {
  if (retval >= 0)
    return 0;
  return -errno;
}


static int ff_getattr(const char *path, struct stat *info) {
  string real_path = GetRealPath(path);
  int retval = lstat(real_path.c_str(), info);
  return MkFuseRetval(retval);
}


static int ff_open(const char *path, struct fuse_file_info *fi) {
  string real_path = GetRealPath(path);
  int fd = open(real_path.c_str(), fi->flags);
  if (fd >= 0) {
    fi->fh = fd;
    int fd_log = open(GetLogPath(real_path).c_str(),
       O_CREAT | O_APPEND | O_WRONLY, 0644);
    assert(fd_log >= 0);
    (*g_fd2log)[fd] = fd_log;
  }
  return MkFuseRetval(fd);
}


static int ff_read(
  const char *path,
  char *buf,
  size_t size,
  off_t offset,
  struct fuse_file_info *fi)
{
  int nbytes = pread(fi->fh, buf, size, offset);
  if (nbytes < 0)
    return -errno;
  dprintf((*g_fd2log)[fi->fh], "%ld %d\n", offset, nbytes);
  return nbytes;
}


static int ff_release(const char *path, struct fuse_file_info *fi) {
  int retval = close(fi->fh);
  close((*g_fd2log)[fi->fh]);
  g_fd2log->erase(fi->fh);
  return MkFuseRetval(retval);
}


static int ff_opendir(const char *path, struct fuse_file_info *fi) {
  string real_path = GetRealPath(path);
  DIR *dirp = opendir(real_path.c_str());
  if (dirp == NULL)
    return -errno;
  fi->fh = reinterpret_cast<intptr_t>(dirp);
  return 0;
}


int ff_readdir(
  const char *path,
  void *buf,
  fuse_fill_dir_t filler,
  off_t offset,
  struct fuse_file_info *fi)
{
  DIR *dirp = reinterpret_cast<DIR *>(fi->fh);
  struct dirent *entry;
  while ((entry = readdir(dirp)) != NULL) {
    int retval = filler(buf, entry->d_name, NULL, 0);
    assert(retval == 0);
  }
  return 0;
}

int ff_releasedir(const char *path, struct fuse_file_info *fi) {
    DIR *dirp = reinterpret_cast<DIR *>(fi->fh);
    int retval = closedir(dirp);
    return MkFuseRetval(retval);
}


int main(int argc, char **argv) {
  if (getenv("FF_PHYS_PATH") == NULL) {
    printf("Set environment variable FF_PHYS_PATH\n");
    return 1;
  }
  if (getenv("FF_LOG_PATH") == NULL) {
    printf("Set environment variable FF_LOG_PATH\n");
    return 1;
  }
  g_phys_path = new string(getenv("FF_PHYS_PATH"));
  g_log_path = new string(getenv("FF_LOG_PATH"));
  if (g_phys_path->empty() || ((*g_phys_path)[0] != '/')) {
    printf("FF_PHYS_PATH must be absolute\n");
    return 1;
  }
  if (g_log_path->empty() || ((*g_log_path)[0] != '/')) {
    printf("FF_LOG_PATH must be absolute\n");
    return 1;
  }
  g_fd2log = new map<int, int>();

  struct fuse_operations ff_operations;
  memset(&ff_operations, 0, sizeof(ff_operations));
  ff_operations.getattr = ff_getattr;
  ff_operations.open = ff_open;
  ff_operations.read = ff_read;
  ff_operations.release = ff_release;
  ff_operations.opendir = ff_opendir;
  ff_operations.readdir = ff_readdir;
  ff_operations.releasedir = ff_releasedir;

  int retval = fuse_main(argc, argv, &ff_operations, NULL);
  return retval;
}
