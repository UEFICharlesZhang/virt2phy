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

const int __endian_bit = 1;
#define is_bigendian() ((*(char *)&__endian_bit) == 0)

uint64_t read_val, file_offset, page_size;

FILE *f;
char *end;

int pid_self;

unsigned long int read_virt(char *path_buf, unsigned long virt_addr)
{
    int i, c, status;

#if DEBUG
    printf("Big endian? %d\n", is_bigendian());
#endif
    f = fopen(path_buf, "rb");
    if (!f)
    {
        printf("Error! Cannot open %s\n", path_buf);
        return -1;
    }

    //Shifting by virt-addr-offset number of bytes
    //and multiplying by the size of an address (the size of an entry in pagemap file)
    file_offset = virt_addr / page_size * PAGEMAP_ENTRY;
#if DEBUG
    printf("Vaddr: 0x%lx, Page_size: %lld, Entry_size: %d\n", virt_addr, (long long int)page_size, PAGEMAP_ENTRY);
    printf("Reading %s at 0x%llx\n", path_buf, (unsigned long long)file_offset);
#endif
    status = fseek(f, file_offset, SEEK_SET);
    if (status)
    {
        perror("Failed to do fseek!");
        return -1;
    }
    errno = 0;
    read_val = 0;
    unsigned char c_buf[PAGEMAP_ENTRY];
    for (i = 0; i < PAGEMAP_ENTRY; i++)
    {
        c = getc(f);
        if (c == EOF)
        {
            printf("\nReached end of the file\n");
            return 0;
        }
        if (is_bigendian())
            c_buf[i] = c;
        else
            c_buf[PAGEMAP_ENTRY - i - 1] = c;
#if DEBUG
        printf("[%d]0x%x ", i, c);
#endif
    }
    for (i = 0; i < PAGEMAP_ENTRY; i++)
    {
#if DEBUG
        printf("%d ", c_buf[i]);
#endif
        read_val = (read_val << 8) + c_buf[i];
    }
#if DEBUG
    printf("\n");
    printf("Result: 0x%llx\n", (unsigned long long)read_val);
#endif
    if (GET_BIT(read_val, 63))
    {
        uint64_t pfn = GET_PFN(read_val);
#if DEBUG
        printf("PFN: 0x%llx (0x%llx)\n", (long long unsigned int)pfn, (long long unsigned int)(pfn * page_size + virt_addr % page_size));
#endif
        return (long long unsigned int)(pfn * page_size + virt_addr % page_size);
    }
    else
#if DEBUG
        printf("Page not present\n");
#endif
    if (GET_BIT(read_val, 62))
#if DEBUG
        printf("Page swapped\n");
#endif
    fclose(f);
    return 0;
}
int parasmaps(int pid)
{
    FILE *f_maps;
    char buf[256];

    unsigned long start, end;
    unsigned long offset;
    unsigned long p_start;

    char path_buf[0x100] = {};
    char maps_path_buf[0x100] = {};

    if (pid != -1)
    {
        sprintf(path_buf, "/proc/%u/pagemap", pid);
        sprintf(maps_path_buf, "/proc/%u/maps", pid);
    }
    else
    {
        sprintf(path_buf, "/proc/self/pagemap");
        sprintf(maps_path_buf, "/proc/self/maps");
    }

    printf("maps_path_buf: %s, path_buf: %s\n", maps_path_buf, path_buf);

    f_maps = fopen(maps_path_buf, "rb");
    if (!f_maps)
    {
        printf("Error! Cannot open %s\n", maps_path_buf);
        return -1;
    }

    printf("memory map for pid: %s;\n", maps_path_buf);

    while (fscanf(f_maps, "%zx-%zx %s %zx %*[^\n]\n", &start, &end, buf, &offset) == 4)
    {
        printf("  v_start: %012lX-%012lX, ", start, end);
        p_start = read_virt(path_buf, start);
        printf("  p_start: %012lX-%012lX %s %08lX \n", p_start, p_start == 0 ? 0 : p_start + end - start, buf, offset);
    }

    fclose(f_maps);
}
int main(int argc, char **argv)
{
    int pid;
    char path_buf[0x100] = {};

    if (argc != 2)
    {
        printf("Argument number is not correct!\n virt2phy PID\n");
        return -1;
    }
    if (!memcmp(argv[1], "self", sizeof("self")))
    {
        pid = -1;
    }
    else
    {
        pid = strtol(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || pid <= 0)
        {
            printf("PID must be a positive number or 'self'\n");
            return -1;
        }
    }

    page_size = getpagesize();

    parasmaps(pid);
    return 0;
}