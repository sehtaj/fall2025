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

#define NBUCKETS 13

struct bucket {
  struct spinlock lock;
  struct buf *head;
};

struct {
  struct bucket buckets[NBUCKETS];
  struct spinlock alloclock;
  struct buf buf[NBUF];
} bcache;

static uint
hash(uint dev, uint blockno)
{
  return (dev * 31 + blockno) % NBUCKETS;
}

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKETS; i++){
    initlock(&bcache.buckets[i].lock, "bcache.bucket");
    bcache.buckets[i].head = 0;
  }
  initlock(&bcache.alloclock, "bcache.alloc");

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  uint h = hash(dev, blockno);
  struct buf *b;

  acquire(&bcache.buckets[h].lock);

  // Is the block already cached?
  for(b = bcache.buckets[h].head; b; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buckets[h].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached, allocate a buffer.
  release(&bcache.buckets[h].lock);

  for(;;){
    acquire(&bcache.alloclock);

    // Find a free buffer.
    struct buf *freeb = 0;
    for(b = bcache.buf; b < bcache.buf + NBUF; b++){
      if(b->refcnt == 0){
        freeb = b;
        break;
      }
    }
    if(!freeb){
      release(&bcache.alloclock);
      panic("bget: no buffers");
    }

    freeb->dev = dev;
    freeb->blockno = blockno;
    freeb->valid = 0;
    freeb->refcnt = 1;

    release(&bcache.alloclock);

    // Insert into bucket.
    acquire(&bcache.buckets[h].lock);

    // Check again if someone added it.
    for(b = bcache.buckets[h].head; b; b = b->next){
      if(b->dev == dev && b->blockno == blockno){
        // Someone added it, put freeb back.
        freeb->refcnt = 0;
        release(&bcache.buckets[h].lock);
        goto retry;
      }
    }

    // Insert freeb.
    freeb->next = bcache.buckets[h].head;
    if(bcache.buckets[h].head)
      bcache.buckets[h].head->prev = freeb;
    bcache.buckets[h].head = freeb;
    freeb->prev = 0;

    release(&bcache.buckets[h].lock);
    acquiresleep(&freeb->lock);
    return freeb;

  retry:;
  }
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

  uint h = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[h].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    if(b->prev)
      b->prev->next = b->next;
    else
      bcache.buckets[h].head = b->next;
    if(b->next)
      b->next->prev = b->prev;
  }
  
  release(&bcache.buckets[h].lock);
}

void
bpin(struct buf *b) {
  uint h = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[h].lock);
  b->refcnt++;
  release(&bcache.buckets[h].lock);
}

void
bunpin(struct buf *b) {
  uint h = hash(b->dev, b->blockno);
  acquire(&bcache.buckets[h].lock);
  b->refcnt--;
  release(&bcache.buckets[h].lock);
}


