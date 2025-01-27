#include <errno.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

/*
 * This is a bodge implementation just to get things up and running
 * quickly. If this is published anywhere I will rewrite it in an OO style.
 */
std::string options = "1. core\n2. syscalls\n3. kernel\n4. usdt\n4. uprobes\n"
                      "8. stop current generator\n9. exit\n\nSelect Option: ";
std::vector<pid_t> allpids;
std::string BASEDIR;

void execute(std::string &path) {
  pid_t pid;
  char *argv[] = {const_cast<char *>(path.c_str()), nullptr};
  char *envp[] = {nullptr};

  std::cout << "executing " << path << std::endl;

  if ((pid = fork()) < 0) {
    /* fork a child process           */
    std::cout << "ERROR: fork failed :" << " " << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  } else if (pid == 0) /* Child */
  {
    if (execve(path.c_str(), argv, envp) < 0) {
      std::cout << "ERROR: exec failed for " << path << " :" << strerror(errno)
                << std::endl;
      exit(EXIT_FAILURE);
    }
  } else {
    allpids.push_back(pid);
  }
}

void syscalls() {
  std::string progs[] = {"syscalls/closeall", "syscalls/mapit",
                         "syscalls/tempfiles"};

  for (auto prog : progs) {
    execute(prog);
  }
}

void core() {
  std::string progs[] = {"core/core"};

  for (auto prog : progs) {
    execute(prog);
  }
}

void kprobes() {
  std::string progs[] = {"kprobes/kprobeme"};

  for (auto prog : progs) {
    execute(prog);
  }
}

void uprobes() {
  std::string progs[] = {"uprobes/uprobeme"};

  for (auto prog : progs) {
    execute(prog);
  }
}

void usdt() {
  std::string progs[] = {"usdt/usdt-passwd"};

  for (auto prog : progs) {
    execute(prog);
  }
}

void terminate(void) {
  if (allpids.empty())
    return;

  /* send SIGTERM to all children */
  for (auto pid : allpids) {
    if (kill(pid, SIGTERM) == -1) {
      std::cerr << "kill failed on pid " << pid << " : " << strerror(errno)
                << std::endl;
    }
  }

  int status = 0;
  for (auto pid : allpids) {
    if ((waitpid(pid, &status, 0)) != pid) {
      std::cout << "wait() failed for pid " << pid << strerror(errno)
                << std::endl;
    } else {
      if (WIFSIGNALED(status)) {
        if (WTERMSIG(status) != SIGTERM) {
          std::cout << "Warning: child process : " << pid
                    << " killed by signal " << WTERMSIG(status)
                    << " (expecting SIGTERM)" << std::endl;
        }
      } else {
        std::cout << "Child process " << pid
                  << " exited for reason other than SIGTERM" << std::endl;
      }
    }
  }
  allpids.clear();
}

void terminate_and_exit() {
  terminate();
  exit(0);
}

int main(int argc, char *argv[]) {
  std::string inp;
  std::string choice;
  char *t;

  if ((t = getenv("BPFHOL_BASEDIR")) != nullptr) {
    BASEDIR = std::string(t);
    std::cout << "BPFHOL_BASEDIR = " << BASEDIR << std::endl;
  } else {
    BASEDIR = "./load_generators";
  }

  if (chdir(BASEDIR.c_str()) == -1) {
    std::cerr << "chdir() to " << BASEDIR << " failed with " << strerror(errno)
              << std::endl;
    exit(EXIT_FAILURE);
  }

  while (1) {
    std::cout << options;
    std::cin >> choice;

    switch (std::stoi(choice)) {
    case 1:
      terminate();
      core();
      break;

    case 2:
      terminate();
      syscalls();
      break;

    case 3:
      terminate();
      kprobes();
      break;

    case 4:
      terminate();
      usdt();
      break;

    case 5:
      terminate();
      uprobes();
      break;

    case 8:
      terminate();
      break;

    case 9:
      terminate_and_exit();
      break;

    default:
      break;
    }
  }
}
