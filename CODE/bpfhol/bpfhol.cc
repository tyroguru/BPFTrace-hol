#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
 * This is a bodge implementation just to get things up and running
 * quickly. If this is published anywhere I will rewrite it in an OO style.
 */

std::string options = "1. syscalls\n2. kprobes\n3. usdt\n4. uprobes\n9. exit\n\nSelect Option: ";
std::vector<pid_t> allpids;
std::string BASEDIR;

void execute(std::string& path)
{
  pid_t pid;
  char* argv[] = { const_cast<char*>(path.c_str()), nullptr };
  char* envp[] = { nullptr };

  std::cout << "executing " << path << std::endl;

  if ((pid = fork()) < 0)
  {     /* fork a child process           */
    std::cout  << "ERROR: fork failed :" << " "
      << strerror(errno) <<  std::endl;
    exit(1);
  }
  else if (pid == 0) /* Child */
  {
    if (execve(path.c_str(), argv, envp ) < 0)
    {
      std::cout  << "ERROR: exec failed for " << path << " :"
        << strerror(errno) <<  std::endl;
      exit(1);
    }
  }
  else
  {
      allpids.push_back(pid);
  }
}

void syscalls()
{
  std::string progs[] = { "syscalls/closeall", "syscalls/mapit" };

  for (auto prog : progs )
  {
    execute(prog);
  }
}

void kprobes()
{
  std::string progs[] = { "kprobes/kprobeme" };

  for (auto prog : progs )
  {
    execute(prog);
  }
}

void uprobes()
{
  std::string progs[] = { "uprobes/uprobeme" };

  for (auto prog : progs )
  {
    execute(prog);
  }
}

void usdt()
{
  std::string progs[] = { "usdt/usdt-passwd" };

  for (auto prog : progs )
  {
    execute(prog);
  }
}

void terminate(void)
{
  int ret;

  if (allpids.empty())
    return;

  /* send SIGTERM to all children */
  for (auto pid : allpids)
  {
    if ((ret = kill(pid, SIGTERM)) == -1)
    {
        std::cout << "kill failed on pid " << pid << " :" <<
          strerror(errno) << std::endl;
    }
  }

  int status = 0;
  for (auto pid : allpids)
  {
      if ((waitpid(pid, &status, 0)) != pid)
      {
          std::cout << "wait() failed for pid " << pid <<
            strerror(errno) << std::endl;
      }
      else
      {
          if (WIFSIGNALED(status))
          {
              if (WTERMSIG(status) != SIGTERM)
              {
                  std::cout << "Warning: child process : " << pid <<
                    " killed by signal " << WTERMSIG(status) <<
                    " (expecting SIGTERM)" << std::endl;
              }
          }
          else
          {
            std::cout << "Child process " << pid <<
              " exited for reason other than SIGTERM" << std::endl;
          }
      }
  }
  allpids.clear();
}

void terminate_and_exit()
{
  terminate();
  exit(0);
}

int main(int argc, char *argv[])
{
  std::string inp;
  std::string choice;
  char *t;
  int ret;

  if ((t = getenv("BPFHOL_BASEDIR")) != nullptr)
  {
      BASEDIR = std::string(t);
      std::cout << "BPFHOL_BASEDIR = " << BASEDIR << std::endl;
  }
  else
  {
      BASEDIR = "./load_generators";
  }

  if ((ret = chdir(BASEDIR.c_str())) == -1)
  {
      std::cout << "chdir() to " << BASEDIR << " failed with " <<
        strerror(errno);
  }

  while (1)
  {
    std::cout << options;
    std::getline(std::cin, choice);

    switch (std::stoi(choice)) {
        case 1: terminate();
                syscalls();
                break;

        case 2: terminate();
                kprobes();
                break;

        case 3: terminate();
                usdt();
                break;

        case 4: terminate();
                uprobes();
                break;

        case 9:
                terminate_and_exit();
                break;

        default:
                break;
    }

    std::cout << "\n+-----------+\n";
    std::cout << "Running load generator; choose (exit) to stop";
    std::cout << "\n+-----------+\n\n";
  }
}
