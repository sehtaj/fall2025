#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "memlayout.h"
#include "proc.h"
#include "defs.h"

int copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len){
  if(srcva >= MAXVA)
    return -1;

  if(srcva + len < srcva)
    return -1;

  if(srcva + len > PLIC)
    return -1;

  memmove(dst, (char *)srcva, len);
  return 0;
}