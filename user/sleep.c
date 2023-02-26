// Sleep for a number of ticks

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("usage: sleep TICK_NUMBER\n");
    exit(1);
  } else if (argc > 2) {
    printf("sleep: too many arguments\n");
    printf("usage: sleep TICK_NUMBER\n");
    exit(1);
  }

  int ticks = atoi(argv[1]);

  if (ticks < 0) {
    printf("sleep: TICK_NUMBER must be a positive integer\n");
    exit(1);
  }

  
  sleep(ticks);

  exit(0);
}
