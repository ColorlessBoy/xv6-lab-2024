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

struct {
  struct buf buf[NBUF];
} bcache;

void
binit(void)
{
  for (int i = 0; i < NBUF; i++) {
    initlock(&(bcache.buf[i].lock), "bcache");
    initsleeplock(&(bcache.buf[i].slock), "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  int index = blockno % NBUF;
  for (int i = 0; i < NBUF; i++, index = (index + 1) % NBUF) {
    acquire(&bcache.buf[index].lock);
    struct buf *b = &(bcache.buf[index]);
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.buf[index].lock);
      acquiresleep(&b->slock);
      return b;
    }
    release(&bcache.buf[index].lock);
  }
  for (int i = 0; i < NBUF; i++, index = (index + 1) % NBUF) {
    acquire(&bcache.buf[index].lock);
    struct buf *b = &(bcache.buf[index]);
    if(b->refcnt == 0){
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.buf[index].lock);
      acquiresleep(&b->slock);
      return b;
    }
    release(&bcache.buf[index].lock);
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
  if(!holdingsleep(&b->slock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->slock))
    panic("brelse");
  releasesleep(&b->slock);
  acquire(&b->lock);
  b->refcnt--;
  release(&b->lock);
}

void
bpin(struct buf *b) {
  acquire(&b->lock);
  b->refcnt++;
  release(&b->lock);
}

void
bunpin(struct buf *b) {
  acquire(&b->lock);
  b->refcnt--;
  release(&b->lock);
}


