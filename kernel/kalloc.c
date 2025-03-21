// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);
void superfreerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct superrun {
  struct superrun *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  struct superrun *freelist;
} supermem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&supermem.lock, "supermem");
  freerange(end, (void*)SUPERPGSTART);
  superfreerange((void*)SUPERPGSTART, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  int n = 0;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    kfree(p);
    n += 1;
  }
  printf("kinit: free %d pages\n", n);
}

void 
superfreerange(void *pa_start, void *pa_end)
{
  char *p;
  int n = 0;
  p = (char*)SUPERPGROUNDUP((uint64)pa_start);
  for(; p + SUPERPGSIZE <= (char*)pa_end; p += SUPERPGSIZE) {
    superfree(p);
    n += 1;
  }
  printf("kinit: free %d superpages\n", n);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= SUPERPGSTART)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void
superfree(void *pa)
{
  struct superrun *sr;

  if(((uint64)pa % SUPERPGSIZE) != 0 || (uint64)pa < SUPERPGSTART || (uint64)pa >= PHYSTOP)
    panic("superfree");
  
  memset(pa, 1, SUPERPGSIZE);

  sr = (struct superrun*)pa;

  acquire(&supermem.lock);
  sr->next = supermem.freelist;
  supermem.freelist = sr;
  release(&supermem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void *
superalloc(void)
{
  struct superrun *sr;

  acquire(&supermem.lock);
  sr = supermem.freelist;
  if(sr) 
    supermem.freelist = sr->next;
  release(&supermem.lock);

  if(sr)
    memset((char*)sr, 5, SUPERPGSIZE);
  return (void*)sr;
}

int
countfree(void) 
{
  int n = 0;
  struct run *p = kmem.freelist;
  while(p) {
    n += 1;
    p = p->next;
  }
  return n;
}
