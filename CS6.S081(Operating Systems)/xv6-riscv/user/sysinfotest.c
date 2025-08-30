#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void)
{
  struct sysinfo info;

  if (sysinfo(&info) < 0) {
    printf("sysinfotest: sysinfo syscall failed\n");
    exit(1);
  }

  printf("free memory: %lu\n", info.freemem);
  printf("nproc: %lu\n", info.nproc);

  exit(0);
}