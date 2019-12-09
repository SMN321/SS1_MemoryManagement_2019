#include <stdlib.h>
#include <stdint.h> //for uint64_t
#include "my_alloc.h"
#include "my_system.h"

static size_t offset = 0;
static char* data = 0;

//arrays which hold the distance between two consecutive mask placements
//do something like (mask <<= delta[i]) and reuse mask, char[] to safe memory
//the initial place of the mask is with zero shift and has to be used before the delta-array is used, equivalently
//would be to add a 0 at delta[0] -- which was done so
//maybe calculate the deltas from delta8 and the length of the mask if they take too much memory
char delta8[] = {0, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1, 5, 1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1};
char delta16[] = {0, 3, 4, 3, 5, 3, 4, 3, 6, 3, 4, 3, 5, 3, 4, 3};
char delta32[] = {0, 7, 8, 7, 9, 7, 8, 7};
char delta64[] = {0, 15, 16, 15};
char delta128[] = {0, 31};
//length of the delta arrays, computed in init_my_alloc for better performance
char delta8_len, delta16_len, delta32_len, delta64_len, delta128_len;

//the bitmasks, initialized in init_my_alloc
int mask8, mask16, mask32, mask64, mask128;


//returns the index of the first free position where the given mask fits, -1 is mask never fits
signed char getIndexFromBitmap(uint64_t bitmap, int mask, char delta[], char deltaLen);

/* Trivialimplementierung, die Speicher nicht re-cycle-t
   und auf das Alignment keine Ruecksicht nimmt
*/

void init_my_alloc() {
    //initialize the delta array lengths
    delta8_len = sizeof(delta8) / sizeof(delta8[0]);
    delta16_len = sizeof(delta16) / sizeof(delta16[0]);
    delta32_len = sizeof(delta32) / sizeof(delta32[0]);
    delta64_len = sizeof(delta64) / sizeof(delta64[0]);
    delta128_len = sizeof(delta128) / sizeof(delta128[0]);

    //the masks are set to size/4 + 1 ones on the LSB
    mask8 = (1 << (8/4 + 1)) - 1;
    mask16 = (1 << (16/4 + 1)) - 1;
    mask32 = (1 << (32/4 + 1)) - 1;
    mask64 = (1 << (64/4 + 1)) - 1;
    mask128 = (1 << (128/4 + 1)) - 1;
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

signed char getIndexFromBitmap(uint64_t bitmap, int mask, char delta[], char deltaLen){
    signed char index = 0;
    for (; (bitmap & mask == 0) && (index < deltaLen); index++) {
        mask <<= delta[index];
    }
    if (index >= deltaLen) return -1;
    return index;
}