#include "kernel/types.h"

#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

char *filename;

char *fmtname(char *path) {
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  return p;
}

void recursive_find(char *path) {
  if (strcmp(path, "./.") == 0) {
    return;
  }
  int fd;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open [%s]\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    return;
  }

  switch (st.type) {
  case T_DEVICE:
  case T_FILE: {
    if (strcmp(fmtname(path), filename) == 0) {
      // Hit
      printf("%s\n", path);
    }
    break;
  }
  case T_DIR: {
    char buf[512] = {};
    char *p;
    struct dirent de;
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      if ((strcmp(".", de.name) == 0 && strcmp("./", buf) != 0) ||
          strcmp("..", de.name) == 0) {
        // printf("skip: %s, buf %s\n", de.name, buf);
        continue;
      }
      if (stat(buf, &st) < 0) {
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      recursive_find(buf);
    }
    break;
  }
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("wrong args, must be 3, only get %d", argc);
    exit(1);
  }

  filename = argv[2];

  recursive_find(argv[1]);
  exit(0);
}
