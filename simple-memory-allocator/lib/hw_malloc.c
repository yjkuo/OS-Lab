#include "hw_malloc.h"
chunk_ptr_t alloc_head[50];
size_t mmap_threshold=32*KiB;
void *start_addr=NULL;
int firstmalloc=1;
chunkhdr bin_hdr[11];
chunkhdr mmap_hdr;
int allocIndex;

void *hw_malloc(size_t bytes)
{
    int index=0;
    size_t bestSize=64*KiB;
    size_t chunkSize=bytes+24;
    if(firstmalloc) {
        start_addr=sbrk(64*KiB);
        if(start_addr==(void *)-1)return NULL;
        /*Initialize*/
        for(int i=0; i<11; ++i) {
            Bin[i]=&bin_hdr[i];
            Bin[i]->next=NULL;
            Bin[i]->prev=NULL;
        }
        mmap_head=&mmap_hdr;
        mmap_head->next=NULL;
        mmap_head->prev=NULL;
        firstmalloc=0;
        chunk_ptr_t first,second;
        first=start_addr;
        setInfo(first,0,0,32*KiB,0);
        second=start_addr+32*KiB;
        setInfo(second,0,0,32*KiB,32*KiB);
        insert(first,10);
        insert(second,10);
    }
    if(chunkSize > mmap_threshold) {
        /*mmap*/
        chunk_ptr_t addr;
        void *start;
        size_t tmp=bytes;
        int mmap_size=0;
        while(tmp>1) {
            tmp/=16;
            mmap_size++;
        }
        mmap_size-=3;
        start = mmap(0,chunkSize,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        if(start == MAP_FAILED) {
            printf("mmap failed\n");
            return NULL;
        }
        addr=start;
        mmap_insert(addr);
        setInfo(addr,1,1,bytes+24,addr->prev->size_and_flag.cur_size);
        alloc_head[allocIndex++]=addr;
        if(allocIndex==50)allocIndex=0;
        return start+(long)start_addr;
    } else {
        while(bestSize >= chunkSize)
            bestSize /= 2;
        bestSize *=2;
        while(bestSize>1) {
            bestSize /=2;
            index++;
        }
        for(int i=index-5; i<11; ++i) {
            if(Bin[i]->next != NULL) {
                chunk_ptr_t lowest=search_low(i);
                chunk_ptr_t user=split(lowest,index-5,i);
                user->size_and_flag.prev_size=0;
                user->size_and_flag.alloc=1;
                user->size_and_flag.mmap=0;
                user->prev->next=user->next;
                user->next->prev=user->prev;
                if(Bin[i]->next==Bin[i]) {
                    Bin[i]->prev=Bin[i]->next=NULL;
                }
                alloc_head[allocIndex++]=user;
                if(allocIndex==50)allocIndex=0;
                return (void *)user+24;
            }
        }
    }
    return NULL;
}

int hw_free(void *mem)
{
    chunk_ptr_t mem_hdr;
    if((long)(mem-(long)start_addr)<64*KiB)
        mem_hdr=mem-24;
    else
        mem_hdr=mem-(long)start_addr;
    if(!check_addr(mem_hdr))return 0;
    unsigned size = mem_hdr->size_and_flag.cur_size;
    int index=0;
    while(size>1) {
        size /=2;
        index++;
    }
    index-=5;
    if(mem_hdr->size_and_flag.mmap) {
        mem_hdr->prev->next=mem_hdr->next;
        mem_hdr->next->prev=mem_hdr->prev;
        mem_hdr=NULL;
        if(mmap_head->next==mmap_head) {
            mmap_head->next=mmap_head->prev=NULL;
        }
        munmap(mem_hdr,size);
        return 1;
    } else {
        Merge(index,mem_hdr->size_and_flag.cur_size,mem_hdr);
        return 1;
    }
    return 0;
}

void *get_start_sbrk(void)
{
    return start_addr;
}

void Merge(int bin_index,unsigned size,chunk_ptr_t hdr)
{
    chunk_ptr_t cur = Bin[bin_index]->next;
    if(cur==NULL || bin_index==10) {
        hdr->size_and_flag.alloc=0;
        insert(hdr,bin_index);
        return;
    }
    for(; cur != Bin[bin_index]; cur=cur->next) {
        if(cur==((void *)hdr+size)) {
            hdr->size_and_flag.cur_size *= 2;
            cur->next->prev=cur->prev;
            cur->prev->next=cur->next;
            cur = NULL;
            if(Bin[bin_index]->next==Bin[bin_index]) {
                Bin[bin_index]->next=Bin[bin_index]->prev=NULL;
            }
            Merge(bin_index+1,size*2,hdr);
            return;
        } else if(cur==((void *)hdr-size)) {
            cur->size_and_flag.cur_size *= 2;
            cur->next->prev=cur->prev;
            cur->prev->next=cur->next;
            hdr = NULL;
            if(Bin[bin_index]->next==Bin[bin_index]) {
                Bin[bin_index]->next=Bin[bin_index]->prev=NULL;
            }
            Merge(bin_index+1,size*2,cur);
            return;
        } else {
            hdr->size_and_flag.alloc=0;
            insert(hdr,bin_index);
            return;
        }
    }
}
void mmap_insert(chunk_ptr_t hdr)
{
    if(mmap_head->next==NULL) {
        mmap_head->next = hdr;
        mmap_head->prev = hdr;
        hdr->next = mmap_head;
        hdr->prev = mmap_head;
        return;
    } else {
        chunk_ptr_t cur=mmap_head->next;
        for(; cur != mmap_head; cur=cur->next) {
            if(hdr <= cur) {
                hdr->next = cur;
                hdr->prev = cur->prev;
                cur->prev->next=hdr;
                cur->prev = hdr;
                return;
            }
        }
        hdr->next = mmap_head;
        hdr->prev = mmap_head->prev;
        mmap_head->prev->next=hdr;
        mmap_head->prev = hdr;
    }
}
chunk_ptr_t split(chunk_ptr_t hdr,int index,int find_index)
{
    while(find_index!=index) {
        hdr->size_and_flag.cur_size /= 2;
        find_index--;
        hdr->prev->next=hdr->next;
        hdr->next->prev=hdr->prev;
        unsigned s =hdr->size_and_flag.cur_size;
        chunk_ptr_t header_ptr;
        header_ptr = (void *)hdr+s;
        header_ptr->size_and_flag.cur_size=s;
        header_ptr->size_and_flag.alloc=0;
        header_ptr->size_and_flag.mmap=0;
        insert(header_ptr,find_index);
        header_ptr->size_and_flag.prev_size = header_ptr->prev->size_and_flag.cur_size;
    }
    return hdr;

}
void insert(chunk_ptr_t ptr,int index)
{
    if(Bin[index]->next==NULL) {
        Bin[index]->next = ptr;
        Bin[index]->prev = ptr;
        ptr->next = Bin[index];
        ptr->prev = Bin[index];
    } else {
        Bin[index]->prev->next = ptr;
        ptr->prev = Bin[index]->prev;
        ptr->next = Bin[index];
        Bin[index]->prev = ptr;
    }
}
chunk_ptr_t search_low(int index)
{
    chunk_ptr_t cur=Bin[index]->next;
    chunk_ptr_t min=cur;
    for(; cur != Bin[index]; cur=cur->next) {
        if(cur<min)
            min=cur;
    }
    return min;
}

void setInfo(chunk_ptr_t ptr,unsigned alloc,unsigned mmap,unsigned curSize,unsigned prevSize)
{
    ptr->size_and_flag.alloc=alloc;
    ptr->size_and_flag.mmap=mmap;
    ptr->size_and_flag.cur_size=curSize;
    ptr->size_and_flag.prev_size=prevSize;
}
int check_addr(chunk_ptr_t ptr)
{
    for(int i=0; i<50; i++) {
        if(alloc_head[i]==ptr)return 1;
    }
    return 0;
}