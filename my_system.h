#ifndef MY_SYSTEM_H
#define MY_SYSTEM_H

#include <stdbool.h>
#include <stdlib.h>

#define BLOCKSIZE	8192

/* Get a 1024-Byte aligned Block of Memory from the System. The return
 * value is 0 if no more memory is availiable. Otherwise it points to
 * the newly allocated block of memory.
 */
void * get_block_from_system();

/*
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

/* Internal Functions and data structures for the tester. */
size_t get_sys_blockcount ();
bool valid_area (size_t start, size_t len);

struct avl_node {
	struct avl_node * next, * prev;
	struct avl_node * left, * right;
	struct avl_node * parent;
	size_t start, len;
	int height;
};

struct avl_node * create_avl (void);
struct avl_node * find_avl (struct avl_node * root, size_t start);
void insert_avl (struct avl_node ** root, size_t start, size_t len);
void remove_avl (struct avl_node ** root, struct avl_node * node);
void __my_assert (char * text);
#define my_assert(B,T) do { if(!(B)) __my_assert(T); } while (0)

#endif
