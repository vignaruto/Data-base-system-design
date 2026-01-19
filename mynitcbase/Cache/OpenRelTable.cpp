#include "OpenRelTable.h"
#include <bits/stdc++.h>
using namespace std;


OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];


AttrCacheEntry *createLinkedList(int length)
{
    AttrCacheEntry *head = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
    AttrCacheEntry *tail = head;
    for (int i = 1; i < length; i++)
    {
        tail->next = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
        tail = tail->next;
    }
    tail->next = nullptr;
    return head;
}

void clearList(AttrCacheEntry* head) 
{
    for (AttrCacheEntry* it = head, *next; it != nullptr; it = next) 
    {
        next = it->next;
        free(it);
    }
}
/* ======================= CONSTRUCTOR ======================= */

OpenRelTable::OpenRelTable() {

    /* ---------- Initialize caches ---------- */
    for (int i = 0; i < MAX_OPEN; i++) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        OpenRelTable::tableMetaInfo[i].free=true;
    }

    /************ Setting up Relation Cache entries ************/

    // 1. Setup Relation Catalog Entry
    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

    RelCacheTable::relCache[RELCAT_RELID] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(RelCacheTable::relCache[RELCAT_RELID]->relCatEntry));
    RelCacheTable::relCache[RELCAT_RELID]->recId.block = RELCAT_BLOCK;
    RelCacheTable::relCache[RELCAT_RELID]->recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

    // 2. Setup Attribute Catalog Entry
    Attribute attrCatRecordInRelCat[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(attrCatRecordInRelCat, RELCAT_SLOTNUM_FOR_ATTRCAT);

    RelCacheTable::relCache[ATTRCAT_RELID] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    RelCacheTable::recordToRelCatEntry(attrCatRecordInRelCat, &(RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry));
    RelCacheTable::relCache[ATTRCAT_RELID]->recId.block = RELCAT_BLOCK;
    RelCacheTable::relCache[ATTRCAT_RELID]->recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;


    /************ Setting up Attribute cache entries ************/

    RecBuffer attrCatBlock(ATTRCAT_BLOCK);

    // 3. Setup Relation Catalog Attributes (Linked List)
    AttrCacheEntry *head = nullptr;
    for (int i = 0; i < RELCAT_NO_ATTRS; i++) {
        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrRecord, i);

        AttrCacheEntry *newNode = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrRecord, &newNode->attrCatEntry);
        newNode->recId.block = ATTRCAT_BLOCK;
        newNode->recId.slot = i;
        newNode->next = head; // Building the list backwards for simplicity
        head = newNode;
    }
    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    // 4. Setup Attribute Catalog Attributes (Linked List)
    head = nullptr;
    for (int i = RELCAT_NO_ATTRS; i < RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS; i++) {
        Attribute attrRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrRecord, i);

        AttrCacheEntry *newNode = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        AttrCacheTable::recordToAttrCatEntry(attrRecord, &newNode->attrCatEntry);
        newNode->recId.block = ATTRCAT_BLOCK;
        newNode->recId.slot = i;
        newNode->next = head;
        head = newNode;
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    for(int i=0;i<2;i++){
        if(i==0){
            tableMetaInfo[i].free=false;
            strcpy(tableMetaInfo[i].relName,RELCAT_RELNAME);
        }
        else if(i==1){
            tableMetaInfo[i].free=false;
            strcpy(tableMetaInfo[i].relName,ATTRCAT_RELNAME);
        }
    }
}

/* ======================= DESTRUCTOR ======================= */

