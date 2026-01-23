#include "Schema.h"

#include <bits/stdc++.h>



int Schema::openRel(char relName[ATTR_SIZE])
{
    int ret = OpenRelTable::openRel(relName);

    // the OpenRelTable::openRel() function returns the rel-id if successful
    // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
    // error codes will be negative
    if (ret >= 0 && ret < MAX_OPEN)
    {
        return SUCCESS;
    }

    // otherwise it returns an error message
    return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
  if (strcmp(relName,RELCAT_RELNAME)==0 && strcmp(relName,ATTRCAT_RELNAME)==0/* relation is relation catalog or attribute catalog */) {
    return E_NOTPERMITTED;
  }

  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);

  if (relId<0 || relId>=MAX_OPEN/* relation is not open */) {
    return E_RELNOTOPEN;
  }

  return OpenRelTable::closeRel(relId);
}

int Schema::createRel(char relName[], int numOfAttributes, char attrNames[][ATTR_SIZE], int attrType[]) {

    // 1. Check if relation already exists
    Attribute relNameAsAttribute;
    strcpy(relNameAsAttribute.sVal, relName);

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    
    RecId targetReid = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAsAttribute, EQ);

    if (targetReid.block != -1 && targetReid.slot != -1) {
        return E_RELEXIST;
    }

    // 2. Check for duplicate attribute names
    // Note: The inner loop should start from i + 1 to avoid comparing an attribute with itself
    for (int i = 0; i < numOfAttributes; i++) {
        for (int j = i + 1; j < numOfAttributes; j++) {
            if (strcmp(attrNames[i], attrNames[j]) == 0) {
                return E_DUPLICATEATTR;
            }
        }
    }

    // 3. Prepare and Insert Relation Catalog Record
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, relName);
    relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal = numOfAttributes;
    relCatRecord[RELCAT_NO_RECORDS_INDEX].nVal = 0;
    relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal = -1;
    relCatRecord[RELCAT_LAST_BLOCK_INDEX].nVal = -1;
    
    // Calculate slots per block
    int slotsPerBlock = floor(2016 / (16.0 * numOfAttributes + 1));
    relCatRecord[RELCAT_NO_SLOTS_PER_BLOCK_INDEX].nVal = slotsPerBlock;

    int retVal = BlockAccess::insert(RELCAT_RELID, relCatRecord);
    if (retVal != SUCCESS) {
        return retVal;
    }

    // 4. Iterate and Insert Attribute Catalog Records
    for (int i = 0; i < numOfAttributes; i++) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        
        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relName);
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrNames[i]);
        attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal = attrType[i];
        attrCatRecord[ATTRCAT_PRIMARY_FLAG_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal = -1;
        attrCatRecord[ATTRCAT_OFFSET_INDEX].nVal = i;

        int ret = BlockAccess::insert(ATTRCAT_RELID, attrCatRecord);

        if (ret != SUCCESS) {
            // Rollback: delete the relation catalog entry if attribute insertion fails
            Schema::deleteRel(relName);
            return E_DISKFULL;
        }
    }

    return SUCCESS;
}

int Schema::deleteRel(char relName[ATTR_SIZE])
{
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //     return E_NOTPERMITTED
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)
    if(strcmp(relName,RELCAT_RELNAME)==0 || strcmp(relName,ATTRCAT_RELNAME)==0){
      return E_NOTPERMITTED;
    }

    // get the rel-id using appropriate method of OpenRelTable class by
    // passing relation name as argument
    int rel_id=OpenRelTable::getRelId(relName);

    // if relation is opened in open relation table, return E_RELOPEN
    if(rel_id !=E_RELNOTOPEN){
      return E_RELOPEN;
    }

    // Call BlockAccess::deleteRelation() with appropriate argument.
    return BlockAccess::deleteRelation(relName);

    // return the value returned by the above deleteRelation() call

    /* the only that should be returned from deleteRelation() is E_RELNOTEXIST.
       The deleteRelation call may return E_OUTOFBOUND from the call to
       loadBlockAndGetBufferPtr, but if your implementation so far has been
       correct, it should not reach that point. That error could only occur
       if the BlockBuffer was initialized with an invalid block number.
    */
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE])
{
    // if the oldRelName or newRelName is either Relation Catalog or Attribute Catalog,
        // return E_NOTPERMITTED
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)
      if(strcmp(oldRelName,(char *)RELCAT_RELNAME)==0 || 
         strcmp(oldRelName,(char *)ATTRCAT_RELNAME)==0 ||
         strcmp(newRelName,(char *)RELCAT_RELNAME)==0 ||
         strcmp(newRelName,(char *)ATTRCAT_RELNAME)==0){
          return E_NOTPERMITTED;
         }

    // if the relation is open
    //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
    //    return E_RELOPEN
    if(OpenRelTable::getRelId(oldRelName)!=E_RELNOTOPEN){
      return E_RELOPEN;
    }

    // retVal = BlockAccess::renameRelation(oldRelName, newRelName);
    // return retVal
    int retval=BlockAccess::renameRelation(oldRelName,newRelName);
    return retval;
}

int Schema::renameAttr(char relName[ATTR_SIZE], char oldAttrName[ATTR_SIZE], char newAttrName[ATTR_SIZE])
{
    // if the relName is either Relation Catalog or Attribute Catalog,
        // return E_NOTPERMITTED
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)
      if(strcmp(relName,(char *)RELCAT_RELNAME)==0 || strcmp(relName,(char *)ATTRCAT_RELNAME)==0){
        return E_NOTPERMITTED;
      }

    // if the relation is open
        //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
        //    return E_RELOPEN
      if(OpenRelTable::getRelId(relName)!=E_RELNOTOPEN){
        return E_RELOPEN;
      }

    // Call BlockAccess::renameAttribute with appropriate arguments.
    int retval=BlockAccess::renameAttribute(relName,oldAttrName,newAttrName);
    return retval;

    // return the value returned by the above renameAttribute() call
}

