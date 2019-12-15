#include <stdlib.h>
#include "my_alloc.h"
#include "my_system.h"
#include <stdio.h>

#define LOG(format, vars...) setbuf(stdout, 0); printf(format, vars);
#define LOGT(text) setbuf(stdout, 0); printf(text);
//#define DEBUG

#define BUCKETSIZE (256-8)/8

//store the size of the block in the MSP of the pointer
//all pointers are multiple of 8 hence the 3 LSB are unused
//to encode the size we need 5 bit
//with the 3 bit from the MSB we only have to safe the 2 MSB from a pointer and hope that they are the same for all others
typedef void* sizePointer;

typedef struct header{
    sizePointer info;
} header;

__uint64_t pointer_msb = 0;
__uint64_t lower62 = ~(((__uint64_t)3) << 62); //the cast is necessary to prevent voodoo when shifting signed values
__uint64_t lower4to9 = 0b11111000;
void *null_from_sizePointer = NULL;
bool null_and_msb_set = false;

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
header *bucket[BUCKETSIZE];
int header_size = 0;

void init_my_alloc() {
    for (int i = 0; i < BUCKETSIZE; ++i) {
        bucket[i] = NULL;
    }
    header_size = sizeof(header);
    /*
    printf("NULL pointer: %x\n", NULL);
    void *null_from_size_pointer = get_pointer_from_size_pointer(build_size_pointer(8, NULL));
    printf("NULL from sizePointer: %x\n", null_from_size_pointer);
    */
    /* //the following lines test the sizePointer util methods
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
    int index = (size - 8) / 8;
    if (!bucket[index] || bucket[index] == null_from_sizePointer) { //if the free-list for this size is empty
        char *block = get_block_from_system();
        if (!null_and_msb_set) {
            set_pointer_msb(block);
#ifdef DEBUG
            LOG("pointer MSB: %p\n", pointer_msb)
#endif
            null_from_sizePointer = get_pointer_from_size_pointer(
                    build_size_pointer(size, NULL)); //size does not matter here
#ifdef DEBUG
            LOG("NULL from sizePointer: %x\n", null_from_sizePointer)
#endif
            null_and_msb_set = true;
        }
        bucket[index] = (header *) block;
        int package_size = header_size + size;
        char *next_header = block + header_size + size;
        int maxOffset = BLOCKSIZE - 2*(header_size + size);
        int i = 0;
        for (; i <= maxOffset; i += package_size) {
            ((header *) block)->info = build_size_pointer(size, next_header);
#ifdef DEBUG
            LOG("header: %p, header->info: %x\n", block, ((header *) block)->info)
            LOG("size in header: %d, pointer in header: %p\n", get_size_from_size_pointer(((header *) block)->info), get_pointer_from_size_pointer(((header *) block)->info))
#endif
            block += package_size;
            next_header += package_size;
        }
        ((header *) block)->info = build_size_pointer(size, NULL);
#ifdef DEBUG
        LOG("header: %x, header->info: %x\n", block, ((header *) block)->info)
        LOG("size in header: %d, pointer in header: %x\n", get_size_from_size_pointer(((header *) block)->info), get_pointer_from_size_pointer(((header *) block)->info))
#endif
    }
    void *ret = (void *) (bucket[index] + 1); //add the header-offset
#ifdef DEBUG
    LOG("a. bucket[%d]: %x, bucket[%d]->info: %x\n", index, bucket[index], index, bucket[index]->info)
#endif
    bucket[index] = (header *) get_pointer_from_size_pointer(bucket[index]->info);
#ifdef DEBUG
    //LOG("b. bucket[%d]: %x, bucket[%d]->info: %x\n", index, bucket[index], index, bucket[index]->info)
    LOG("b. bucket[%d]: %x\n", index, bucket[index])
#endif
    return ret;
}

void my_free(void* ptr) {
    header *ptr_header = ((header *) ptr) - 1;
    int size = get_size_from_size_pointer(ptr_header->info);
    int index = (size - 8) / 8;
    //add the returned ptr to the top of the free list
    *ptr_header = (header) {build_size_pointer(size, bucket[index])};
    bucket[index] = ptr_header;
#ifdef DEBUG
    LOG("after free: ptr: %x, bucket[%d]: %x\n", ptr, index, ptr_header);
#endif
}
