#include <stdlib.h>
#include "my_alloc.h"
#include "my_system.h"
#include <stdio.h>

#define LOG(format, vars...) printf(format, vars)
#define LOGT(text) printf(text)
//#define DEBUG

//store the size of the block in the MSP of the pointer
//all pointers are multiple of 8 hence the 3 LSB are unused
//to encode the size we need 5 bit
//with the 3 bit from the MSB we only have to safe the 2 MSB from a pointer and hope that they are the same for all others
typedef void* sizePointer;

typedef struct header{
    sizePointer info;
} header;

__uint64_t pointer_msb = 0;
__uint64_t lower62 = ~(3 << 62);
__uint64_t size_mask = 0b11111 << 3;

void *get_pointer_from_size_pointer(sizePointer pointer) {
    return (void *) (pointer_msb | (lower62 & (((__uint64_t) pointer) << 3)));
}

int get_size_from_size_pointer(sizePointer pointer) {
    return (int) (size_mask & ((__uint64_t) pointer >> 59));
}
//TODO: test functions
//stores pointers to the lists of free header
header *headers[256/16+1];

int header_size = 0;

void init_my_alloc() {

    for (int i = 0; i < (256/16+1); ++i) {
        headers[i] = NULL;
    }
    header_size = sizeof(header);

}

void* my_alloc(size_t size) {
    int index = size / 16;
    int size_normalized = ((size-1) / 16 + 1) * 16; //pow(2, index + 3);
    if (size_normalized == 0) {
        size_normalized = 8;
    }
    header *first_p = headers[index];
#ifdef DEBUG
    LOG("malloc was called with size %d, first_p: %x, index: %d, size_normalized: %d\n", size, first_p, index, size_normalized);
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
        int package_size = size_normalized + header_size;
        int max_offset = (BLOCKSIZE/package_size -1) * package_size;
        char *max_package = ((size_t) page) + ((size_t) max_offset);
        char *current_package = page;
        header *current_header;
        while (current_package < max_package) {
            current_header = current_package;
            *current_header = (header) {size_normalized, (struct header *) (current_package + package_size)};
#ifdef DEBUG
            LOG("address: %x, size: %d, address of next package: %x\n", current_header, current_header->size, current_header->next);
#endif
            current_package += package_size;
        }
        current_header = current_package;
        *current_header = (header) {size_normalized, NULL};
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
    int index  = (header_of_ptr->size) / 16;//log2(header_of_ptr->size) - 3; //find the size of the memory that ptr points to
#ifdef DEBUG
    LOG("address of header_of_ptr: %x, size: %d, index: %d, address of next package: %x\n", header_of_ptr, header_of_ptr->size, index, header_of_ptr->next);
    LOG("currently headers[%d] points to %x\n", index, headers[index]);
#endif
    //append header_of_pointer to the front of headers
    header_of_ptr->next = headers[index];
    headers[index] = header_of_ptr;
#ifdef DEBUG
    LOG("after free: header_of_ptr->next: %x, headers[index]: %x\n", header_of_ptr->next, headers[index]);
#endif
}
