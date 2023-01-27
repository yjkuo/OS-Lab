#ifndef HW_MALLOC_H
#define HW_MALLOC_H
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#define KiB 1024

typedef struct chunkhdr *chunk_ptr_t;

typedef struct chunk_info_t {
    unsigned prev_size:31;
    unsigned alloc:1;
    unsigned cur_size:31;
    unsigned mmap:1;
} chunk_info_t;

typedef struct chunkhdr {
    chunk_ptr_t prev;
    chunk_ptr_t next;
    chunk_info_t size_and_flag;
} chunkhdr;
chunk_ptr_t Bin[11];
chunk_ptr_t mmap_head;

chunk_ptr_t split(chunk_ptr_t hdr,int index,int find_index);
void insert(chunk_ptr_t ptr,int index);
chunk_ptr_t search_low(int index);
void Merge(int bin_index,unsigned size,chunk_ptr_t hdr);
void mmap_insert(chunk_ptr_t hdr);
void setInfo(chunk_ptr_t ptr,unsigned alloc,unsigned mmap,unsigned curSize,unsigned prevSize);
int check_addr(chunk_ptr_t ptr);
void *hw_malloc(size_t bytes);
int hw_free(void *mem);
void *get_start_sbrk(void);

#endif
