#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

// An optimized version of copyin that copies data from user to kernel 
// faster by using user pagetable mappings inserted into the process 
// struct by the mapu2kpgtble function.
// Return 0 on success, -1 on error.
int
copyin_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
  struct proc *p = myproc();

  if(srcva + len < srcva || srcva + len > p->sz) 
	return -1;
  memmove((void*)dst, (void*)srcva, len);
  return 0;
}

// An optimized version of copyin string that copies a string from user to kernel
// faster by using user pagetable mappings inserted into the process 
// struct by the mapu2kpgtble function.
// Return 0 on success, -1 on error.
int
copyinstr_new(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
//  struct proc *pr = myproc();
  char *p = (char *)srcva;

//  if (srcva + max < srcva || srcva + max > myproc()->sz)
//   return -1;

  for (uint64 i = 0; i < max; i++) {
    char c = *p++;
    *dst++ = c;
    if (c == '\0')
      return 0;
  }

  return -1;  // no NUL before max
}
