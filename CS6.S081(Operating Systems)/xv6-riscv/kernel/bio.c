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

static int
hash(uint blockno)
{
  return blockno % NBUCKET;
}

struct {
  struct spinlock bucket_locks[NBUCKET];  // one lock per bucket
  struct buf buf[NBUF];                   // all buffers
  struct buf bucket_heads[NBUCKET];       // each bucket has its own LRU list head
} bcache;

void
binit(void)
{
  struct buf *b;

  for (int i = 0; i < NBUCKET; i++) {
      initlock(&bcache.bucket_locks[i], "bcache.bucket");
      bcache.bucket_heads[i].prev = &bcache.bucket_heads[i];
    }

  for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
    b->next = bcache.bucket_heads[0].next;
    b->prev = &bcache.bucket_heads[0];
    bcache.bucket_heads[0].next->prev = b;
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int idx = hash(blockno);
  struct buf *head = &bcache.bucket_heads[idx];
  acquire(&bcache.bucket_locks[idx]);


  // Is the block already cached?
  for(b = head->next; b != head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_locks[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(int i = 0; i < NBUCKET; i++){
    acquire(&bcache.bucket_locks[i]);
    struct buf *bhead = &bcache.bucket_heads[i];
    for(b = bhead->prev; b != bhead; b = b->prev){
      if(b->refcnt == 0){
        b->prev->next = b->next;
        b->next->prev = b->prev;

        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        b->next = head->next;
        b->prev = head;
        head->next->prev = b;
        head->next = b;

        release(&bcache.bucket_locks[i]);
        release(&bcache.bucket_locks[idx]);

        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.bucket_locks[i]);
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
  int idx = hash(b->blockno);
  acquire(&bcache.bucket_locks[idx]);

  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    struct buf *head = &bcache.bucket_heads[idx];
    b->next = head->next;
    b->prev = head;
    head->next->prev = b;
    head->next = b;
  }
  
  release(&bcache.bucket_locks[idx]);
}

void
bpin(struct buf *b) {
  int idx = hash(b->blockno);
  acquire(&bcache.bucket_locks[idx]);
  b->refcnt++;
  release(&bcache.bucket_locks[idx]);
}

void
bunpin(struct buf *b) {
  int idx = hash(b->blockno);
  acquire(&bcache.bucket_locks[idx]);
  b->refcnt--;
  release(&bcache.bucket_locks[idx]);
}


