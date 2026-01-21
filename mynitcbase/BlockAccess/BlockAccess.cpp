#include "BlockAccess.h"

#include <bits/stdc++.h>


RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);

    // let block and slot denote the record id of the record being currently checked
    int block,slot;

    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)

        // block = first record block of the relation
        // slot = 0
        RelCatEntry x;
        RelCacheTable::getRelCatEntry(relId,&x);
        block=x.firstBlk;
        slot=0;
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)

        // block = search index's block
        // slot = search index's slot + 1
        block=prevRecId.block;
        slot=prevRecId.slot+1;
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */

        // get the record with id (block, slot) using RecBuffer::getRecord()
        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function
        RecBuffer buffer(block);
        HeadInfo head;
        buffer.getHeader(&head);

        if(slot>=head.numSlots)// If slot >= the number of slots per block(i.e. no more slots in this block)
        {
            // update block = right block of block
            // update slot = 0
            block=head.rblock;
            slot=0;
            if (block < 0) break;  // Stop if invalid block
            continue;  // continue to the beginning of this while loop
        }
        unsigned char slotmap[head.numSlots];
        buffer.getSlotMap(slotmap);

        if(slot>=head.numSlots || slotmap[slot]==SLOT_UNOCCUPIED)// if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        {
            // increment slot and continue to the next record slot
            slot++;
            continue;
        }
        int numattr=head.numAttrs;
        Attribute record[numattr];
        buffer.getRecord(record,slot);

        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        /* use the attribute offset to get the value of the attribute from
           current record */
        AttrCatEntry y;
        if(AttrCacheTable::getAttrCatEntry(relId,attrName,&y)!=SUCCESS){
            return RecId{-1,-1};
        }

        int offset=y.offset;
        if(offset<0 || offset>=numattr){
            return RecId{-1,-1};
        }
        Attribute recordattrvalue=record[offset];

        int cmpVal;  // will store the difference between the attributes
        // set cmpVal using compareAttrs()
        if(y.attrType==STRING){
            cmpVal=compareAttrs(recordattrvalue,attrVal,STRING);
        }
        else{
            cmpVal=compareAttrs(recordattrvalue,attrVal,NUMBER);
        }

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            prevRecId=RecId{block,slot};
            RelCacheTable::setSearchIndex(relId,&prevRecId);
            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;    // set newRelationName with newName

    // search the relation catalog for an entry with "RelName" = newRelationName
    strcpy(newRelationName.sVal,newName);

    RecId recid=linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,newRelationName,EQ);

    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    if(recid.block!=-1 && recid.slot!=-1){
        return E_RELEXIST;
    }


    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;    // set oldRelationName with oldName

    // search the relation catalog for an entry with "RelName" = oldRelationName
    strcpy(oldRelationName.sVal,oldName);

    RecId recid1=linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,oldRelationName,EQ);

    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    if(recid1.block==-1 && recid1.slot==-1){
        return E_RELNOTEXIST;
    }

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
   RecBuffer buffer(RELCAT_BLOCK);
   HeadInfo head;
   buffer.getHeader(&head);
   int numattr=head.numAttrs;
   Attribute record[numattr];
   buffer.getRecord(record,recid1.slot);
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    // set back the record value using RecBuffer.setRecord
    strcpy(record[RELCAT_REL_NAME_INDEX].sVal,newName);
    buffer.setRecord(record,recid1.slot);

    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numattr1=record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    //for i = 0 to numberOfAttributes :
    //    linearSearch on the attribute catalog for relName = oldRelationName
    //    get the record using RecBuffer.getRecord
    //
    //    update the relName field in the record to newName
    //    set back the record using RecBuffer.setRecord
    for(int i=0;i<numattr1;i++){
        RecId recid2=linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,oldRelationName,EQ);
        if(recid2.block!=-1 && recid2.slot!=-1){
            RecBuffer buffer(recid2.block);
            buffer.getRecord(record,recid2.slot);
            strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal,newName);
            buffer.setRecord(record,recid2.slot);
        }
        else{
            break;
        }
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal, relName);

    // Search for the relation with name relName in relation catalog
    RecId recid = linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    // If relation with name relName does not exist
    if (recid.block == -1 && recid.slot == -1) {
        return E_RELNOTEXIST;
    }

    /* reset the searchIndex of the attribute catalog */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    // NEW VARIABLE: To store the location of the attribute we want to rename
    RecId renameRecId{-1, -1}; 

    /* iterate over all Attribute Catalog Entry records */
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
        attrToRenameRecId = linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        // if there are no more attributes left to check
        if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1) {
            break;
        }

        RecBuffer buffer(attrToRenameRecId.block);
        buffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);

        // Check if this is the attribute we want to rename
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0) {
            // SAVE the ID in our separate variable, do not overwrite the loop iterator yet
            renameRecId = attrToRenameRecId;
        }

        // Check if the NEW name already exists (Duplicate check)
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0) {
            return E_ATTREXIST;
        }
    }

    // if renameRecId is still {-1, -1}, we never found the attribute
    if (renameRecId.block == -1 && renameRecId.slot == -1) {
        return E_ATTRNOTEXIST;
    }

    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    RecBuffer buff(renameRecId.block);
    buff.getRecord(attrCatEntryRecord, renameRecId.slot);
    
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    
    buff.setRecord(attrCatEntryRecord, renameRecId.slot);

    return SUCCESS;
}


