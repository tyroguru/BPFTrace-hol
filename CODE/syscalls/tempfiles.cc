#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  /* Improve this - maybe create random string for directory -
   * use folly:randomString() */

  for (;;)
  {
    open("/bilbo/baggins/", __O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
    sleep(5);
  }
}

