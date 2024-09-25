#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void safe_close(int* fd) {
  close(*fd);
  *fd = -1;
}

int
main(int argc, char *argv[])
{
  int start_number = 2;
  int max_number = 280;
  int has_right = 0;

  int root_pipe[2];
  int data_pipe[2];
  pipe(root_pipe);
  pipe(data_pipe);
  if (fork() == 0) {
    // child process
    safe_close(&root_pipe[0]); // 子进程不需要读 root_pipe
    safe_close(&data_pipe[1]); // 子进程不需要写 data_pipe
    printf("prime %d\n", start_number);
    int i;
    int tmp_pipe[2];
    while (read(data_pipe[0], &i, sizeof(int)) > 0) {
      if (i % start_number != 0) {
        if(has_right == 0) {
          pipe(tmp_pipe);
          if (fork() == 0) {
            printf("prime %d\n", i);
            safe_close(&tmp_pipe[1]); // 新的子进程不需要写tmp_pipe
            safe_close(&data_pipe[0]); // 关闭旧的data_pipe的读端
            data_pipe[0] = tmp_pipe[0];
            start_number = i;
          } else {
            has_right = 1;
            safe_close(&tmp_pipe[0]); // 父进程不需要读tmp_pipe
            data_pipe[1] = tmp_pipe[1];
          }
        } else {
          write(data_pipe[1], &i, sizeof(int));
        }
      }
    }
    safe_close(&data_pipe[1]); // 关闭右侧写
    safe_close(&data_pipe[0]); // 关闭左侧读
    safe_close(&root_pipe[1]); // 关闭 root_pipe 写
  } else {
    // 主进程 
    safe_close(&root_pipe[1]); // 主进程不需要写 root_pipe，只负责监听。
    safe_close(&data_pipe[0]); // 主进程不需要读 data_pipe，只负责写。
    int i;
    for(i = 3; i<= max_number; i++) {
      write(data_pipe[1], &i, sizeof(int));
    }
    safe_close(&data_pipe[1]); // 关闭右侧写
    // 等待子进程结束，利用的是pipe的阻塞特性
    // 当pipe所有的读端被关闭时，读操作才会返回0
    read(root_pipe[0], &i, 1);
    safe_close(&root_pipe[0]);
  }

  exit(0);
}
