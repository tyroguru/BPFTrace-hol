#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
    for (;;)
    {
        sleep(5);
        for (int i = 65535; i > 3; i--)
        {
            close(i);
        }
    }
}
