// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

struct spinlock pgref;
#define PAGE_ID(p) (((p) - KERNBASE) / PGSIZE)
#define MAX_PAGE_ID PAGE_ID(PHYSTOP)
int pageref[MAX_PAGE_ID];

#define PA2PAGE(p) pageref[PAGE_ID((uint64)(p))]

struct spinlock cntpage;

int cntpg;

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

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&pgref, "pgref");
  initlock(&cntpage, "cntpage");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&pgref);
  if (--PA2PAGE(pa) <= 0) {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&pgref);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  cntpg++;
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    PA2PAGE(r) = 1;
  }
  return (void*)r;
}


void
krefpage(void* pa) {
  acquire(&pgref);
  PA2PAGE(pa)++;
  release(&pgref);
}

void*
kcopy_n_deref(void* pa) {
  acquire(&pgref);
  if (PA2PAGE(pa) <= 1) {
    release(&pgref);
    return (void*) pa;
  }

  uint64 new = (uint64)kalloc();

  if (new == 0) {
    release(&pgref);
    return 0;
  }
  memmove((void*)new, (void*)pa, PGSIZE);

  PA2PAGE(pa)--;

  release(&pgref);

  return (void*)new;
}