int BlockAccess::insert(int relId, Attribute *record) {
    // 1. Get the relation catalog entry from relation cache
    RelCatEntry relcatentry;
    int ret = RelCacheTable::getRelCatEntry(relId, &relcatentry);

    if (ret != SUCCESS) {
        return ret;
    }

    int blockNum = relcatentry.firstBlk;
    RecId rec_id = {-1, -1};

    int numOfSlots = relcatentry.numSlotsPerBlk;
    int numOfAttributes = relcatentry.numAttrs;
    int prevBlockNum = -1;

    /* Traversing the linked list of existing record blocks to find a free slot
    */
    while (blockNum != -1) {
        RecBuffer buffer(blockNum);
        HeadInfo head;
        buffer.getHeader(&head);

        unsigned char slotmap[numOfSlots];
        buffer.getSlotMap(slotmap);

        int freeslot = -1;
        for (int i = 0; i < numOfSlots; i++) {
            if (slotmap[i] == SLOT_UNOCCUPIED) {
                freeslot = i;
                break;
            }
        }

        if (freeslot != -1) {
            rec_id.block = blockNum;
            rec_id.slot = freeslot;
            break;
        }

        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }

    // 2. If no free slot is found, allocate a new block
    if (rec_id.block == -1 && rec_id.slot == -1) {
        if (relId == RELCAT_RELID) {
            return E_MAXRELATIONS;
        }

        // Use the default constructor to allocate a new block
        RecBuffer newblock; 
        int newblocknum = newblock.getBlockNum();
        
        if (newblocknum == E_DISKFULL) {
            return E_DISKFULL;
        }

        rec_id.block = newblocknum;
        rec_id.slot = 0;

        // Set the header for the new block
        HeadInfo header;
        newblock.getHeader(&header);
        header.lblock = prevBlockNum;
        header.rblock = -1;
        header.numEntries = 0;
        header.numSlots = numOfSlots;
        header.numAttrs = numOfAttributes;
        newblock.setHeader(&header);

        // Initialize slot map as empty
        unsigned char newslotmap[numOfSlots];
        for (int i = 0; i < numOfSlots; i++) {
            newslotmap[i] = SLOT_UNOCCUPIED;
        }
        newblock.setSlotMap(newslotmap);

        // Link the new block to the previous block
        if (prevBlockNum != -1) {
            RecBuffer prevblock(prevBlockNum);
            HeadInfo prevhead;
            prevblock.getHeader(&prevhead);
            prevhead.rblock = rec_id.block;
            prevblock.setHeader(&prevhead);
        } else {
            // This is the very first block of the relation
            relcatentry.firstBlk = rec_id.block;
        }

        // Always update the last block pointer in the catalog entry
        relcatentry.lastBlk = rec_id.block;
        // (Note: RelCacheTable::setRelCatEntry is called at the end)
    }

    // 3. Insert the record into the identified slot
    RecBuffer finalbuffer(rec_id.block);
    finalbuffer.setRecord(record, rec_id.slot);

    // Update slot map to occupied
    unsigned char finalslotmap[numOfSlots];
    finalbuffer.getSlotMap(finalslotmap);
    finalslotmap[rec_id.slot] = SLOT_OCCUPIED;
    finalbuffer.setSlotMap(finalslotmap);

    // Increment block header entry count
    HeadInfo finalheader;
    finalbuffer.getHeader(&finalheader);
    finalheader.numEntries++;
    finalbuffer.setHeader(&finalheader);

    // 4. Update the Relation Cache with new record count and block pointers
    relcatentry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relcatentry);

    return SUCCESS;
}