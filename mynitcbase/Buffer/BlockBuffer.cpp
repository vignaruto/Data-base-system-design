#include "BlockBuffer.h"
#include "bits/stdc++.h"

// the declarations for these functions can be found in "BlockBuffer.h"

BlockBuffer::BlockBuffer(int blockNum) {
  // initialise this.blockNum with the argument
  this->blockNum=blockNum;
}

BlockBuffer::BlockBuffer(char blockType) {
    int blockTypeInt;

    // 1. Map the char to the internal Integer constants
    if (blockType == 'R') {
        blockTypeInt = REC;
    } else if (blockType == 'I') {
        blockTypeInt = IND_INTERNAL;
    } else if (blockType == 'L') {
        blockTypeInt = IND_LEAF;
    } else {
        blockTypeInt = UNUSED_BLK;
    }

    // 2. Call the static method to get a free block
    // We use a local variable 'allocatedBlock' to avoid confusion with 'this->blockNum'
    int allocatedBlock = getFreeBlock(blockTypeInt);

    // 3. Assign to the object's member variable
    this->blockNum = allocatedBlock;

    // 4. Basic validation check
    if (allocatedBlock < 0 || allocatedBlock >= DISK_BLOCKS) {
        // The caller will see the error code in this->blockNum
        return;
    }
}

int BlockBuffer::getBlockNum(){

    //return corresponding block number.
  return this->blockNum;
}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

// load the block header into the argument pointer
// the declarations for these functions can be found in "BlockBuffer.h"

RecBuffer::RecBuffer() : BlockBuffer('R'){}
// call parent non-default constructor with 'R' denoting record block.


/*
Used to get the header of the block into the location pointed to by `head`
NOTE: this function expects the caller to allocate memory for `head`
*/
int BlockBuffer::getHeader(struct HeadInfo *head) {

  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
  }

  // ... (the rest of the logic is as in stage 2)

  memcpy(&head->numSlots, bufferPtr + 24, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->lblock, bufferPtr + 8, 4);
  // ... (the rest of the logic is as in stage 2)
  return SUCCESS;
}

/*
Used to get the record at slot `slotNum` into the array `rec`
NOTE: this function expects the caller to allocate memory for `rec`
*/
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  // ...
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }
  // ... (the rest of the logic is as in stage 2
  HeadInfo head;
  getHeader(&head);

  int attrcount=head.numAttrs;
  int slotcount=head.numSlots;

  int slotmapsize=slotcount;
  int recordsize= attrcount*ATTR_SIZE;
  unsigned char *slotpointer=bufferPtr+HEADER_SIZE+slotmapsize+(recordsize*slotNum);

  memcpy(rec,slotpointer,recordsize);

  return SUCCESS;
}

/* NOTE: This function will NOT check if the block has been initialised as a
   record or an index block. It will copy whatever content is there in that
   disk block to the buffer.
   Also ensure that all the methods accessing and updating the block's data
   should call the loadBlockAndGetBufferPtr() function before the access or
   update is done. This is because the block might not be present in the
   buffer due to LRU buffer replacement. So, it will need to be bought back
   to the buffer before any operations can be done.
 */
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char ** buffPtr) {
    /* check whether the block is already present in the buffer
       using StaticBuffer.getBufferNum() */
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    // if present (!=E_BLOCKNOTINBUFFER),
        // set the timestamp of the corresponding buffer to 0 and increment the
        // timestamps of all other occupied buffers in BufferMetaInfo.
    if(bufferNum!=E_BLOCKNOTINBUFFER){
      for(int i=0;i<BUFFER_CAPACITY;i++){
        if(!StaticBuffer::metainfo[i].free){
          StaticBuffer::metainfo[i].timeStamp++;
        }
      }
      StaticBuffer::metainfo[bufferNum].timeStamp=0;
    }

    // else
        // get a free buffer using StaticBuffer.getFreeBuffer()

        // if the call returns E_OUTOFBOUND, return E_OUTOFBOUND here as
        // the blockNum is invalid

        // Read the block into the free buffer using readBlock()
    else{
      bufferNum=StaticBuffer::getFreeBuffer(this->blockNum);

      if(bufferNum==E_OUTOFBOUND){
        return E_OUTOFBOUND;
      }
      Disk::readBlock(StaticBuffer::blocks[bufferNum],this->blockNum);
    }

    // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
    *buffPtr=StaticBuffer::blocks[bufferNum];

    // return SUCCESS;
    return SUCCESS;
}

