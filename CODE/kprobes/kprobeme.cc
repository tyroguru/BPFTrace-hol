#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <mutex>
#include <syncstream>

#include <iostream>
#include <string>
#include <thread>

using namespace std::chrono_literals;
std::mutex testMutex;

void blockers(void) {
  /*
   * Create some contention for the static tracepoint exercise
   * although this may not actually do what I want. Leave it here for
   * now anyway.
   */
  auto f = [](void) {
    const std::lock_guard<std::mutex> lock(testMutex);
    std::this_thread::sleep_for(1s);
  };

  std::thread t(f);
  std::thread t1(f);
  std::thread t2(f);

  t.join();
  t1.join();
  t2.join();
}

int main() {
  int err;
  int fd = -1;
  char buf[1024];

  for (auto i = 0; i < INT_MAX; ++i) {
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

    blockers();

    std::this_thread::sleep_for(1s);
  }
}
