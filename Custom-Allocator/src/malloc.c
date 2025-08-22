#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

/* Counters */
static int atexit_registered = 0;
static int num_mallocs = 0;
static int num_frees = 0;
static int num_reuses = 0;
static int num_grows = 0;
static int num_splits = 0;
static int num_coalesces = 0;
static int num_blocks = 0;
static int num_requested = 0;
static int max_heap = 0;
static double total_malloc_time = 0;

/* Print heap statistics at exit */
void printStatistics(void) {
    printf("\nheap management statistics\n");
    printf("mallocs:        %d\n", num_mallocs);
    printf("frees:          %d\n", num_frees);
    printf("reuses:         %d\n", num_reuses);
    printf("grows:          %d\n", num_grows);
    printf("splits:         %d\n", num_splits);
    printf("coalesces:      %d\n", num_coalesces);
    printf("blocks:         %d\n", num_blocks);
    printf("requested:      %d\n", num_requested);
    printf("max heap:       %d\n", max_heap);
    printf("total malloc time (seconds): %.6f\n", total_malloc_time);
}

/* Block metadata structure */
struct _block {
    size_t size;
    struct _block *next;
    struct _block *prev;
    bool free;
    char padding[3]; /* Alignment */
};

/* Global variables for the heap */
struct _block *heapList = NULL;
struct _block *last_alloc = NULL;

/* Find a free block using the specified allocation strategy */
struct _block *findFreeBlock(struct _block **last, size_t size) {
    struct _block *curr = heapList;

#if defined FIT && FIT == 0
    /* First Fit */
    while (curr && !(curr->free && curr->size >= size)) {
        *last = curr;
        curr = curr->next;
    }
#endif

#if defined BEST && BEST == 0
    /* Best Fit */
    struct _block *best = NULL;
    while (curr) {
        if (curr->free && curr->size >= size) {
            if (!best || curr->size < best->size) {
                best = curr;
            }
        }
        *last = curr;
        curr = curr->next;
    }
    curr = best;
#endif

#if defined WORST && WORST == 0
    /* Worst Fit */
    struct _block *worst = NULL;
    while (curr) {
        if (curr->free && curr->size >= size) {
            if (!worst || curr->size > worst->size) {
                worst = curr;
            }
        }
        *last = curr;
        curr = curr->next;
    }
    curr = worst;
#endif

#if defined NEXT && NEXT == 0
    /* Next Fit */
    if (last_alloc) {
        curr = last_alloc->next;
    } else {
        curr = heapList;
    }
    struct _block *start = curr;
    while (curr) {
        if (curr->free && curr->size >= size) {
            last_alloc = curr;
            return curr;
        }
        *last = curr;
        curr = curr->next;
        if (!curr) {
            curr = heapList;
        }
        if (curr == start) {
            break;
        }
    }
    return NULL;
#endif

    return curr;
}

/* Request more space from the OS */
struct _block *growHeap(struct _block *last, size_t size) {
    struct _block *curr = (struct _block *)sbrk(0);
    struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

    assert(curr == prev);
    if (curr == (struct _block *)-1) {
        return NULL;
    }

    if (!heapList) {
        heapList = curr;
    }

    if (last) {
        last->next = curr;
    }

    curr->size = size;
    curr->next = NULL;
    curr->free = false;

    num_grows++;
    max_heap += size + sizeof(struct _block);
    return curr;
}

/* Allocate memory with timing */
void *malloc(size_t size) {
    if (atexit_registered == 0) {
        atexit_registered = 1;
        atexit(printStatistics);
    }

    size = ALIGN4(size);
    if (size == 0) {
        return NULL;
    }

    clock_t start_time = clock(); // Start timing

    num_requested += size;

    struct _block *last = heapList;
    struct _block *next = findFreeBlock(&last, size);

    if (next) {
        num_reuses++;
        if (next->size > size + sizeof(struct _block) + 4) {
            struct _block *split = (struct _block *)((char *)next + sizeof(struct _block) + size);
            split->size = next->size - size - sizeof(struct _block);
            split->free = true;
            split->next = next->next;

            next->size = size;
            next->next = split;

            num_splits++;
            num_blocks++;
        }
    } else {
        next = growHeap(last, size);
        if (!next) {
            return NULL;
        }
        num_blocks++;
    }

    next->free = false;
    num_mallocs++;

    clock_t end_time = clock(); // End timing
    total_malloc_time += (double)(end_time - start_time) / CLOCKS_PER_SEC;

    return BLOCK_DATA(next);
}

/* Free memory */
void free(void *ptr) {
    if (!ptr) {
        return;
    }

    struct _block *curr = BLOCK_HEADER(ptr);
    assert(curr);

    curr->free = true;
    num_frees++;

    if (curr->next && curr->next->free) {
        curr->size += sizeof(struct _block) + curr->next->size;
        curr->next = curr->next->next;
        num_coalesces++;
        num_blocks--;
    }

    if (curr->prev && curr->prev->free) {
        curr->prev->size += sizeof(struct _block) + curr->size;
        curr->prev->next = curr->next;
        num_coalesces++;
        num_blocks--;
    }
}

/* Allocate and zero memory */
void *calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void *ptr = malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

/* Resize memory */
void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    struct _block *curr = BLOCK_HEADER(ptr);
    if (curr->size >= size) {
        return ptr;
    }

    void *new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, curr->size);
        free(ptr);
    }
    return new_ptr;
}
