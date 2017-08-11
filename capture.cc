/**
 * Copyright CERN; jblomer@cern.ch
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <TFile.h>
#include <TTree.h>

#include <cassert>
#include <cstdio>
#include <string>

#include "wire_format.h"

using namespace std;


const char *kDefaultFanout = "iotrace.fanout";
const char *kDefaultOutput = "iotrace.root";

int g_pipe_ctrl[2];
std::string g_fanout;
std::string g_output;
TFile *g_root_output{NULL};
TTree *g_root_tree{NULL};

static void CommitTree() {
  printf("Committing ROOT tree... ");
  g_root_output = g_root_tree->GetCurrentFile();
  g_root_output->Write();
  g_root_output->Close();
  delete g_root_output;
  printf("ok\n");
}


static void MakePipe(int pipe_fd[2]) {
  int retval = pipe(pipe_fd);
  assert(retval == 0);
}

/**
 * Writes to a pipe should always succeed.
 */
static void WritePipe(int fd, const void *buf, size_t nbyte) {
  int num_bytes;
  do {
    num_bytes = write(fd, buf, nbyte);
  } while ((num_bytes < 0) && (errno == EINTR));
  assert((num_bytes >= 0) && (static_cast<size_t>(num_bytes) == nbyte));
}


static void SignalExit(int signal) {
  char c = 'T';
  WritePipe(g_pipe_ctrl[1], &c, 1);
}

static void SignalKill(int signal) {
  unlink(g_fanout.c_str());
  CommitTree();
  exit(0);
}

static void Usage(const char *progname) {
  printf("%s [-o output.root] [-s fanout socket]\n", progname);
}


int main(int argc, char **argv) {
  g_fanout = kDefaultFanout;
  g_output = kDefaultOutput;

  int c;
  while ((c = getopt(argc, argv, "hvo:s:")) != -1) {
    switch (c) {
      case 'h':
      case 'v':
        Usage(argv[0]);
        return 0;
      case 'o':
        g_output = optarg;
        break;
      case 's':
        g_fanout = optarg;
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", c);
        Usage(argv[0]);
        return 1;
    }
  }

  g_root_output = new TFile(g_output.c_str(), "RECREATE");
  g_root_tree = new TTree("Events", "Operations performed on the input data");

  MakePipe(g_pipe_ctrl);
  signal(SIGTERM, SignalExit);
  signal(SIGINT, SignalExit);
  signal(SIGQUIT, SignalKill);

  unlink(g_fanout.c_str());
  int retval = mkfifo(g_fanout.c_str(), 0666);
  assert(retval == 0);
  printf("Listening on %s\n", g_fanout.c_str());
  int fd_fanout = open(g_fanout.c_str(), O_RDONLY);
  assert(fd_fanout >= 0);

  struct pollfd watch_fds[2];
  watch_fds[0].fd = g_pipe_ctrl[0];
  watch_fds[1].fd = fd_fanout;
  watch_fds[0].revents = watch_fds[1].revents = 0;
  watch_fds[0].events = watch_fds[1].events = POLLIN | POLLPRI;

  struct iotrace_frame frame;
  ULong64_t var_seqno = 0;
  ULong64_t var_duration_ns;
  Long64_t var_offset;
  Long64_t var_size;
  g_root_tree->Branch("seqno", &var_seqno, "seqno/l");
  g_root_tree->Branch("duration_ns", &var_duration_ns, "duration_ns/l");
  g_root_tree->Branch("offset", &var_offset, "offset/L");
  g_root_tree->Branch("size", &var_size, "size/L");
  while (true) {
    retval = poll(watch_fds, 2, -1);
    if (retval < 0)
      continue;

    if (watch_fds[0].revents)
      break;
    if (!watch_fds[1].revents)
      continue;

    watch_fds[1].revents = 0;
    ssize_t nbytes = read(fd_fanout, &frame, sizeof(frame));
    if (nbytes == 0) {
      printf("pipe closed, reopening\n");
      fd_fanout = open(g_fanout.c_str(), O_RDONLY);
      assert(fd_fanout >= 0);
      watch_fds[1].fd = fd_fanout;
      continue;
    }
    if (nbytes != sizeof(frame)) {
      fprintf(stderr, "invalid frame\n");
      continue;
    }
    switch (frame.op) {
      case IOO_OPEN:
        printf("file was opened (took %ldns)\n", frame.duration_ns);
        break;
      case IOO_READ:
        printf("read %ld bytes in (took %ldns)\n",
               frame.info.read.size, frame.duration_ns);
        var_duration_ns = frame.duration_ns;
        var_size = frame.info.read.size;
        var_offset = 0;
        g_root_tree->Fill();
        break;
      case IOO_SEEK:
        printf("seek %ld bytes (took %ldns)\n",
               frame.info.seek.offset, frame.duration_ns);
        var_duration_ns = frame.duration_ns;
        var_size = -1;
        var_offset = frame.info.seek.offset;
        g_root_tree->Fill();
        break;
      default:
        printf("unknown operation\n");
    }
    var_seqno++;
  }

  printf("commiting tree...\n");
  unlink(g_fanout.c_str());
  CommitTree();
  //delete g_root_tree;
  delete g_root_output;
  return 0;
}
