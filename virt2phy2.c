#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#define DEBUG 1

#define PAGEMAP_ENTRY 8
#define GET_BIT(X, Y) (X & ((uint64_t)1 << Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF


uint64_t   page_size;

unsigned long int read_pagemap(int pid, unsigned long virt_addr)
{
    int status;
    FILE *f;
    char pagemap_path_buf[0x100] = {};
    
    unsigned long  file_offset;
    unsigned long  page_entry;


    sprintf(pagemap_path_buf, "/proc/%u/pagemap", pid);
    f = fopen(pagemap_path_buf, "rb");
    if (!f)
    {
        printf("Error! Cannot open %s\n", pagemap_path_buf);
        return -1;
    }

    file_offset = virt_addr / page_size * PAGEMAP_ENTRY;
    printf("pagemap offset  : 0x%lx\n", file_offset);
    status = fseek(f, file_offset, SEEK_SET);
    if (status)
    {
        perror("Failed to do fseek!");
        return -1;
    }

    status = fread(&page_entry, 1, 8, f);
    printf("pagemap entry   : 0x%lx\n", page_entry);
    if (GET_BIT(page_entry, 63))
    {
        uint64_t pfn = GET_PFN(page_entry);

        if (GET_BIT(page_entry, 62))
        {
            printf("Page swapped\n");
            return 0;
        }
        printf("PFN             : 0x%lx \nPhysical address: 0x%lx\n", (unsigned long)pfn, (unsigned long)(pfn * page_size + virt_addr % page_size));
    }
    else
    {
        printf("Page not present\n");
    }

    fclose(f);
    return 0;
}
int main(int argc, char **argv)
{
    int pid;
    char *end;
    unsigned long virt_addr;

    if(argc!=3){
        printf("Argument number is not correct!\n pagemap PID VIRTUAL_ADDRESS\n");
        return -1;
    }
    if(!memcmp(argv[1],"self",sizeof("self"))){
        pid = -1;
    }
    else{
        pid = strtol(argv[1],&end, 10);
        if (end == argv[1] || *end != '\0' || pid<=0){
            printf("PID must be a positive number or 'self'\n");
            return -1;
        }
    }
    virt_addr = strtoll(argv[2], NULL, 16);
    page_size = getpagesize();

    read_pagemap(pid,virt_addr);
    return 0;
}