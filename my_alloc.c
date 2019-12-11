#include <stdlib.h>
#include <stdint.h> //for uint64_t
#include "my_alloc.h"
#include "my_system.h"

static size_t offset = 0;
static char* data = 0;

//the uint64_t which stores the bitmap for the given block of 256 byte is stored at the beginning of each block

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
char delta256[] = {0};
//length of the delta arrays, computed in init_my_alloc for better performance
unsigned char delta8_len, delta16_len, delta32_len, delta64_len, delta128_len, delta256_len;

//the bitmasks, initialized in init_my_alloc
unsigned int mask8, mask16, mask32, mask64, mask128, mask256;
//length of the ones in the bitmasks, used to get the right index of the blocks in the bitmap
unsigned char mask8_len, mask16_len, mask32_len, mask64_len, mask128_len, mask256_len;

//the offset measured in bytes from the pointer where the bitmap lies to the returned pointer if memory is allocated
unsigned char return_pointer_offset[] = {0, 8, 0, 16, 24, 16, 0, 32, 40, 32, 48, 56, 48, 32, 0, 64, 72, 64, 80, 88, 80,
                                         64, 96, 104, 96, 112, 120, 112, 96, 64, 0, 128, 136, 128, 144, 152, 144, 128,
                                         160, 168, 160, 176, 184, 176, 160, 128, 192, 200, 192, 208, 216, 208, 192, 224,
                                         232, 224, 240, 248, 240, 224, 192, 128, 0};

//returns the index of the first free position where the given mask fits, -1 is mask never fits
signed char getIndexFromBitmap(uint64_t bitmap, unsigned int mask, unsigned char mask_len, char delta[], unsigned char deltaLen);

/* Trivialimplementierung, die Speicher nicht re-cycle-t
   und auf das Alignment keine Ruecksicht nimmt
*/

void init_my_alloc() {
    //initialize the delta array lengths, could be calculated if static memory is not enough
    delta8_len = sizeof(delta8) / sizeof(delta8[0]);
    delta16_len = sizeof(delta16) / sizeof(delta16[0]);
    delta32_len = sizeof(delta32) / sizeof(delta32[0]);
    delta64_len = sizeof(delta64) / sizeof(delta64[0]);
    delta128_len = sizeof(delta128) / sizeof(delta128[0]);
    delta256_len = sizeof(delta256) / sizeof(delta256[0]);

    //the length of the masks are calculated to be blocksize/4 + 1
    mask8_len = 8/4 + 1;
    mask16_len = 16/4 + 1;
    mask32_len = 32/4 + 1;
    mask64_len = 64/4 + 1;
    mask128_len = 128/4 + 1;
    mask256_len = 256/4 + 1;
    //the masks are set to size/4 + 1 ones on the LSB
    mask8 = (1 << mask8_len) - 1;
    mask16 = (1 << mask16_len) - 1;
    mask32 = (1 << mask32_len) - 1;
    mask64 = (1 << mask64_len) - 1;
    mask128 = (1 << mask128_len) - 1;
    mask256 = (1 << mask256_len) - 1;
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

//returns the index of the value of the block in the bitmap, e.g. 0 for the first block of size 8,
//1 for the second block of size 8, 2 for the first block of size 16 and so on...
signed char getIndexOfBlockInBitmap(uint64_t bitmap, int mask, unsigned char mask_len, char delta[], char deltaLen) {
    signed char delta_sum = 0; //keeps track over the whole shift width
    int index = 0;
    //bitmap & mask is only 0 if the block is empty, i.e. bitmap is filled with 0 in this area so no memory was allocated there
    for (; bitmap & mask && (index < deltaLen); index++) {
        mask <<= delta[index];
        delta_sum += delta[index];
    }
    if (index >= deltaLen) return -1;
    return delta_sum + mask_len - 1;
}