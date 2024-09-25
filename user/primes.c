#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int prepare_context(int new_start_number, int* start_number_pointer, int* has_right_pointer, int data_pipe[2]) {
  int tmp_pipe[2];
  pipe(tmp_pipe);
  int pid = fork();
  if (pid == 0) {
    printf("prime %d\n", new_start_number);
    close(tmp_pipe[1]); // 新的子进程不需要写tmp_pipe
    data_pipe[0] = tmp_pipe[0];
    *start_number_pointer = new_start_number;
    // TODO: 弄清楚这里的bug产生机制，我先猜测是pipe资源不足，感觉很奇怪，输出到 prime 5 就停止了
    // 在这里 close root_pipe[0] 会出现意想不到的bug
    // close(root_pipe[0]); // 子进程不需要读 root_pipe
  } else {
    *has_right_pointer = 1;
    close(tmp_pipe[0]); // 父进程不需要读 tmp_pipe
    close(data_pipe[1]); // 关闭旧的pipe的写端
    data_pipe[1] = tmp_pipe[1];
  }
  return pid;
}

int
main(int argc, char *argv[])
{
  int start_number = 2;
  int max_number = 280;
  int has_right = 0;

  int root_pipe[2];
  pipe(root_pipe);
  int data_pipe[2];
  int pid = prepare_context(start_number, &start_number, &has_right, data_pipe);
  if (pid == 0) {
    // child process
    close(root_pipe[0]); // 子进程不需要读 root_pipe
    int i;
    while (read(data_pipe[0], &i, sizeof(int)) > 0) {
      if (i % start_number != 0) {
        if(has_right == 0) {
          prepare_context(i, &start_number, &has_right, data_pipe);
        } else {
          write(data_pipe[1], &i, sizeof(int));
        }
      }
    }
    close(data_pipe[0]); // 关闭左侧读
    close(data_pipe[1]); // 关闭右侧写
    close(root_pipe[1]); // 关闭 root_pipe 写
  } else {
    // 主进程 
    close(root_pipe[1]); // 主进程不需要写 root_pipe，只负责监听。
    for(int i = 3; i<= max_number; i++) {
      write(data_pipe[1], &i, sizeof(int));
    }
    close(data_pipe[1]); // 关闭右侧写
    char tmp;
    // 等待子进程结束，利用的是pipe的阻塞特性
    // 当pipe所有的读端被关闭时，读操作才会返回0
    while(read(root_pipe[0], &tmp, 1) > 0) { } 
    close(root_pipe[0]);
  }

  exit(0);
}
