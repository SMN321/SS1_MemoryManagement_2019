#ifndef MY_ALLOC_H
#define MY_ALLOC_H

#include <stdlib.h>

/* This function is called exactly once before the first call to
 * my_alloc or my_free.
 */
void init_my_alloc();

/* Return a pointer to size bytes of memory. Size will be a multiple of
 * 8 Bytes. The return value must be aligned to 8 bytes.
 */
void* my_alloc(size_t size);

/* This function is used to free a block of memory that was previously
 * allocated by my_alloc. ptr will always be a pointer that has been
 * returned by my_alloc.
 */
void my_free(void * ptr);

#endif
