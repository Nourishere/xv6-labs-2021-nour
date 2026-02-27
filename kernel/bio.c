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

#define NOBKTS 13
struct bucket {
	struct spinlock lock;
	struct buf head;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct bucket buck[NOBKTS];
} bcache;

uint
bhash(uint blockno)
{
  return blockno % NOBKTS;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  struct bucket *bkt = bcache.buck;
  for(int i = 0; i < NOBKTS; i++)
	initlock(&bkt[i].lock, "block");

  // Create linked list of buffers
  for(int i = 0; i < NOBKTS; i++){
    bkt[i].head.prev = &bkt[i].head;
    bkt[i].head.next = &bkt[i].head;
  }
  for(int i = 0; i < NBUF; i++){
	b = &bcache.buf[i];
	initsleeplock(&b->lock, "buffer");
	struct bucket* bk = &bcache.buck[bhash(i)];
	b->next = bk->head.next;
	b->prev = &bk->head;
	bk->head.next->prev = b;
	bk->head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  struct buf *victim = 0;
  struct bucket* bkt = 0;
  uint64 minticks = ~0;

  for(;;){
    victim = 0;
    minticks = ~0;
	int new_h = bhash(blockno);
	bkt = &bcache.buck[new_h];

	acquire(&bkt->lock);
	// Is the block already cached?
	for(b = bkt->head.next; b != &bkt->head; b = b->next){
	  if(b->dev == dev && b->blockno == blockno){
	    b->refcnt++;
	    release(&bkt->lock);
	    acquiresleep(&b->lock);
	    return b;
	  }
	}
	release(&bkt->lock);

	// No cached block. Allocate one
	acquire(&bcache.lock);
	for(int i = 0; i < NBUF; i++){
	    struct buf *buf = &bcache.buf[i];
	    if(buf->refcnt == 0 && buf->ticks < minticks){
		  minticks = buf->ticks;
		  victim = buf;
	    }
	}
	if(victim == 0)
	    panic("bget: no free buffers");

	int old_h = bhash(victim->blockno);

	struct bucket *first, *second;
	if(old_h < new_h){
	    first = &bcache.buck[old_h];
	    second = &bcache.buck[new_h];
	} else if(old_h > new_h){
	    first = &bcache.buck[new_h];
	    second = &bcache.buck[old_h];
	} else {
	    first = second = &bcache.buck[old_h];
	}

	acquire(&first->lock);
	if(first != second)
	  acquire(&second->lock);
	release(&bcache.lock);

    // Victim may have been grabbed by another thread
    if(victim->refcnt != 0){
      release(&first->lock);
      if(first != second)
		release(&second->lock);
      continue;
    }

    // Another thread may have cached this block while we were working
	for(b = bkt->head.next; b != &bkt->head; b = b->next){
	  if(b->dev == dev && b->blockno == blockno){
	    b->refcnt++;
	    release(&first->lock);
	    if(first != second)
		   release(&second->lock);
	    acquiresleep(&b->lock);
	    return b;
	  }
	}
	// relinking
	victim->prev->next = victim->next;
	victim->next->prev = victim->prev;

	victim->next = bkt->head.next;
	victim->prev = &bkt->head;
	bkt->head.next->prev = victim;
	bkt->head.next = victim;

	victim->refcnt = 1;
	victim->dev = dev;
	victim->blockno = blockno;
	victim->valid = 0;

	release(&first->lock);

	if(first != second)
	  release(&second->lock);

	acquiresleep(&victim->lock);
	return victim;
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

  acquire(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
  
  release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


