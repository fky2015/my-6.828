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
  // Global array to track CoW pages.
  int cow_pages[(uint64) PHYSTOP / PGSIZE + 1];
} kmem;

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
  
  int need_free = 0;
  acquire(&kmem.lock);
    uint64 idx = PGROUNDDOWN((uint64)pa) / PGSIZE;
    kmem.cow_pages[idx] -= 1;
    if (kmem.cow_pages[idx] <= 0) {
      need_free = 1;
      kmem.cow_pages[idx] = 0;
    }
  release(&kmem.lock);

  if (!need_free){
    return;
  } 


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
  if(r)
    {
      kmem.freelist = r->next;
      uint64 idx = PGROUNDDOWN((uint64)r) / PGSIZE;
      kmem.cow_pages[idx] = 1;
    }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void
kref(void * pa)
{
  acquire(&kmem.lock);
  uint64 idx = PGROUNDDOWN((uint64)pa) / PGSIZE;
  kmem.cow_pages[idx] += 1;
  // printf("pa: %p refs %d\n", PGROUNDDOWN((uint64) pa), kmem.cow_pages[idx]);
  release(&kmem.lock);
}

// Duplicate COW page.
//
// Return 1 if pte is not pointed to a valid COW page.
// Return 2 if there is a mapping error.
int de_cow(pagetable_t pagetable, pte_t *pte, uint64 va) {
  // printf("[de_COW] pagetable: %p, pte_t: %p, va: %p\n", pagetable, *pte, va);

  if (!((*pte & RSV_1) && (*pte & PTE_V) && (*pte & PTE_U))) 
    return 1;
    
  uint64 pa;
  uint flags;
  char *mem;
  pa = PTE2PA(*pte);
  // Restore PTE_W
  flags = PTE_FLAGS(*pte);
  flags |= PTE_W;
  flags &= ~RSV_1;
  if ((mem = kalloc()) == 0)
    goto err;
  
  memmove(mem, (char *)pa, PGSIZE);
  
  // NOTE: Need to ummap current page first.
  uvmunmap(pagetable, PGROUNDDOWN(va), 1, 1);

  if (mappages(pagetable, PGROUNDDOWN(va), PGSIZE, (uint64)mem, flags) != 0) {
    printf("[de_cow] error in mappages");
    kfree(mem);
    goto err;
  }

  // Turn on PTE_W.
  *pte |= PTE_W;
  // printf("[de_COW] new_pa: %p\n", mem);

  return 0;
  
  err:
  return 2;
}