OpenRelTable::~OpenRelTable() {

    // close all open relations (from rel-id = 2 onwards. Why?)
    for (int i = 2; i < MAX_OPEN; ++i) {
        if (!tableMetaInfo[i].free) {
        OpenRelTable::closeRel(i); // we will implement this function later
        }
    }

    for (int i = 0; i < MAX_OPEN; i++) {
        free(RelCacheTable::relCache[i]);
        clearList(AttrCacheTable::attrCache[i]);

        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    /* traverse through the tableMetaInfo array,
    find the entry in the Open Relation Table corresponding to relName.*/
    for(int i=0;i<MAX_OPEN;i++){
        if(tableMetaInfo[i].free==false && (strcmp(tableMetaInfo[i].relName,relName)==0)){
            return i;
        }
    }

  // if found return the relation id, else indicate that the relation do not
  // have an entry in the Open Relation Table.

    return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE])
{
    int ret=OpenRelTable::getRelId(relName);
    if(ret!=E_RELNOTOPEN/* the relation `relName` already has an entry in the Open Relation Table */){
    // (checked using OpenRelTable::getRelId())

    // return that relation id;
    return ret;
  }

  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
    int relId=OpenRelTable::getFreeOpenRelTableEntry();

  if (relId==E_CACHEFULL/* free slot not available */){
    return E_CACHEFULL;
  }


  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/

    Attribute attrrelname;
    memcpy(attrrelname.sVal,relName,ATTR_SIZE);
    RelCacheTable::resetSearchIndex(RELCAT_RELID);


  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RecId relcatRecId=BlockAccess::linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,attrrelname,EQ);

  if (/* relcatRecId == {-1, -1} */relcatRecId.block==-1 && relcatRecId.slot==-1) {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      update the recId field of this Relation Cache entry to relcatRecId.
      use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */
    RecBuffer buffer(relcatRecId.block);
    RelCatEntry relcatentry;
    Attribute record[RELCAT_NO_ATTRS];
    buffer.getRecord(record,relcatRecId.slot);
    RelCacheTable::recordToRelCatEntry(record,&relcatentry);
    RelCacheTable::relCache[relId]=(RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    RelCacheTable::relCache[relId]->recId=relcatRecId;
    RelCacheTable::relCache[relId]->relCatEntry=relcatentry;

  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  int numattr=relcatentry.numAttrs;
  AttrCacheEntry* listHead=createLinkedList(numattr);
  AttrCacheEntry*node=listHead;
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  /*iterate over all the entries in the Attribute Catalog corresponding to each
  attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
  care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
  corresponding to Attribute Catalog before the first call to linearSearch().*/
  while(true)
  {
      /* let attrcatRecId store a valid record id an entry of the relation, relName,
      in the Attribute Catalog.*/
      RecId attrcatRecId=BlockAccess::linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,attrrelname,EQ);

      /* read the record entry corresponding to attrcatRecId and create an
      Attribute Cache entry on it using RecBuffer::getRecord() and
      AttrCacheTable::recordToAttrCatEntry().
      update the recId field of this Attribute Cache entry to attrcatRecId.
      add the Attribute Cache entry to the linked list of listHead .*/
      // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
      if(attrcatRecId.block!=-1 && attrcatRecId.slot!=-1){
        RecBuffer buff(attrcatRecId.block);
        Attribute rec[ATTRCAT_NO_ATTRS];
        AttrCatEntry attrcatentry;
        buff.getRecord(rec,attrcatRecId.slot);
        AttrCacheTable::recordToAttrCatEntry(rec,&attrcatentry);
        node->recId=attrcatRecId;
        node->attrCatEntry=attrcatentry;
        node=node->next;
      }
      else{
        break;
      }
  }

  // set the relIdth entry of the AttrCacheTable to listHead.
  AttrCacheTable::attrCache[relId]=listHead;

  /****** Setting up metadata in the Open Relation Table for the relation******/

  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
  OpenRelTable::tableMetaInfo[relId].free=false;
  memcpy(tableMetaInfo[relId].relName,relcatentry.relName,ATTR_SIZE);

  return relId;
}

int OpenRelTable::closeRel(int relId)
{
    if (relId==RELCAT_RELID || relId==ATTRCAT_RELID/* rel-id corresponds to relation catalog or attribute catalog*/) {
    return E_NOTPERMITTED;
  }

  if (/* 0 <= relId < MAX_OPEN */relId<0 || relId>=MAX_OPEN) {
    return E_OUTOFBOUND;
  }

  if (tableMetaInfo[relId].free==true/* rel-id corresponds to a free slot*/) {
    return E_RELNOTOPEN;
  }

  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function
  free(RelCacheTable::relCache[relId]);
  clearList(AttrCacheTable::attrCache[relId]);
  RelCacheTable::relCache[relId]=nullptr;
  AttrCacheTable::attrCache[relId]=nullptr;

  // update `tableMetaInfo` to set `relId` as a free slot
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
  tableMetaInfo[relId].free=true;


  return SUCCESS;
}

int OpenRelTable::getFreeOpenRelTableEntry() {

  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/
    for(int i=0;i<MAX_OPEN;i++){
        if(tableMetaInfo[i].free==true){
            return i;
        }
    }

  // if found return the relation id, else return E_CACHEFULL.
  return E_CACHEFULL;
}