// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

struct buckets {
  struct buf buf[NBUF];
  struct spinlock lock;
} bcache[NBUCKET];

static struct spinlock global_lock;

void
binit(void)
{
  // struct buf *b;
  char buf[20];

  initlock(&global_lock, "bcache-global");

  for (int i = 0; i < NBUF; i++) {
    snprintf(buf, 20, "bcache-%d", i);
    initlock(&bcache[i].lock, buf);
  }

  // // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int k = (dev * 300 + blockno) % NBUCKET;

  acquire(&bcache[k].lock);

  // Is the block already cached?
  for (int i = 0; i < NBUF; i++ ) {
    b = &bcache[k].buf[i];
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache[k].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int i = 0; i < NBUF; i++){
    b = &bcache[k].buf[i];
    // printf("i: %d, b->refcnt: %d\n", i, b->refcnt);
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache[k].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int k = (b->dev * 300 + b->blockno) % NBUCKET;

  acquire(&bcache[k].lock);
  b->refcnt--;
  // if (b->refcnt == 0) {
  //   // no one is waiting for it.
  //   b->next->prev = b->prev;
  //   b->prev->next = b->next;
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
  
  release(&bcache[k].lock);
}

void
bpin(struct buf *b) {
  int k = (b->dev * 300 + b->blockno) % NBUCKET;
  acquire(&bcache[k].lock);
  b->refcnt++;
  release(&bcache[k].lock);
}

void
bunpin(struct buf *b) {
  int k = (b->dev * 300 + b->blockno) % NBUCKET;
  acquire(&bcache[k].lock);
  b->refcnt--;
  release(&bcache[k].lock);
}


