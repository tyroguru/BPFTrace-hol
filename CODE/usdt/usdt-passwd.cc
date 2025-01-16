#include <crypt.h>
#include <iostream>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/sdt.h>
#include <sys/types.h>
#include <unistd.h>

class ProcPass {
public:
  ProcPass(std::string tentry) { testentry = &tentry; }
  ~ProcPass(){};
  void read_etc_passwd(void);

private:
  std::string *testentry;
};

void ProcPass::read_etc_passwd(void) {
  struct passwd *pwd_entry = NULL;

  int found = 0;

  setpwent(); // go to the top of /etc/passwd

  while (!found && (pwd_entry = getpwent())) {
    if (strcmp(testentry->c_str(), pwd_entry->pw_name) == 0) {
      DTRACE_PROBE2(usdt_samp1, passwdfound, pwd_entry->pw_name,
                    pwd_entry->pw_dir);
      found = 1;
    }
  }

  endpwent();
}

int main(int argc, char *argv[]) {
  std::string s("vulnscanner");
  ProcPass p(s);

  while (1) {
    p.read_etc_passwd();
    sleep(1);
  }
}
