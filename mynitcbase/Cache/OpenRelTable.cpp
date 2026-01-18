#include "OpenRelTable.h"
#include <bits/stdc++.h>
using namespace std;

/* ======================= CONSTRUCTOR ======================= */

OpenRelTable::OpenRelTable() {

    /* ---------- Initialize caches ---------- */
    for (int i = 0; i < MAX_OPEN; i++) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

    /************ Setting up Relation Cache entries ************/
// (we need to populate relation cache with entries for the relation catalog
//  and attribute catalog.)

/**** setting up Relation Catalog relation in the Relation Cache Table****/
RecBuffer relCatBlock(RELCAT_BLOCK);
Attribute relCatRecord[RELCAT_NO_ATTRS];
relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

struct RelCacheEntry relCacheEntry;
RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
relCacheEntry.recId.block = RELCAT_BLOCK;
relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

// allocate this on the heap because we want it to persist outside this function
RelCacheTable::relCache[RELCAT_RELID] =
    (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
*(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

/**** setting up Attribute Catalog relation in the Relation Cache Table ****/
// set up the relation cache entry for the attribute catalog similarly
// from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
// set the value at RelCacheTable::relCache[ATTRCAT_RELID]

RecBuffer relCatBlock1(RELCAT_BLOCK);
Attribute relCatRecord1[RELCAT_NO_ATTRS];
relCatBlock1.getRecord(relCatRecord1, RELCAT_SLOTNUM_FOR_ATTRCAT);

struct RelCacheEntry relCacheEntry1;
RelCacheTable::recordToRelCatEntry(relCatRecord1,
                                  &relCacheEntry1.relCatEntry);
relCacheEntry1.recId.block = RELCAT_BLOCK;
relCacheEntry1.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

// allocate this on the heap because we want it to persist outside this function
RelCacheTable::relCache[ATTRCAT_RELID] =
    (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
*(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry1;

/************ Setting up Attribute cache entries ************/
// (we need to populate attribute cache with entries for the relation catalog
//  and attribute catalog.)

/**** setting up Relation Catalog relation in the Attribute Cache Table ****/
RecBuffer attrCatBlock(ATTRCAT_BLOCK);

// iterate through all the attributes of the relation catalog and create a linked
// list of AttrCacheEntry (slots 0 to 5)
// for each of the entries, set
//    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
//    attrCacheEntry.recId.slot = i   (0 to 5)
//    and attrCacheEntry.next appropriately
// NOTE: allocate each entry dynamically using malloc

// set the next field in the last entry to nullptr
for (int i = 0; i < RELCAT_NO_ATTRS; i++) {

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    struct AttrCacheEntry attrCacheEntry;

    attrCatBlock.getRecord(attrCatRecord, i);

    AttrCacheTable::recordToAttrCatEntry(
        attrCatRecord,
        &attrCacheEntry.attrCatEntry
    );

    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
    attrCacheEntry.recId.slot  = i;
    attrCacheEntry.next = nullptr;

    AttrCacheTable::attrCache[i] =
        (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

    *(AttrCacheTable::attrCache[i]) = attrCacheEntry;

    AttrCacheTable::attrCache[i]->next =
        AttrCacheTable::attrCache[RELCAT_RELID];

    AttrCacheTable::attrCache[RELCAT_RELID] =
        AttrCacheTable::attrCache[i];
}

/**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
// set up the attributes of the attribute cache similarly.
// read slots 6-11 from attrCatBlock and initialise recId appropriately
// set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]

for (int i = RELCAT_NO_ATTRS;
     i < RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS;
     i++) {

    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    struct AttrCacheEntry attrCacheEntry;

    attrCatBlock.getRecord(attrCatRecord, i);

    AttrCacheTable::recordToAttrCatEntry(
        attrCatRecord,
        &attrCacheEntry.attrCatEntry
    );

    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
    attrCacheEntry.recId.slot  = i;
    attrCacheEntry.next = nullptr;

    AttrCacheTable::attrCache[i] =
        (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

    *(AttrCacheTable::attrCache[i]) = attrCacheEntry;

    AttrCacheTable::attrCache[i]->next =
        AttrCacheTable::attrCache[ATTRCAT_RELID];

    AttrCacheTable::attrCache[ATTRCAT_RELID] =
        AttrCacheTable::attrCache[i];
}

/* =======================================================
       STUDENTS : Relation cache entry
       ======================================================= */

    RecBuffer relBuf(RELCAT_BLOCK);
    HeadInfo relHdr;
    relBuf.getHeader(&relHdr);

    int block = RELCAT_BLOCK;

    for (int i = 0; i < relHdr.numEntries; i++) {

        Attribute rec[RELCAT_NO_ATTRS];
        relBuf.getRecord(rec, i);

        if (strcmp(rec[RELCAT_REL_NAME_INDEX].sVal, "Students") == 0) {

            RelCacheEntry *node =
                (RelCacheEntry*)malloc(sizeof(RelCacheEntry));

            RelCacheTable::recordToRelCatEntry(rec, &node->relCatEntry);
            node->recId.block = block;
            node->recId.slot  = i;

            RelCacheTable::relCache[2] = node;
            break;
        }

        if (i == relHdr.numEntries - 1 && relHdr.rblock != -1) {
            block = relHdr.rblock;
            relBuf = RecBuffer(block);
            relBuf.getHeader(&relHdr);
            i = -1;
        }
    }

    /* =======================================================
       STUDENTS : Attribute cache
       ======================================================= */

    RecBuffer buf(ATTRCAT_BLOCK);
    HeadInfo hdr;
    buf.getHeader(&hdr);

    AttrCacheEntry *head = nullptr;
    AttrCacheEntry *tail = nullptr;
    block = ATTRCAT_BLOCK;

    for (int i = 0; i < hdr.numEntries; i++) {

        Attribute rec[ATTRCAT_NO_ATTRS];
        buf.getRecord(rec, i);

        if (strcmp(rec[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0) {

            AttrCacheEntry *node =
                (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

            AttrCacheTable::recordToAttrCatEntry(rec, &node->attrCatEntry);
            node->recId.block = block;
            node->recId.slot  = i;
            node->dirty = false;
            node->next = nullptr;

            if (!head)
                head = tail = node;
            else {
                tail->next = node;
                tail = node;
            }
        }

        if (i == hdr.numEntries - 1 && hdr.rblock != -1) {
            block = hdr.rblock;
            buf = RecBuffer(block);
            buf.getHeader(&hdr);
            i = -1;
        }
    }

    AttrCacheTable::attrCache[2] = head;

}

/* ======================= DESTRUCTOR ======================= */

OpenRelTable::~OpenRelTable() {

    for (int i = 0; i < MAX_OPEN; i++) {

        if (RelCacheTable::relCache[i]) {
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
}

/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

  // if relname is RELCAT_RELNAME, return RELCAT_RELID
  if(strcmp(relName,RELCAT_RELNAME)==0){
    return RELCAT_RELID;
  }
  // if relname is ATTRCAT_RELNAME, return ATTRCAT_RELID
  if(strcmp(relName,ATTRCAT_RELNAME)==0){
    return ATTRCAT_RELID;
  }

  if(strcmp(relName,"Students")==0){
    return 2;
  }

  return E_RELNOTOPEN;
}
