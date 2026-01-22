#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "kalloc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  #if (LAB_LAZY == 1)
  addr = myproc() -> sz;
  if(n < 0){
    myproc()->sz += n;
    uint64 new = myproc()->sz;
	// check for bounds
	if(new < myproc()->heap_base)
	  return -1;
	// free the memory
	uvmunmap(myproc()->pagetable, PGROUNDDOWN(new), (PGROUNDDOWN(addr)-PGROUNDDOWN(new))/PGSIZE, 0);
  }
  else
    myproc()->sz += n;
  #else
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  #endif
  #if (LAB_PGTBLE == 1)
  mapu2kpgtbl(myproc()->pagetable, myproc()->kpagetable, 0, myproc()->sz);
  #endif
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  backtrace();
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/* author: Nour
 * trace systemcalls 
 */
uint64
sys_trace(void)
{
  struct proc *p = myproc();
  int n;
  if(argint(0, &n) < 0)
    return -1;
  p -> mask = n;	
  return 0;
}

#if (LAB_TRAP == 1)
// alarm the user of usage after a specified time
uint64
sys_sigalarm(void)
{
  struct proc* p = myproc();
  // fetch arguments
  if(argint(0, &(p->alarm_ticks)) < 0)
    return -1;
  if(argaddr(1, &(p->alarm_handler)) < 0)
    return -1;

  return 0;
}
#endif

#if(LAB_TRAP == 1)
uint64
sys_sysinfo(void)
{
	struct sysinfo info;
	uint64 uaddr;
	if (argaddr(0, &uaddr) < 0)
   		return -1;
	info.freemem = freecount(); 
	info.nproc = proccount();
    if(copyout(myproc()->pagetable, uaddr, (char*) &info, sizeof(info)) < 0)
		return -1;
	return 0;
}
#endif

#if(LAB_TRAP == 1)
uint64
sys_sigreturn(void){
    struct proc* p = myproc();
	// restore saved registers
	p->trapframe->epc = p->saved_epc;
	p->trapframe->a0  = p->reg[1];
	p->trapframe->a1  = p->reg[2];
	p->trapframe->a2  = p->reg[3];
	p->trapframe->a3  = p->reg[4];
	p->trapframe->a4  = p->reg[5];
	p->trapframe->a5  = p->reg[6];
	p->trapframe->a6  = p->reg[7];
	p->trapframe->a7  = p->reg[8];
	p->trapframe->ra  = p->reg[9];
	p->trapframe->sp  = p->reg[10];
	p->trapframe->s0  = p->reg[11];
	p->trapframe->s1  = p->reg[12];
	p->trapframe->s2  = p->reg[13];
	p->trapframe->s3  = p->reg[14];
	p->trapframe->s4  = p->reg[15];
	p->trapframe->s5  = p->reg[16];
	p->trapframe->s6  = p->reg[17];
	p->trapframe->s7  = p->reg[18];
	p->trapframe->s8  = p->reg[19];
	p->trapframe->s9  = p->reg[20];
	p->trapframe->s10 = p->reg[21];
	p->trapframe->s11 = p->reg[22];

	// free the flag for another interrupt
	p->alarmflag = 1;
	return 0;
}
#endif
