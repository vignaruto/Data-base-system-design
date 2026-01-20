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