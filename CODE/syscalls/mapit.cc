#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
  int pgsz = getpagesize();
  std::vector<std::pair<void*, int>> mappings;

  /*
   * We simply want to create a bunch of mappings and then
   * tear down those mappings.
   */
  for (;;)
  {
    for (int i = 0; i < 10000; i++)
    {
     int mapsz = ((i % 10) + 1) * pgsz;;
     void *addr = mmap(NULL, mapsz, PROT_READ | PROT_WRITE,
                       (i % 4 ? MAP_SHARED : MAP_PRIVATE) | MAP_ANONYMOUS,
                       -1, 0);

     mappings.push_back(std::make_pair(addr, mapsz));
    }

    sleep(1);

    /* Now remove the mappings */
    for (const std::pair<void*, int> &mapping : mappings)
    {
      munmap(mapping.first, mapping.second);
    }

    mappings.clear();

    sleep(1);
  }
}
