#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <syncstream>
#include <cstdlib>
#include <sys/prctl.h>

__attribute__((optnone))
int main(int argc, char *argv[])
{
  std::srand((unsigned) std::time(NULL));

  auto f = [](int index) {
    while (1) {
      std::string tName("sysCallTH" + std::to_string(index));
      prctl(PR_SET_NAME, tName.c_str(), 0, 0, 0);

      // Just do batches of 100 gettime() syscalls to generate some
      // work and then go to sleep for between 50 and 550 millisecs.
      auto sleepTime = 50 + rand() % 500;

      for (auto i = 0; i < 100; ++i) {
        auto tid = gettid();
      }


      // std::osyncstream(std::cout) << "sleep time: " << sleepTime << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }
  };

  std::thread t(f, 1);
  std::thread t2(f, 2);
  std::thread t3(f, 3);
  std::thread t4(f, 4);
  std::thread t5(f, 5);

  t.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
}
