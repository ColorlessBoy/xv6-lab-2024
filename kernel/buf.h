struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock slock;
  struct spinlock lock;
  uint refcnt;
  uchar data[BSIZE];
};