/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  // get the header of the block using getHeader() function
  getHeader(&head);

  int slotCount = head.numSlots/* number of slots in block from header */;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
  memcpy(slotMap,slotMapInBuffer,slotCount);

  return SUCCESS;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {
    
    StaticBuffer::cmpattrs++;
    if (attrType == STRING) {
        return strcmp(attr1.sVal, attr2.sVal);
    } else {
        double diff = attr1.nVal - attr2.nVal;
        if (diff > 0) return 1;
        if (diff < 0) return -1;
        return 0;
    }
}


int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) return ret;

    HeadInfo head;
    this->getHeader(&head);

    int numAttr = head.numAttrs;
    int numSlots = head.numSlots;

    if (slotNum >= numSlots || slotNum < 0) {
        return E_OUTOFBOUND;
    }

    /* CALCULATION:
       1. Skip Header: HEADER_SIZE
       2. Skip Slot Map: numSlots bytes (each slot is 1 byte)
       3. Skip previous records: slotNum * (number of attributes * attribute size)
    */
    int recordSize = ATTR_SIZE * numAttr;
    int offset = HEADER_SIZE + numSlots + (slotNum * recordSize);

    memcpy(bufferPtr + offset, rec, recordSize);

    // Update dirty bit so Buffer Manager knows to write this to disk
    StaticBuffer::setDirtyBit(this->blockNum);

    return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head){

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block using
    // loadBlockAndGetBufferPtr(&bufferPtr).
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if(ret!=SUCCESS){
      return ret;
    }

    // cast bufferPtr to type HeadInfo*
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    // copy the fields of the HeadInfo pointed to by head (except reserved) to
    // the header of the block (pointed to by bufferHeader)
    //(hint: bufferHeader->numSlots = head->numSlots )
    bufferHeader->lblock=head->lblock;
    bufferHeader->numAttrs=head->numAttrs;
    bufferHeader->numEntries=head->numEntries;
    bufferHeader->numSlots=head->numSlots;
    bufferHeader->pblock=head->pblock;
    bufferHeader->rblock=head->rblock;

    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed, return the error code
    
    // return SUCCESS;
    return StaticBuffer::setDirtyBit(this->blockNum);
}

int BlockBuffer::setBlockType(int blockType){

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if(ret!=SUCCESS){
      return ret;
    }

    // store the input block type in the first 4 bytes of the buffer.
    // (hint: cast bufferPtr to int32_t* and then assign it)
    // *((int32_t *)bufferPtr) = blockType;
    *((int32_t *)bufferPtr) = blockType;

    // update the StaticBuffer::blockAllocMap entry corresponding to the
    // object's block number to `blockType`.
    StaticBuffer::blockAllocMap[this->blockNum]=blockType;

    // update dirty bit by calling StaticBuffer::setDirtyBit()
    // if setDirtyBit() failed
        // return the returned value from the call

    // return SUCCESS
    return StaticBuffer::setDirtyBit(this->blockNum);
}

int BlockBuffer::getFreeBlock(int blockType){

    // iterate through the StaticBuffer::blockAllocMap and find the block number
    // of a free block in the disk.
    int freeblock=-1;
    for(int i=0;i<DISK_BLOCKS;i++){
      if(StaticBuffer::blockAllocMap[i]==UNUSED_BLK){
          freeblock=i;
          break;
      }
    }

    // if no block is free, return E_DISKFULL.
    if(freeblock==-1){
      return E_DISKFULL;
    }

    // set the object's blockNum to the block number of the free block.
    this->blockNum=freeblock;

    // find a free buffer using StaticBuffer::getFreeBuffer() .
    int freebuffer=StaticBuffer::getFreeBuffer(freeblock);

    // initialize the header of the block passing a struct HeadInfo with values
    // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
    // to the setHeader() function.
    HeadInfo head;
    this->getHeader(&head);
    head.pblock=-1;
    head.lblock=-1;
    head.rblock=-1;
    head.numEntries=0;
    head.numAttrs=0;
    head.numSlots=0;
    this->setHeader(&head);

    // update the block type of the block to the input block type using setBlockType().
    this->setBlockType(blockType);

    // return block number of the free block.
    return freeblock;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block using
       loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret=loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if(ret!=SUCCESS){
      return ret;
    }

    // get the header of the block using the getHeader() function
    HeadInfo head;
    this->getHeader(&head);

    int numSlots = head.numSlots/* the number of slots in the block */;

    // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
    // argument `slotMap` to the buffer replacing the existing slotmap.
    // Note that size of slotmap is `numSlots`
    unsigned char * slotMapinbuffer=bufferPtr+HEADER_SIZE;
    memcpy(slotMapinbuffer,slotMap,numSlots);

    // update dirty bit using StaticBuffer::setDirtyBit
    // if setDirtyBit failed, return the value returned by the call

    // return SUCCESS
    return StaticBuffer::setDirtyBit(this->blockNum);
}
