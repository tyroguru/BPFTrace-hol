#include <cstdio>
#include <thread>

using namespace std::chrono_literals;

static int x;

extern "C" {
void foo() {
  x += 2;
}

void bar() {
  x += 5;
}
}

int main() {
  // Runs for 3 minutes
  for (int i = 0; i < 36; ++i) {
    if (i % 2 == 0) {
      foo();
    } else {
      bar();
    }

    std::this_thread::sleep_for(5s);
  }

  printf("%d\n", x);
}
