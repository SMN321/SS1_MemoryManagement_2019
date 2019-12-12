#include <stdlib.h>
#include "my_alloc.h"
#include "my_system.h"
#include <math.h>

typedef struct header{
    int size;
    struct header *next;
} header;

//stores pointers to the lists of free header
header *headers[6];

int header_size = 0;

void init_my_alloc() {
    for (int i = 0; i < 6; ++i) {
        headers[i] = NULL;
    }
    header_size = sizeof(header);
}

void* my_alloc(size_t size) {
    int index = log2(size) - 3;
    header *first_p = headers[index];
    //empty bucket
    if (!first_p || !first_p->next) {
        char *page = get_block_from_system();
        int package_size = size + header_size;
        int offset = 0;
        int maxOffset = BLOCKSIZE - size - header_size; //so that the last pointer to 'next' does not point outside the block
        for (; offset < maxOffset; offset += size + header_size) {
            *((header *) (page + offset)) = (header) {size, (struct header *) (page + offset + package_size)};
        }
        *((header *) (page + offset)) = (header) {size, NULL};
        if (!first_p)
            first_p = (header *) page;
        else
            first_p->next = (header *) page;
    }
    headers[index] = first_p->next;
    return first_p + 1; //+1 is implicitly converted to +header_size
}

void my_free(void* ptr) {
    header *header_of_ptr = (header *) (((char *) ptr) - header_size);
    int index  = log2(header_of_ptr->size) - 3; //find the size of the memory that ptr points to
    //append header_of_pointer to the front of headers
    header_of_ptr->next = headers[index];
    headers[index] = header_of_ptr;
}
