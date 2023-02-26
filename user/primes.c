
#include "kernel/types.h"

#include "kernel/stat.h"
#include "user/user.h"

const int end = 35;

// k == 0, 1
void redirect(int k, int pipe[2]) {
  close(k);
  dup(pipe[k]);
  close(pipe[0]);
  close(pipe[1]);
}

int find_prime_based_on(int *left_pipe) {
  int child_pid = 0;
  if (left_pipe) {
    redirect(0, left_pipe);
    int ret = 0;
    int from_left = 0;
    int p = 0;
    while ((ret = read(0, &from_left, sizeof(&from_left))) != 0) {
      // Get new number
      if (p == 0) {
        // First time get a value.
        p = from_left;
        printf("prime %d\n", p);
        int right_pipe[2] = {};
        pipe(right_pipe);
        if ((child_pid = fork()) != 0) {
          // Current pid.
          redirect(1, right_pipe);
        } else {
          // Child pid.
          child_pid = find_prime_based_on(right_pipe);
        }
      } else {
        // Test current divider.
        if (from_left % p != 0) {
          // send to right.
          write(1, &from_left, sizeof(&from_left));
        }
      }
    }
    // Terminate

  } else {
    // Root process.
    int first_pipe[2];
    pipe(first_pipe);
    if ((child_pid = fork()) != 0) {
      redirect(1, first_pipe);
      // Current process.
      for (int i = 2; i < end; i++) {
        write(1, &i, sizeof(&i));
      }
    } else {
      child_pid = find_prime_based_on(first_pipe);
    }
  }
  close(1);
  return child_pid;
}

int main() {
  int child_pid = find_prime_based_on(0);
  // Wait child to exits first.
  if (child_pid) {
    wait(&child_pid);
  }
  exit(0);
}
