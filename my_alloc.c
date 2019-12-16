#include <stdlib.h>
#include "my_alloc.h"
#include "my_system.h"

#define DIFFSIZES (32)

typedef struct header {
    __uint8_t nextSize; //data or free after header: size from 0 to 255
    __uint8_t nextFree; //bool for data or free after header
    __uint8_t prevSize; //data or free before header: size from 0 to 255
    __uint8_t prevFree; //bool for data or free before header
    __uint8_t startOfBlock; //bool for is header at start of block
    __uint8_t endOfBlock; //bool for is header at end of block
    __uint8_t curLastInBlock; //bool for is header last header in block
    __uint8_t dummy; //sizeof(header)%8 has to be 0
} header;
size_t header_size = sizeof(header);

typedef struct freespace {
    struct freespace *nextfree;
    struct freespace *prevfree;
} freespace;
size_t freespace_size = sizeof(freespace);

int leftSpaceInBlock = BLOCKSIZE;

freespace *freelist[DIFFSIZES];

void linkFreeToList(freespace *free, int size) {
    //add at front
    freespace *list = freelist[size/8 - 1];
    free->nextfree = list;
    free->prevfree = NULL;
    if(free->nextfree) {
        free->nextfree->prevfree = free;
    }
    freelist[size/8 - 1] = free;
}

void deleteFreeFromList(freespace *wasfree, int size) {
    if(wasfree->nextfree) {
        //wasfree not end of list
        wasfree->nextfree->prevfree = wasfree->prevfree;
    } //else: wasfree end of list and therefore wasfree->nextfree is NULL
    if(wasfree->prevfree) {
        //wasfree not start of list
        wasfree->prevfree->nextfree = wasfree->nextfree;
    } else {
        //wasfree start of list
        freelist[size/8 - 1] = wasfree->nextfree;
    }
}

void addHeader(header *h, __uint16_t nextsize, __uint8_t nextFree, __uint16_t prevsize, __uint8_t prevFree,
               __uint8_t startOfBlock, __uint8_t endOfBlock, __uint8_t curLastInBlock) {
    h->nextSize = nextsize -1;
    h->nextFree = nextFree;
    h->prevSize = prevsize -1;
    h->prevFree = prevFree;
    h->startOfBlock = startOfBlock;
    h->endOfBlock = endOfBlock;
    h->curLastInBlock = curLastInBlock;
}

void addHeaderWithFreespace(header *h, __uint16_t prevsize, __uint8_t prevFree,
                            __uint8_t startOfBlock, __uint8_t endOfBlock, __uint8_t curLastInBlock,
                            int freeSize) {
    //add header with freespace
    addHeader(h, freeSize, 1, prevsize, prevFree, startOfBlock, endOfBlock, curLastInBlock);
    freespace *freeAfterHeader = (freespace *)((char *)h + header_size);
    linkFreeToList(freeAfterHeader, freeSize);

    //update header after freespace
    if(!h->endOfBlock) {
        header *headerAfterFree = (header *)((char *)freeAfterHeader + freeSize);
        headerAfterFree->prevFree = 1;
        headerAfterFree->prevSize = freeSize -1;
    }
}

void addHeaderWithOversizedFreespace(header *h, int freeSize, __uint8_t endOfBlock) {
    while(freeSize - 256 - (int)header_size >= (int)(header_size + freespace_size)) {
        //freespace can be split
        addHeaderWithFreespace(h, h->prevSize+1, h->prevFree,
                               h->startOfBlock, 0, 0, 256);
        freeSize -= 256 + header_size;
        h = (header *)((char *)h + header_size + 256);
        h->prevSize = 255;
        h->prevFree = 1;
    }

    if(freeSize - (int)header_size > 256) {
        //split freespace
        addHeaderWithFreespace(h, h->prevSize+1, h->prevFree,
                               h->startOfBlock, 0, 0, 16);
        freeSize -= 16 + header_size;
        h = (header *)((char *)h + header_size + 16);
        h->prevSize = 15;
        h->prevFree = 1;
    }

    addHeaderWithFreespace(h, h->prevSize+1, h->prevFree,
                           h->startOfBlock, endOfBlock, endOfBlock, freeSize - header_size);

}

