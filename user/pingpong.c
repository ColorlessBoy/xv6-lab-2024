#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int parent_message[2];
  int child_message[2];
  char message[128];
  pipe(parent_message);
  pipe(child_message);
  int pid = fork();

  if (pid == 0) {
    // child process
    close(parent_message[1]);
    close(child_message[0]);
    while (read(parent_message[0], message, sizeof(message)) > 0) {
      if (strcmp(message, "ping") == 0) {
        printf("%d: received ping\n", getpid());
        write(child_message[1], "pong", 5);
      }
    }
    close(parent_message[0]);
    close(child_message[1]);
  } else {
    // parent process
    close(parent_message[0]);
    close(child_message[1]);
    write(parent_message[1], "ping", 5);
    while (read(child_message[0], message, sizeof(message)) > 0) {
      if (strcmp(message, "pong") == 0) {
        printf("%d: received pong\n", getpid());
        break;
      }
    }
    close(parent_message[1]);
    close(child_message[0]);
  }

  exit(0);
}
