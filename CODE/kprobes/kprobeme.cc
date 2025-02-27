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
#include <pthread.h>
#include <thread>

using namespace std::chrono_literals;

class Mutex {
private:
    void *_handle;
public:
    Mutex(void *shmMemMutex,  bool recursive =false);
    virtual ~Mutex();

    void lock();
    void unlock();
};

Mutex::Mutex(void *shmMemMutex, bool recursive)
{
    _handle = shmMemMutex;
    pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    // ::pthread_mutexattr_settype(&attr, recursive ? PTHREAD_MUTEX_RECURSIVE_NP : PTHREAD_MUTEX_FAST_NP);

    if (::pthread_mutex_init((pthread_mutex_t*)_handle, &attr) == -1) {
        ::free(_handle);
        throw std::runtime_error("Unable to create mutex");
    }
}
Mutex::~Mutex()
{
    ::pthread_mutex_destroy((pthread_mutex_t*)_handle);
}
void Mutex::lock()
{
    if (::pthread_mutex_lock((pthread_mutex_t*)_handle) != 0) {
        throw std::runtime_error("Unable to lock mutex");
    }
}

void Mutex::unlock()
{
    if (::pthread_mutex_unlock((pthread_mutex_t*)_handle) != 0) {
        throw std::runtime_error("Unable to unlock mutex");
    }
}

pthread_mutex_t testMutex;

void blockers(void) {

  Mutex m(&testMutex);
  /*
   * Create some contention for the static tracepoint exercise
   * although this may not actually do what I want. Leave it here for
   * now anyway.
   */
  auto f = [&m](void) {
    m.lock();
    std::this_thread::sleep_for(2s);
    m.unlock();
  };

  std::thread t(f);
  std::thread t1(f);
  std::thread t2(f);
  std::thread t3(f);
  std::thread t4(f);

  t.join();
  t1.join();
  t2.join();
  t3.join();
  t4.join();
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
