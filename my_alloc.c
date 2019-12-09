#include <stdlib.h>
#include "my_alloc.h"
#include "my_system.h"

static size_t offset = 0;
static char* data = 0;

//arrays which hold the distance between two consecutive mask placements
//do something like (mask <<= delta[i]) and reuse mask, char[] to safe memory
//the initial place of the mask is with zero shift and has to be used before the delta-array is used, equivalently
//would be to add a 0 at delta[0]
//maybe calculate the deltas from delta8 and the length of the mask if they take too much memory
char delta8[] = {1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1};
char delta16[] = {3, 4, 3, 5, 3, 4, 3, 6, 3, 4, 3, 5, 3, 4, 3};
char delta32[] = {7, 8, 7, 9, 7, 8, 7};
char delta64[] = {15, 16, 15};
char delta128[] = {31};

/* Trivialimplementierung, die Speicher nicht re-cycle-t
   und auf das Alignment keine Ruecksicht nimmt
*/

void init_my_alloc() {
}

void* my_alloc(size_t size) {
   char* ret;
   if (!data || offset + size > BLOCKSIZE) {
      offset = 0;
      data = get_block_from_system();
   }
   ret = data + offset;
   offset += size;
   return ret;
}

void my_free(void* ptr) {
}
