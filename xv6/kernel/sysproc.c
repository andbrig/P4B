#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

int
sys_clone(void) {
  void *arg1;
  void *arg2;
  void *stack;
  void *function;

  // retrieve from stack
  if(argptr(0, (void*)&function, sizeof(void*)) == -1 ||
    argptr(1, (void*)&arg1, sizeof(void*)) == -1 ||
    argptr(2, (void*)&arg2, sizeof(void*)) == -1 ||
    argptr(3, (void*)&stack, sizeof(void*)) == -1) {
      cprintf("bad args\n");
      return -1;
  }

  // check page alignment and stack size
  if((uint) stack % PGSIZE != 0 || proc->sz - (uint)stack < PGSIZE) {
    cprintf("page alignment or size error\n");
    return -1;
  }

  return clone(function, arg1, arg2, stack);
}
int
sys_join(void) {
  void** stack;

  // retrieve from stack
  if(argptr(0, (void*)&stack, sizeof(void*)) == -1) {
    return -1;
  }

  return join(stack);
}
int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&sbrk_lock);
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  release(&sbrk_lock);
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
