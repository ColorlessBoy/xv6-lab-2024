#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"


void get_argument(char* buffer, int max) {
  gets(buffer, max);
  for(int i = 0; i < strlen(buffer); i++) {
    if(buffer[i] == '\0') {
      break;
    }
    if(buffer[i] == '\n' || buffer[i] == '\r') {
      buffer[i] = '\0';
      break;
    }
  }
}

int
main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "Usage: xargs command command_arg...\n");
    exit(1);
  }

  char buffer[MAXARG];

  // ["xargs", "echo", "arg1", "arg2", "arg3"]
  // after the loop: ["echo", "arg1", "arg2", "arg3", "arg3"]
  for(int i = 1; i < argc; i++) {
    argv[i-1] = argv[i];
  }
  argv[argc-1] = buffer;

  get_argument(buffer, sizeof(buffer));
  while(strlen(buffer) > 0){
    if(fork() == 0) {
      exec(argv[0], argv);
      exit(0);
    } else {
      wait(0);
      get_argument(buffer, sizeof(buffer));
    }
  }
  exit(0);
}