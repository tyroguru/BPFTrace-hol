#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <thread>

using namespace std::chrono_literals;

int main() {
  int err;
  int fd = -1;
  char buf[1024];

  for (int i = 0; i < 120; ++i) {
    fd = ::syscall(SYS_open, "/proc/uptime", O_RDONLY);
    if (fd < 0) {
      ::perror("open");
    }

    err = ::syscall(SYS_read, fd, &buf[0], sizeof(buf));
    if (err < 0) {
      ::perror("read");
    }

    // Leaks 1/5 file descriptors
    if (i % 5) {
      err = ::syscall(SYS_close, fd);
      if (err < 0) {
        ::perror("close");
      }
    }

    char filename[] = "/tmp/bpftrace-HOL-.XXXXXX";
    int fd = mkstemp(filename);
    if (fd != -1) {
      close(fd);
      unlink(filename);
    }

    std::this_thread::sleep_for(1s);
  }
}
