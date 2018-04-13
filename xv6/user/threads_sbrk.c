/*
 * test to make sure threads handle sbrk() correctly
 * Authors:
 * - Varun Naik, Spring 2018
 */
#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 0x1000
#define check(exp, msg) if(exp) {} else {\
  printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg);\
  printf(1, "TEST FAILED\n");\
  kill(ppid);\
  exit();}

int ppid = 0;
volatile int global = 0;
volatile char *limit = 0;

void
func1(void *arg1, void *arg2)
{
  int pid;
  char *addr;

  // Change external state
  pid = getpid();
  check(pid > ppid, "getpid() returned the wrong pid in func1");
  ++global;

  addr = sbrk(PGSIZE);
  check(addr == limit, "sbrk() failed in func1");
  limit += PGSIZE;

  exit();

  check(0, "Continued after exit in func1");
}

void
func2(void *arg1, void *arg2)
{
  int pid, i;
  char *addr;

  // Change external state
  pid = getpid();
  check(pid > ppid, "getpid() returned the wrong pid in func2");
  // Perform assignment, rather than calling ++global, to avoid a data race
  global = 1;

  // If sbrk() is incorrect, this fails with high probability
  for (i = 0; i < 10000; ++i) {
    addr = sbrk(PGSIZE);
    check(addr == limit || addr == limit + PGSIZE, "sbrk() failed in func2");
    addr = sbrk(-PGSIZE);
    check(addr == limit + PGSIZE || addr == limit + 2*PGSIZE, "sbrk() failed in func2");
  }

  exit();

  check(0, "Continued after exit in func2");
}

void
parallel_sbrk(void)
{
  void *stack1, *stack2;
  int pid1, pid2, status;
  char *addr;

  // Expand address space for stack
  stack1 = sbrk(PGSIZE);
  check(stack1 != (char *)-1, "sbrk() failed in parallel_sbrk");
  check((uint)stack1 % PGSIZE == 0, "sbrk() return value is not page aligned in parallel_sbrk");
  limit = (char *)stack1 + PGSIZE;

  pid1 = clone(&func1, NULL, NULL, stack1);
  check(pid1 > ppid, "clone() failed in parallel_sbrk");

  pid2 = join(&stack2);
  status = kill(pid1);
  check(status == -1, "Child was still alive after join() in parallel_sbrk");
  check(pid1 == pid2, "join() returned the wrong pid in parallel_sbrk");
  check(stack1 == stack2, "join() returned the wrong stack in parallel_sbrk");
  check(global == 1, "global is incorrect in parallel_sbrk");
  check(limit == (char *)stack1 + 2*PGSIZE, "limit is incorrect in parallel_sbrk");

  // Go back to original limit
  addr = sbrk(0);
  printf(1, "Addr is actually: %p; it should be %p", addr, (char*)stack1 + 2*PGSIZE);
  check(addr == (char *)stack1 + 2*PGSIZE, "sbrk() failed in parallel_sbrk");
  addr = sbrk(-2*PGSIZE);
  check(addr == (char *)stack1 + 2*PGSIZE, "sbrk() failed in parallel_sbrk");
}

void
race_sbrk(void)
{
  void *stack1, *stack2, *stack3, *stack4;
  int pid1, pid2, pid3, pid4, status;
  char *addr;

  // Expand address space for stacks
  stack1 = sbrk(2*PGSIZE);
  stack2 = (char *)stack1 + PGSIZE;
  check(stack1 != (char *)-1, "sbrk() failed in race_sbrk");
  check((uint)stack1 % PGSIZE == 0, "sbrk() return value is not page aligned in race_sbrk");
  limit = (char *)stack1 + 2*PGSIZE;

  pid1 = clone(&func2, NULL, NULL, stack1);
  pid2 = clone(&func2, NULL, NULL, stack2);
  check(pid1 > ppid, "clone() failed in race_sbrk");
  check(pid2 > pid1, "clone() failed in race_sbrk");

  pid3 = join(&stack3);
  pid4 = join(&stack4);
  status = kill(pid1);
  check(status == -1, "Child was still alive after join() in race_sbrk");
  status = kill(pid2);
  check(status == -1, "Child was still alive after join() in race_sbrk");

  addr = sbrk(0);
  check((char *)stack1 + 2*PGSIZE == addr, "Limit of address space changed in race_sbrk");
  if (pid1 == pid3) {
    check(pid1 == pid3, "join() returned the wrong pid in race_sbrk");
    check(pid2 == pid4, "join() returned the wrong pid in race_sbrk");
    check(stack1 == stack3, "join() returned the wrong stack in race_sbrk");
    check(stack2 == stack4, "join() returned the wrong stack in race_sbrk");
  } else {
    check(pid1 == pid4, "join() returned the wrong pid in race_sbrk");
    check(pid2 == pid3, "join() returned the wrong pid in race_sbrk");
    check(stack1 == stack4, "join() returned the wrong stack in race_sbrk");
    check(stack2 == stack3, "join() returned the wrong stack in race_sbrk");
  }
  check(global == 1, "global is incorrect in race_sbrk");

  // Go back to original limit
  addr = sbrk(0);
  check(addr == (char *)stack1 + 2*PGSIZE, "sbrk() failed in race_sbrk");
  addr = sbrk(-2*PGSIZE);
  check(addr == (char *)stack1 + 2*PGSIZE, "sbrk() failed in race_sbrk");
}

int
main(int argc, char *argv[])
{
  ppid = getpid();
  check(ppid > 2, "getpid() failed");

  parallel_sbrk();
  global = 0;
  limit = 0;

  race_sbrk();
  global = 0;
  limit = 0;

  printf(1, "PASSED TEST!\n");
  exit();
}
