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

    /* =======================================================
       STUDENTS : Relation cache entry (Hardcoded for testing)
       ======================================================= */

    RecBuffer relBuf(RELCAT_BLOCK);
    HeadInfo relHdr;
    relBuf.getHeader(&relHdr);
    int currRelBlock = RELCAT_BLOCK;

    while (currRelBlock != -1) {
        for (int i = 0; i < relHdr.numEntries; i++) {
            Attribute rec[RELCAT_NO_ATTRS];
            relBuf.getRecord(rec, i);

            if (strcmp(rec[RELCAT_REL_NAME_INDEX].sVal, "Students") == 0) {
                RelCacheEntry *node = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
                RelCacheTable::recordToRelCatEntry(rec, &node->relCatEntry);
                node->recId.block = currRelBlock;
                node->recId.slot  = i;
                RelCacheTable::relCache[2] = node;
                goto find_student_attrs; // Exit nested loop
            }
        }
        currRelBlock = relHdr.rblock;
        if (currRelBlock != -1) {
            relBuf = RecBuffer(currRelBlock);
            relBuf.getHeader(&relHdr);
        }
    }

    find_student_attrs:
    /* =======================================================
       STUDENTS : Attribute cache (Hardcoded for testing)
       ======================================================= */

    RecBuffer attrBuf(ATTRCAT_BLOCK);
    HeadInfo attrHdr;
    attrBuf.getHeader(&attrHdr);
    int currAttrBlock = ATTRCAT_BLOCK;
    AttrCacheEntry *studentHead = nullptr;

    while (currAttrBlock != -1) {
        for (int i = 0; i < attrHdr.numEntries; i++) {
            Attribute rec[ATTRCAT_NO_ATTRS];
            attrBuf.getRecord(rec, i);

            if (strcmp(rec[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0) {
                AttrCacheEntry *node = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
                AttrCacheTable::recordToAttrCatEntry(rec, &node->attrCatEntry);
                node->recId.block = currAttrBlock;
                node->recId.slot  = i;
                node->next = studentHead; // Build linked list
                studentHead = node;
            }
        }
        currAttrBlock = attrHdr.rblock;
        if (currAttrBlock != -1) {
            attrBuf = RecBuffer(currAttrBlock);
            attrBuf.getHeader(&attrHdr);
        }
    }
    AttrCacheTable::attrCache[2] = studentHead;
}

/* ======================= DESTRUCTOR ======================= */

OpenRelTable::~OpenRelTable() {
    for (int i = 0; i < MAX_OPEN; i++) {
        // Free Relation Cache entry
        if (RelCacheTable::relCache[i]) {
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }

        // Free Attribute Cache Linked List
        AttrCacheEntry *curr = AttrCacheTable::attrCache[i];
        while (curr) {
            AttrCacheEntry *next = curr->next;
            free(curr);
            curr = next;
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    if (strcmp(relName, RELCAT_RELNAME) == 0) return RELCAT_RELID;
    if (strcmp(relName, ATTRCAT_RELNAME) == 0) return ATTRCAT_RELID;
    if (strcmp(relName, "Students") == 0) return 2;

    return E_RELNOTOPEN;
}