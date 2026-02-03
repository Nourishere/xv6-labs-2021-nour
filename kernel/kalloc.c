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

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
#if(_LAB_COW == 1)
// reference count for each page
uint64 refcount[PHYSTOP/PGSIZE];
// refernce lock
struct spinlock reflock;
#endif
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
	#if(_LAB_COW == 1)
	refcount[(uint64)p/PGSIZE]=1;
	#endif
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  #if(_LAB_COW == 1)
  uint64 np = (uint64) pa / PGSIZE;
  acquire(&reflock);
  if(refcount[np] < 1)
	panic("kfree: COW: cannot free freed!\n");
  refcount[np] -= 1;
  release(&reflock);

  if(refcount[np] != 0)
	return;
  #endif
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
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
  if(r){
    kmem.freelist = r->next;
    #if(_LAB_COW == 1)
	acquire(&reflock);
	uint64 np = (uint64) r / PGSIZE;
	if(refcount[np] != 0)
	  panic("kalloc: first alloc has ref\n");
	refcount[np] += 1;
	release(&reflock);
	#endif
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

// Return the amount of free memory the system has.
uint64
freecount(void)
{
	uint64 free=0;
	struct run *p = 0;
	p = kmem.freelist;
	while(p){
		p = p -> next;
		free++;
	}
	return free * PGSIZE; 
}
