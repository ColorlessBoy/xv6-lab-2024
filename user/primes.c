#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int prepare_context(int new_start_number, int* start_number_pointer, int* has_right_pointer, int left_pipe[2], int right_pipe[2], int root_pipe[2]) {
  pipe(right_pipe);
  int pid = fork();
  if (pid == 0) {
    printf("prime %d\n", new_start_number);
    close(left_pipe[0]);
    left_pipe[0] = right_pipe[0];
    *start_number_pointer = new_start_number;
    close(right_pipe[1]); // 子进程不需要写 left_pipe
    // TODO: 弄清楚这里的bug产生机制，我先猜测是pipe资源不足
    // 在这里 close root_pipe[0] 会出现意想不到的bug
    // close(root_pipe[0]); // 子进程不需要读 root_pipe
  } else {
    *has_right_pointer = 1;
    close(right_pipe[0]); // 父进程不需要读 right_pipe
  }
  return pid;
}

int
main(int argc, char *argv[])
{
  int left_pipe[2];
  int right_pipe[2];
  int start_number = 2;
  int max_number = 280;
  int has_right = 0;

  int root_pipe[2];
  pipe(root_pipe);
  int pid = prepare_context(start_number, &start_number, &has_right, left_pipe, right_pipe, root_pipe);
  if (pid == 0) {
    // child process
    close(root_pipe[0]); // 子进程不需要读 root_pipe
    int i;
    while (read(left_pipe[0], &i, sizeof(int)) > 0) {
      if (i % start_number != 0) {
        if(has_right == 0) {
          prepare_context(i, &start_number, &has_right, left_pipe, right_pipe, root_pipe);
        } else {
          write(right_pipe[1], &i, sizeof(int));
        }
      }
    }
    close(left_pipe[0]); // 关闭左侧读
    close(right_pipe[1]); // 关闭右侧写
    close(root_pipe[1]); // 关闭 root_pipe 写
  } else {
    // 主进程 
    close(root_pipe[1]); // 主进程不需要写 root_pipe，只负责监听。
    for(int i = 3; i<= max_number; i++) {
      write(right_pipe[1], &i, sizeof(int));
    }
    close(right_pipe[1]); // 关闭右侧写
    char tmp;
    // 等待子进程结束，利用的是pipe的阻塞特性
    // 当pipe所有的读端被关闭时，读操作才会返回0
    while(read(root_pipe[0], &tmp, 1) > 0) { } 
    close(root_pipe[0]);
  }

  exit(0);
}
