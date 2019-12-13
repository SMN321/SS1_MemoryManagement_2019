#include <stdlib.h>
#include "my_alloc.h"
#include "my_system.h"
#include <math.h>
#include <stdio.h>

#define LOG(format, vars...) printf(format, vars)
#define LOGT(text) printf(text)
//#define DEBUG

typedef struct header{
    unsigned int size;
    struct header *next;
} header;

//stores pointers to the lists of free header
header *headers[256/8];

int header_size = 0;

void init_my_alloc() {
    for (int i = 0; i < 6; ++i) {
        headers[i] = NULL;
    }
    header_size = sizeof(header);
}

void* my_alloc(size_t size) {
    int index = size / 8 - 1; //log2(size) - 3;
    header *first_p = headers[index];
#ifdef DEBUG
    LOG("malloc was called with size %d, first_p: %x\n", size, first_p);
#endif
    //empty bucket
    if (!first_p ) {
#ifdef DEBUG
        LOGT("new page ordered because of !first_p\n");
#endif
        char *page = get_block_from_system();
#ifdef DEBUG
        LOG("page: %x, page + BLOCKSIZE: %x\n", page, page + BLOCKSIZE);
#endif
        int package_size = size + header_size;
        int max_offset = (BLOCKSIZE/package_size -1) * package_size;
        char *max_package = ((size_t) page) + ((size_t) max_offset);
        char *current_package = page;
        header *current_header;
        while (current_package < max_package) {
            current_header = current_package;
            *current_header = (header) {size, (struct header *) (current_package + package_size)};
#ifdef DEBUG
            LOG("address: %x, size: %d, address of next package: %x\n", current_header, current_header->size, current_header->next);
#endif
            current_package += package_size;
        }
        current_header = current_package;
        *current_header = (header) {size, NULL};
#ifdef DEBUG
        LOG(" last header in page: address: %x, size: %d, address of next package: %x\n", current_header, current_header->size, current_header->next);
#endif
        first_p = (header *) page;
    }
    headers[index] = first_p->next;
    first_p->next = NULL;
    return first_p + 1; //+1 is implicitly converted to +header_size
}

void my_free(void* ptr) {
#ifdef DEBUG
    LOG("free was called with pointer to %x\n", ptr);
#endif
    header *header_of_ptr = (header *) (((char *) ptr) - header_size);
    int index  = header_of_ptr->size / 8 - 1;//log2(header_of_ptr->size) - 3; //find the size of the memory that ptr points to
#ifdef DEBUG
    LOG("address of header_of_ptr: %x, size: %d, address of next package: %x\n", header_of_ptr, header_of_ptr->size, header_of_ptr->next);
    LOG("currently headers[%d] points to %x\n", index, headers[index]);
#endif
    //append header_of_pointer to the front of headers
    header_of_ptr->next = headers[index];
    headers[index] = header_of_ptr;
#ifdef DEBUG
    LOG("after free: header_of_ptr->next: %x, headers[index]: %x\n", header_of_ptr->next, headers[index]);
#endif
}
