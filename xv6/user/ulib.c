#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

#define PGSZ (0x1000)

char*
strcpy(char *s, char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
  char *dst, *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

int
thread_create(void (*start_routine)(void*, void*), void* arg1, void* arg2) {
//printf(1, "before malloc\n");
  void* mem_start = malloc(PGSZ);
  if(mem_start == NULL)
    return -1;
  //printf(1, "after malloc\n");
  void* ustack = mem_start;
  uint start_mod = (uint) mem_start % PGSZ;
  // check if page aligned
  if(start_mod != 0) {
  //  printf(1, "page alignment issue incoming...\n");
    ustack = (void*)ustack + (uint)(PGSZ - start_mod);
    //ustack += (start_mod);
  }
//  printf(1, "mem_start: %p, ustack: %p\n\n", mem_start, ustack);
  *((uint*)ustack) = (uint) mem_start;
  //printf(1, "mem_start addr (in create): %p\n", mem_start);
  //printf(1, "mem_start: %p, ustack: %p\n", mem_start, ustack);
  //*((uint*)(ustack + PGSZ - sizeof(void*))) = (uint) mem_start; // put malloc'd ptr as first thing on stack
  return clone(start_routine, arg1, arg2, ustack);
}

int
thread_join() {
  void* ustack = NULL;
  int join_ret = join(&ustack);
//  if(join_ret != -1) {
  //  void* free_ptr = (void*)*((uint*)(ustack + PGSZ - sizeof(void*)));
  //printf(1, "mem_start addr (in join): %p\n", ustack);
  uint* free_ptr = (uint*)ustack;
    free((void*)*free_ptr);
//  }
  //free(ustack);
  return join_ret;
}

void
lock_init(lock_t* lock) {
  lock->ticket = 0;
  lock->turn = 0;
}

void
lock_acquire(lock_t* lock) {
  int myturn = fetch_and_add(&lock->ticket, 1);
  while(lock->turn != myturn)
    ;
}

void
lock_release(lock_t* lock) {
  lock->turn = lock->turn + 1;
}
