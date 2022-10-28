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

#define HASH(blockno) (blockno % NBUCKET)
#define BUF2BKT(b)    HASH(b->blockno)

struct {
    struct spinlock lock[NBUCKET];
    struct buf buf[NBUF];

    // Linked list of all buffers, through prev/next.
    // Sorted by how recently the buffer was used.
    // head.next is most recent, head.prev is least.
    struct buf hashbkt[NBUCKET];
} bcache;

char bkt_name[NBUCKET][10];
void binit(void) {
    struct buf *b;

    // Create linked list of buffers
    for (int i = 0; i < NBUCKET; i++) {
        snprintf(bkt_name[i], 10, "bcache_%d", i);
        initlock(&bcache.lock[i], bkt_name[i]);
        bcache.hashbkt[i].prev = &bcache.hashbkt[i];
        bcache.hashbkt[i].next = &bcache.hashbkt[i];
    }
    struct buf *head = &bcache.hashbkt[0];
    for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
        b->next = head->next;
        b->prev = head;
        initsleeplock(&b->lock, "buffer");
        head->next->prev = b;
        head->next = b;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *bget(uint dev, uint blockno) {
    struct buf *b;
    struct buf *head;
    int bktid = HASH(blockno);

    acquire(&bcache.lock[bktid]);
    head = &bcache.hashbkt[bktid];

    // Is the block already cached?
    for (b = head->next; b != head; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release(&bcache.lock[bktid]);
            acquiresleep(&b->lock);
            return b;
        }
    }

    // Not cached.
    // Recycle the least recently used (LRU) unused buffer.
    for (b = head->prev; b != head; b = b->prev) {
        if (b->refcnt == 0) {
            b->dev = dev;
            b->blockno = blockno;
            b->valid = 0;
            b->refcnt = 1;
            release(&bcache.lock[bktid]);
            acquiresleep(&b->lock);
            return b;
        }
    }

    // Not found unused buffer in current bucket.
    // Recycle from other buckets.
    // acquire with priority to avoid deadlock!!!!
    for (int i = (bktid + 1) % NBUCKET; i != bktid; i = (i + 1) % NBUCKET) {
        acquire(&bcache.lock[i]);
        struct buf *head_i = &bcache.hashbkt[i];
        for (b = head_i->prev; b != head_i; b = b->prev) {
            if (b->refcnt == 0) {
                // Move b to new bucket
                b->next->prev = b->prev;
                b->prev->next = b->next;
                b->next = head->next;
                b->prev = head;
                head->next->prev = b;
                head->next = b;
                // Initialize b
                b->dev = dev;
                b->blockno = blockno;
                b->valid = 0;
                b->refcnt = 1;
                release(&bcache.lock[i]);
                release(&bcache.lock[bktid]);
                acquiresleep(&b->lock);
                return b;
            }
        }
        release(&bcache.lock[i]);
    }

    panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf *bread(uint dev, uint blockno) {
    struct buf *b;

    b = bget(dev, blockno);
    if (!b->valid) {
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b) {
    if (!holdingsleep(&b->lock))
        panic("bwrite");
    virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b) {
    if (!holdingsleep(&b->lock))
        panic("brelse");

    releasesleep(&b->lock);

    int bktid = BUF2BKT(b);
    acquire(&bcache.lock[bktid]);
    b->refcnt--;
    if (b->refcnt == 0) {
        struct buf *head = &bcache.hashbkt[bktid];
        // no one is waiting for it.
        b->next->prev = b->prev;
        b->prev->next = b->next;
        b->next = head->next;
        b->prev = head;
        head->next->prev = b;
        head->next = b;
    }
    release(&bcache.lock[bktid]);
}

void bpin(struct buf *b) {
    int bktid = BUF2BKT(b);
    acquire(&bcache.lock[bktid]);
    b->refcnt++;
    release(&bcache.lock[bktid]);
}

void bunpin(struct buf *b) {
    int bktid = BUF2BKT(b);
    acquire(&bcache.lock[bktid]);
    b->refcnt--;
    release(&bcache.lock[bktid]);
}
