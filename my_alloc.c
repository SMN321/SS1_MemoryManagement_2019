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
__uint64_t lower4to9 = 0b11111000;


//sizePointer util functions
void *get_pointer_from_size_pointer(sizePointer pointer) {
    return (void *) (pointer_msb | (lower62 & (((__uint64_t) pointer) << 3)));
}

int get_size_from_size_pointer(sizePointer pointer) {
    return ((int) (lower4to9 & ((__uint64_t) pointer >> 56))) + 8;
}

void set_pointer_msb(void *pointer) {
    pointer_msb = (~lower62) & ((__uint64_t) pointer);
}

sizePointer build_size_pointer(int size, void *pointer) {
    __uint64_t p = (__uint64_t) pointer;
    __uint64_t size_normalized = (__uint64_t) ((size - 8) / 8);
    p = (lower62 & p) >> 3;
    size_normalized <<= 59;
    return (sizePointer) (size_normalized | p);
}

//stores pointers to the lists of free header
header *headers[32];

int header_size = 0;

void init_my_alloc() {
    for (int i = 0; i < (256/16+1); ++i) {
        headers[i] = NULL;
    }
    header_size = sizeof(header);
    /* the following lines test the sizePointer util methods
    void *ptr = get_block_from_system();
    set_pointer_msb(ptr);
    for (int size = 8; size <= 256; size += 8) {
        printf("ptr: %x, size: %d\n", ptr, size);
        sizePointer sp = build_size_pointer(size, ptr);
        int restored_size = get_size_from_size_pointer(sp);
        void *restored_ptr = get_pointer_from_size_pointer(sp);
        printf("restored pointer: %x, restored size: %d\n", restored_ptr, restored_size);
    }
    exit(0);
    */
}

void* my_alloc(size_t size) {

}

void my_free(void* ptr) {

}
