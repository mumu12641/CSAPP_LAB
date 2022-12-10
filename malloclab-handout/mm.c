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
#define WSIZE 4 /* Word and header/footer size (bytes) */ // line:vm:mm:beginconst
#define DSIZE 8                               /* Double word size (bytes) */
#define CHUNKSIZE   (1 << 12) /* Extend heap by this amount (bytes) */ // line:vm:mm:endconst

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
#define HDRP(bp) ((char *)(bp) - WSIZE)                        // line:vm:mm:hdrp
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE))) // line:vm:mm:nextblkp
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp)-DSIZE))) // line:vm:mm:prevblkp

// 获取前驱
#define GET_PRED(bp) (unsigned int *)(int)(GET(bp))
// 获取后继
#define GET_SUCC(bp) ((unsigned int *)(int)(GET((unsigned int *)bp + 1)))

// #define GET_HEAD(num) ((unsigned int *)(int)(GET(heap_listp + WSIZE * num)))
/* $end mallocmacros */

char *heap_listp = 0; /* Pointer to first block */

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);

/* operate LinkList */
void insert(void * bp);
void delete(void * bp);
int search(size_t size);
unsigned int *get_head(int num);
void set_head(unsigned int *addr,int num);
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    int i = 1;
    if ((heap_listp = mem_sbrk((4 + 10) * WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(heap_listp, 0);
    // 放置10个大小类的 head pointer
    // {16 - 31} {31 - 64}...{8192 - INT_MAX}
    for (; i <= 10; i++) {
        PUT(heap_listp + (i * WSIZE), NULL);
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
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {}

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
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
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

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; // line:vm:mm:beginextend
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL; // line:vm:mm:endextend

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));
    /* Free block header */ // line:vm:mm:freeblockhdr
    PUT(FTRP(bp), PACK(size, 0));
    /* Free block footer */ // line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    /* New epilogue header */ // line:vm:mm:newepihdr

    /* Coalesce if the previous block was free */
    return coalesce(bp); // line:vm:mm:returnblock
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
/* $begin mmfree */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1 */
        insert(bp);
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
      delete(NEXT_BLKP(bp));
      size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
      PUT(HDRP(bp), PACK(size, 0));
      PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
      delete (PREV_BLKP(bp));
      size += GET_SIZE(HDRP(PREV_BLKP(bp)));
      PUT(FTRP(bp), PACK(size, 0));
      PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
      bp = PREV_BLKP(bp);
    }

    else { /* Case 4 */
      delete (NEXT_BLKP(bp));
      delete (PREV_BLKP(bp));
      size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
      PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
      PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
      bp = PREV_BLKP(bp);
    }
    insert(bp);
    return bp;
}
/* $end mmfree */

void insert(void *bp){
    int size =  GET_SIZE(HDRP(bp));
    int num = search(size);
    // unsigned int * pred = GET_PRED(bp);
    // unsigned int * succ = GET_SUCC(bp);
    // unsigned int * old_head = get_head(num);
    // set_head(pred,num);
    // *((unsigned int *) bp + 1) = *old_head; 
    if (get_head(num) == NULL){
        // set_head
        PUT(heap_listp + num * WSIZE,bp);
        
    }
}

void delete(void *bp){
    int size = GET_SIZE((HDRP(bp)));
    int num = search(size);
    unsigned int *pred = GET_PRED(bp);
    unsigned int *succ = GET_SUCC(bp);
    // pred.next = succ
    *GET_SUCC(GET_PRED(pred)) = *succ;
}

// 返回大小类的序号 从0开始到9
int search(size_t size) {
    int i ;
    for(i = 5 ;i <= 14;i++){
        if(size <= ((1 << i)-1)){
            return i - 5;
        }
    }
    return i - 5;
}

unsigned int *get_head(int num){
    return (unsigned int *)(heap_listp) + num;
}

void set_head(unsigned int * addr,int num){
    PUT(heap_listp + WSIZE * num,addr);
} 