void replaceFreeSpaceWithData(freespace *free, int dataSize) {
    header *headerBeforeFreespace = (header *)((char *)free - header_size);
    if(dataSize == 8) {
        dataSize = 16;
    }
    int freeSize = headerBeforeFreespace->nextSize +1;

    //delete freespace
    deleteFreeFromList(free, freeSize);

    if(freeSize - dataSize < (int)(header_size + freespace_size)) {
        //perfect fit or not enough space left for additional header and freespace
        dataSize = freeSize;

        //update header after data
        if(!headerBeforeFreespace->endOfBlock) {
            //there is a header after the inserted data
            header *headerAfterData = (header *)((char *)free + dataSize);
            headerAfterData->prevFree = 0;
            headerAfterData->prevSize = dataSize -1;
            if(headerBeforeFreespace->curLastInBlock) {
                //the header after the inserted data becomes the current last header in block
                headerBeforeFreespace->curLastInBlock = 0;
                leftSpaceInBlock -= header_size;
                int take = leftSpaceInBlock;
                if(take > 256) {
                    take = 256;
                }
                leftSpaceInBlock -= take;
                __uint8_t endOfBlock = 0;
                if(leftSpaceInBlock < (int)(header_size + freespace_size)) {
                    endOfBlock = 1;
                }
                addHeaderWithFreespace(headerAfterData, headerAfterData->prevSize+1, headerAfterData->prevFree, 0, endOfBlock, 1, take);
            }
        }
    } else {
        //enough space left for additional header and freespace
        header *headerAfterData = (header *)((char *)free + dataSize);
        int leftSize = freeSize - (dataSize + header_size);
        int take = leftSize;
        u_int8_t endOfBlock = headerBeforeFreespace->endOfBlock;
        if(headerBeforeFreespace->curLastInBlock && !headerBeforeFreespace->endOfBlock) {
            take += leftSpaceInBlock;
            if(take > 256) {
                take = 256;
            }
            if(leftSpaceInBlock - (take - leftSize) < (int)(header_size + freespace_size)) {
                endOfBlock = 1;
            }
        }
        leftSpaceInBlock -= take - leftSize;
        addHeaderWithFreespace(headerAfterData, dataSize, 0, 0, endOfBlock, headerBeforeFreespace->curLastInBlock, take);
        headerBeforeFreespace->curLastInBlock = 0;
        headerBeforeFreespace->endOfBlock = 0;
    }

    //update header before data
    headerBeforeFreespace->nextSize = dataSize -1;
    headerBeforeFreespace->nextFree = 0;

}

void newBlock() {
    //get block
    header *firstHeaderInBlock = get_block_from_system();
    leftSpaceInBlock = BLOCKSIZE;

    //add header with freespace
    int freeSize = DIFFSIZES*8;
    addHeaderWithFreespace(firstHeaderInBlock, 0, 0, 1, 0, 1, freeSize);
    leftSpaceInBlock -= header_size + freeSize;
}

void init_my_alloc() {
}

void* my_alloc(size_t size) {
    void *ret = NULL;

    //search for free space
    int i = size / 8 - 1;
    for (; i < DIFFSIZES && !ret; i++) {
        ret = freelist[i];
    }
    if (!ret) {
        //no free space found -> new block
        newBlock();
        ret = freelist[DIFFSIZES - 1];
    }

    //free space available
    replaceFreeSpaceWithData(ret, size);

    return ret;
}

void my_free(void* ptr) {
    header *headerOfPtr = (header *)((char *)ptr - header_size);
    addHeaderWithFreespace(headerOfPtr, headerOfPtr->prevSize+1, 0,
                           headerOfPtr->startOfBlock, headerOfPtr->endOfBlock, headerOfPtr->curLastInBlock, headerOfPtr->nextSize+1);
}
