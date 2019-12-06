#include <stdlib.h>
#include "my_alloc.h"
#include "my_system.h"

static size_t offset = 0;
static char* data = 0;

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
