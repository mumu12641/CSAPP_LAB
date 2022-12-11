/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "南通聚集地",
    /* First member's full name */
    "Peng Bin",
    /* First member's email address */
    "pengbin020813@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE 4 /* Word and header/footer size (bytes) */ //
line:vm:mm:beginconst #define DSIZE 8                               /* Double
word size (bytes) */ #define CHUNKSIZE   (1 << 12) /* Extend heap by this
amount (bytes) */ // line:vm:mm:endconst

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc)) // line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))              // line:vm:mm:get
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7) // line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1) // line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)                        //
line:vm:mm:hdrp #define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
// line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE))) //
line:vm:mm:nextblkp #define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char
*)(bp)-DSIZE))) // line:vm:mm:prevblkp

// 获取前驱
#define GET_PRED(bp) GET(bp)
#define SET_PRED(bp,val) PUT(GET_PRED(bp),val)
// 获取后继
#define GET_SUCC(bp) GET((unsigned int *)(bp) + 1)
#define SET_SUCC(bp,val) PUT(GET_SUCC(bp),val)
// #define GET_SUCC(bp) ((unsigned int *)(unsigned int)(GET((unsigned int
*)bp + 1)))

// #define GET_HEAD(num) ((unsigned int *)(int)(GET(heap_listp + WSIZE *
num)))
/* $end mallocmacros */

char *heap_listp = 0; /* Pointer to first block */
#define NONE (unsigned int) -1
/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);

/* operate LinkList */
static void insert(void * bp);
static void delete(void * bp);
static int search(size_t size);
static void *get_head(int num);
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    // mem_init();
    int i = 1;
    if ((heap_listp = mem_sbrk(14 * WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(heap_listp, 0);
    // 放置10个大小类的 head pointer
    // {16 - 31} {31 - 64}...{8192 - INT_MAX}
    for (; i <= 10; i++) {
      PUT(heap_listp + (i * WSIZE), NONE);
    }
    PUT(heap_listp + i * WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + (i + 1) * WSIZE, PACK(DSIZE, 1));
    PUT(heap_listp + (i + 2) * WSIZE, PACK(0, 1));
    heap_listp += WSIZE;
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t asize;     
    size_t extendsize; 
    char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)   
        asize = 2 * DSIZE; 
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE); 

    if ((bp = find_fit(asize)) != NULL) { 
        place(bp, asize);                   
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE); 
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;    
    place(bp, asize); 
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    if(ptr == NULL){
        return;
    }
    size_t size = GET_SIZE(HDRP(ptr));
    // if (heap_listp == 0){
    //     mm_init();
    // }
    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr), PACK(size, 0));
    SET_PRED(ptr, NONE);
    SET_SUCC(ptr, NONE);
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    size = GET_SIZE(HDRP(oldptr));
    copySize = GET_SIZE(HDRP(newptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize - WSIZE);
    mm_free(oldptr);
    return newptr;
}

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
/* $begin mmextendheap */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; 
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    SET_PRED(bp, NONE);
    SET_SUCC(bp, NONE);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
/* $begin mmfree */
static void *coalesce(void *bp) {
    void *prev = PREV_BLKP(bp);
    void *next = NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC(FTRP(prev));
    size_t next_alloc = GET_ALLOC(HDRP(next));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1 */
        insert(bp);
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
      delete(next);
      size += GET_SIZE(HDRP(next));
      PUT(HDRP(bp), PACK(size, 0));
      PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
      delete(prev);
      size += GET_SIZE(HDRP(prev));
      PUT(FTRP(bp), PACK(size, 0));
      PUT(HDRP(prev), PACK(size, 0));
      bp = PREV_BLKP(bp);
    }

    else { /* Case 4 */
      delete (next);
      delete (prev);
      size += GET_SIZE(HDRP(prev)) + GET_SIZE(FTRP(next));
      PUT(HDRP(prev), PACK(size, 0));
      PUT(FTRP(next), PACK(size, 0));
      bp = prev;
    }
    insert(bp);
    return bp;
}
/* $end mmfree */

static void place(void *bp, size_t asize) {
  size_t csize = GET_SIZE(HDRP(bp));

  delete (bp);

    // 分割
  if ((csize - asize) >= 2 * DSIZE) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    void *new_bp = NEXT_BLKP(bp);
    PUT(HDRP(new_bp), PACK(csize - asize, 0));
    PUT(FTRP(new_bp), PACK(csize - asize, 0));
    SET_PRED(new_bp, NONE);
    SET_SUCC(new_bp, NONE);
    coalesce(new_bp);
  }
  else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
  }
}

static void *find_fit(size_t asize){
    int num = search((int)asize);
    // 不分割 不合并
    for(;num <= 9;num++){
        void *head = get_head(num);
        void *bp = GET(head);
        while(bp!=NULL){
            // head = GET
            if (GET_SIZE(HDRP(bp)) >= asize){
                return bp;
            }
            bp = GET_SUCC(bp);
        }
    }

    return NULL;
}

static void insert(void *bp){
    int size =  GET_SIZE(HDRP(bp));
    int num = search(size);
    void *head = get_head(num);
    if (head == NULL){
        PUT(head,bp);
        SET_PRED(bp, NONE);
        SET_SUCC(bp, NONE);
    }else{
        SET_PRED(head,bp);
        SET_SUCC(bp,head);
        PUT(head,bp);
        SET_PRED(bp, NONE);
    }
    // if (get_head(num) == NULL){
    //     PUT(heap_listp + num * WSIZE,bp);
    //     PUT(GET_PRED(bp),NULL);
    //     PUT(GET_SUCC(bp),NULL);
    // }else{
    //     void * old_head = get_head(num);
    //     PUT(heap_listp + num * WSIZE,bp);
    //     PUT(GET_SUCC(bp),get_head(num));
    //     PUT(GET_PRED(get_head(num)),bp);
    //     PUT(GET_PRED(bp),NULL);
    //     PUT(heap_listp + num* WSIZE,bp);
    // }
}

static void delete (void *bp) {
  int size = GET_SIZE((HDRP(bp)));
  int num = search(size);
  void *head = get_head(num);
  void *prev = GET_PRED(bp);
  void *succ = GET_SUCC(bp);
  SET_PRED(bp, NONE);
  SET_SUCC(bp, NONE);
  if (prev == NULL && succ == NULL) {
    PUT(head, NONE);
  } else if (prev == NULL && succ != NULL) {
    SET_PRED(succ, NONE);
    PUT(head, succ);
  } else if (prev != NULL && succ == NULL) {
    SET_SUCC(prev, NONE);
  } else if (prev != NULL && succ != NULL) {
    SET_PRED(succ, prev);
    SET_SUCC(prev, succ);
  }
}

// 返回大小类的序号 从0开始到9
static int search(size_t size) {
  int i;
  for (i = 5; i <= 14; i++) {
    if (size <= ((1 << i) - 1)) {
      return i - 5;
    }
  }
  return i - 5;
}

static void *get_head(int num) {
    return (char*)(heap_listp) + num * WSIZE;
}