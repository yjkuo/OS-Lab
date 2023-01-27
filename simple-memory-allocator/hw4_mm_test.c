#include "hw4_mm_test.h"
void *start_brk;
int main(int argc, char *argv[])
{
    char buf[50];
    while(fgets(buf,50,stdin)) {
        char *tok = strtok(buf," ");
        if(strcmp(tok,"alloc")==0) {
            tok = strtok(NULL," ");
            void *addr = hw_malloc((size_t)atoi(tok));
            start_brk=get_start_sbrk();
            printf("0x%012lx\n",(long)(addr-start_brk));
        } else if(strcmp(tok,"free")==0) {
            tok = strtok(NULL," ");
            long addr = strtol(tok,NULL,0);
            if(hw_free(start_brk+addr))
                printf("sucess\n");
            else
                printf("fail\n");
        } else if(strcmp(tok,"print")==0) {
            tok = strtok(NULL," ");
            if(strcmp(tok,"mmap_alloc_list\n")==0) {
                chunk_ptr_t cur = mmap_head->next;
                if(cur != NULL) {
                    for(; cur != mmap_head; cur=cur->next) {
                        printf("0x%012lx--------%d\n",(long)(void *)cur,cur->size_and_flag.cur_size);
                    }
                }
            } else {
                int index;
                sscanf(tok,"bin[%d]",&index);
                chunk_ptr_t cur = Bin[index]->next;
                if(cur!=NULL) {
                    for(; cur != Bin[index]; cur=cur->next) {
                        printf("0x%012lx--------%d\n",(long)((void *)cur-start_brk),cur->size_and_flag.cur_size);
                    }
                }
            }
        }
    }
    return 0;
}
