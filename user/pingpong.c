#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {

  int parent2child[2];
  int child2parent[2];

  char a_byte[1] = {1};

  pipe(parent2child);
  pipe(child2parent);
  
  char buf[1];
  memset(buf, 0, sizeof(buf));

  int child_pid;

  if ((child_pid = fork()) == 0) {
    // Child process.
    int pid = getpid();

    int ret = read(parent2child[0], buf, sizeof(buf));
    if (ret != 1) {
      printf("ping failed, ret len: %d\n", ret);
      exit(1);
    }

    printf("%d: received ping\n", pid, buf[0]);

    buf[0] += 1;

    write(child2parent[1], buf, sizeof(buf));
  } else {
    // Parent Process
    int pid = getpid();

    int ret = write(parent2child[1], a_byte, sizeof(a_byte));
    if (ret != 1) {
      wait(&child_pid);
      printf("write to ping failed, ret len: %d\n", ret);
      exit(1);
    }

    wait(&child_pid);

    ret = read(child2parent[0], buf, sizeof(buf));
    if (ret != 1) {
      printf("pong failed, ret len: %d\n", ret);
      exit(1);
    }

    printf("%d: received pong\n", pid, buf[0]);
  }

  exit(0);
}
