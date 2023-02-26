#include "kernel/types.h"

#include "kernel/param.h"
#include "kernel/stat.h"
#include "user/user.h"

char *my_argv[MAXARG];

char buf[500];
char in;

/**
 * @brief Read a line into buf, `\n` will not be appended to the end of the buf.
 *
 * @param buf
 * @return 0 for read something, -1 for read nothing.
 */
int read_line(char *buf) {
  char *p = buf;
  int ret = -1;
  while (read(0, &in, 1) == 1) {
    ret = 0;
    if (in == '\n')
      break;
    *p++ = in;
  }
  *p = 0;
  return ret;
}

int main(int argc, char *argv[]) {

  int i = 0;
  for (; i < argc - 1; i++) {
    my_argv[i] = argv[i + 1];
  }

  my_argv[i] = buf;

  while (read_line(buf) == 0) {

    int child_id = 0;
    if ((child_id = fork())) {
      // Parent.
      wait(&child_id);
    } else {
      // Child.
      exec(my_argv[0], my_argv);
      break;
    }
  }

  exit(0);
}
