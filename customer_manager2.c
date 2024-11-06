/*
 * Program: customer_manager2.c
 * Student Name: Naol Samuel Erega
 * Student ID: 20210889
 * Assignment: 3
 *
 * Description:
 * ------------
 * This program implements a customer management system using a hash table to store
 * customer data in two tables: one for IDs (`iTable`) and another for names (`nTable`).
 * Each customer entry includes an ID, name, and purchase amount. The database 
 * dynamically resizes as necessary to accommodate a growing number of customers.
 * 
 * Functionality:
 * --------------
 * 1. **Database Creation and Destruction**:
 *    - `CreateCustomerDB`: Allocates and initializes a database with hash tables.
 *    - `DestroyCustomerDB`: Cleans up and frees memory for all database structures.
 * 
 * 2. **Registration and Expansion**:
 *    - `RegisterCustomer`: Adds a new customer entry, expanding tables if needed.
 *      - Uses two hash tables to store customers by both `id` and `name`.
 *      - Automatically resizes tables when the load factor exceeds 75%.
 * 
 * 3. **Unregistration**:
 *    - `UnregisterCustomerByID`: Removes a customer by ID, ensuring memory cleanup.
 *    - `UnregisterCustomerByName`: Removes a customer by name.
 * 
 * 4. **Retrieval and Calculation**:
 *    - `GetPurchaseByID` and `GetPurchaseByName`: Retrieve the purchase amount for a 
 *       customer based on either ID or name.
 *    - `GetSumCustomerPurchase`: Calculates the sum of all customer purchases, using a 
 *       function pointer (`FUNCPTR_T`) to customize the calculation.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "customer_manager.h"
#define MAX_BUCKET_COUNT 1048576
#define LOAD_FACTOR 0.75
#define HASH_MULTIPLIER 65599

int iBucketCount=1024;
/*--------------------------------------------------------------------*/
static int hash_function(const char *pcKey, int iBucketCount)

/* Return a hash code for pcKey that is between 0 and iBucketCount-1,
  inclusive. Adapted from the EE209 lecture notes. */
{
  int i;
  unsigned int uiHash = 0U;
  for (i = 0; pcKey[i] != '\0'; i++)
    uiHash = uiHash * (unsigned int)HASH_MULTIPLIER
          + (unsigned int)pcKey[i];
  return (int)(uiHash % (unsigned int)iBucketCount);
}
/*--------------------------------------------------------------------*/
struct UserInfo {
  char *name;                // customer name
  char *id;                  // customer id
  int purchase;              // purchase amount (> 0)
  struct UserInfo* iNext;  // Next item in id linked list
  struct UserInfo* nNext;  // Next item in name linked list
};

struct DB {
  struct UserInfo** iTable;   /* Pointer to the ID HashTable */
  struct UserInfo** nTable;/* Pointer to the Name HashTable */
  int iBucketCount;   
  int numItems; /*For expansion. Assumption: Both hashtables expan at the same time */        
};
/*--------------------------------------------------------------------*/
DB_T
CreateCustomerDB(void)
{ 
  DB_T d;
  d = (DB_T) calloc(1, sizeof(struct DB));
  if (d == NULL) { /* Allocation failed */
    fprintf(stderr, "Error: Can't allocate a memory for DB_T\n");
    return NULL;
  }
  d->iBucketCount = iBucketCount; /* Start with 1024 */

  /* Memory allocation for the id table */
  d->iTable = (struct UserInfo** )calloc(d->iBucketCount, sizeof(struct UserInfo*));
  if (d->iTable == NULL) {
      fprintf(stderr, "Error: Can't allocate a memory for id table of size %d\n",
	    d->iBucketCount);   
    free(d);
    return NULL;
  }

  /* Memory allocation for the name table */
  d->nTable = (struct UserInfo** )calloc(d->iBucketCount, sizeof(struct UserInfo*));
  if (d->nTable == NULL) {
    fprintf(stderr, "Error: Can't allocate a memory for name table of size %d\n",
	    d->iBucketCount);
    free(d->iTable);  
    free(d);
    return NULL;
  }
  d->numItems=0; /* Number of already stored item initializtion */
  return d;
}
/*--------------------------------------------------------------------*/
void DestroyCustomerDB(DB_T d) {
  int processedItems = 0; /* Tracks the number of valid items processed */
  struct UserInfo* curr;
  struct UserInfo* next;

  if (d == NULL) return; /* No need to destroy an empty database */

  /* Iterate over each bucket until all filled items are processed */
  for (int i = 0; i < d->iBucketCount && processedItems < d->numItems; i++) {
    curr = d->iTable[i];
    while (curr) { /* Iterate the linked list for the current bucket */
      /* Free the current user's information */
      free(curr->id);
      free(curr->name);

      /* Move to the next user before releasing the current memory */
      next = curr->iNext;

      /* Release the current user's memory */
      free(curr);
      curr = next; /* Update to the next user */
      
      processedItems++; /* Increment the count of processed items */

      /* Stop if we have processed all assigned items */
      if (processedItems >= d->numItems) break;
    }
  }

  /* Release both ID and name tables */
  free(d->iTable);
  free(d->nTable);
  free(d);
}

/*--------------------------------------------------------------------*/
int 
RegisterCustomer(DB_T d, const char *id, const char *name, const int purchase){

  struct UserInfo *curr,*next, *newUsr;  /* For traversing linkedlist*/
  struct UserInfo **iTableTempo, **nTableTempo; /* Temporary tables during expansion */
  int iKey, nKey; /* Hash keys */

  if (d == NULL || id == NULL || name == NULL || purchase <= 0) return -1; 

  /* Checking whether the item already exist or not */
  iKey = hash_function(id, d->iBucketCount);
  for (curr = d->iTable[iKey]; curr; curr = curr->iNext){
    if (strcmp(curr->id, id) == 0 || strcmp(curr->name, name) == 0) {
      return -1;  /* Duplicate id or name found */
    }
  }
  nKey = hash_function(name, d->iBucketCount);
  for (curr = d->nTable[nKey]; curr; curr = curr->nNext) {
    if (strcmp(curr->id, id) == 0 || strcmp(curr->name, name) == 0) {
      return -1;  /* Duplicate id or name found */
    }
  }
  
  /* Allocate memory for the newUsr */
  newUsr = (struct UserInfo *)calloc(1, sizeof(struct UserInfo));
  if (newUsr == NULL) {
    fprintf(stderr, "Error: Unable to allocate memory for new user.\n");
    return -1; 
  }

  newUsr->id = strdup(id);
  if (newUsr->id == NULL) {
    fprintf(stderr, "Error: Unable to allocate memory for user ID.\n");
    free(newUsr); /* Clean up previously allocated memory */
    return -1; 
  }

  newUsr->name = strdup(name);
  if (newUsr->name == NULL) {
    fprintf(stderr, "Error: Unable to allocate memory for user name.\n");
    free(newUsr->id); /* Clean up previously allocated memory */
    free(newUsr);
    return -1; 
  }
  newUsr->purchase = purchase;
  
  if ((d->numItems >= LOAD_FACTOR * d->iBucketCount)  
                            && (d->iBucketCount < MAX_BUCKET_COUNT)){ /* Expand */
 
    /* Memory allocation for the new tables */
    int newBucketCount = 2 * d->iBucketCount;
    iTableTempo= (struct UserInfo **)calloc(newBucketCount, sizeof(struct UserInfo*));
    if (!iTableTempo) {
      fprintf(stderr, "Error: Memory failure to expand to the tables of size %d\n",
        newBucketCount); 
        free(newUsr->id);
        free(newUsr->name);
        free(newUsr);  
      return -1;
    }

    nTableTempo = (struct UserInfo **)calloc(newBucketCount, sizeof(struct UserInfo*));
    if (!nTableTempo) {
      fprintf(stderr, "Error: Memory failure to expand to the tables of size %d\n",
        newBucketCount);
      free(iTableTempo); 
      free(newUsr->id);
      free(newUsr->name);
      free(newUsr); 
      return -1;
    } 
    int processedItems = 0;  /* Tracks the number of valid items processed */
    for (int i = 0; i < d->iBucketCount && processedItems < d->numItems; i++) {
      curr = d->iTable[i];
      while (curr) {
        /* Calculate new hash keys for expanded table size */
        int iKey = hash_function(curr->id, newBucketCount);
        int nKey = hash_function(curr->name, newBucketCount);

        /* Save the next pointer before moving current item */
        next = curr->iNext;

        /* Insert current item into the new tables */
        curr->iNext = iTableTempo[iKey];
        curr->nNext = nTableTempo[nKey];
        iTableTempo[iKey] = curr;
        nTableTempo[nKey] = curr;

        /* Move to the next item */
        curr = next;

        /* Increment processed items count */
        processedItems++;

        /* Exit early if all items have been processed */
        if (processedItems >= d->numItems) break;
      }
    }

    /* Free the old tables and assign the new ones */ 
    free(d->iTable);
    free(d->nTable); 
    d->iTable = iTableTempo;
    d->nTable = nTableTempo;
    d->iBucketCount = newBucketCount;

    /* Insert newUsr in expanded tables */
    iKey=hash_function(id, d->iBucketCount);
    nKey=hash_function(name,d->iBucketCount);
    newUsr->nNext= d->nTable[nKey];
    newUsr->iNext= d->iTable[iKey];
    d->iTable[iKey]= newUsr;
    d->nTable[nKey]= newUsr;  
  }
  else{ 
    newUsr->nNext= d->nTable[nKey];
    newUsr->iNext= d->iTable[iKey];
    d->iTable[iKey]= newUsr;
    d->nTable[nKey]= newUsr;
  }
  d->numItems++;
  return 0;
}
/*--------------------------------------------------------------------*/
int
UnregisterCustomerByID(DB_T d, const char *id)
{
  struct UserInfo* delUsr=NULL; /* Pointer to item that is being unregistered*/
  struct UserInfo *next, *curr; /* For traversing the linked list */                           
  int iKey,nKey; /* Keeps hash keys */
  
  if (d == NULL || id == NULL) return -1; /* Nothing to delete */

  /*Find the hash value for the id*/
  iKey=hash_function(id, d->iBucketCount);

  if(d->iTable[iKey]==NULL) return -1; /* id doesn't exist */

  /* Check front of list */
  if (strcmp(d->iTable[iKey]->id, id) == 0) { 
    delUsr = d->iTable[iKey];
    d->iTable[iKey] = delUsr->iNext; /* Adjusting the id table */
  } 
  else {  
    /* Traverse the linked list to find the item */
    curr = d->iTable[iKey];
    next = curr->iNext;
    while (next) {
        if (strcmp(next->id, id) == 0) {
            delUsr = next;
            curr->iNext = delUsr->iNext; /* Remove from iTable list */
            break;
        }
        curr = next;
        next = curr->iNext;
    }
  }
  if(!delUsr) return -1; /* Item to be deleted is not found */

  /*Find the hash value for the name*/
  nKey=hash_function(delUsr->name, d->iBucketCount);

 /* Adjusting the nTable before releasing the memory */
  curr=d->nTable[nKey];
  if(curr==delUsr) { /* The item to be deleted is at front */
      d->nTable[nKey]=delUsr->nNext;
  }
  else{ /* Traverse to find delUsr in nTable */
    while(curr->nNext!=delUsr){
      curr=curr->nNext;
    }
    curr->nNext=delUsr->nNext; /* Adjust the list */
  }

  /*  Freeing the memory of to be deleted item */
  free(delUsr->id);
  free(delUsr->name);
  free(delUsr);

  /* Adjusting the database's number of items */
  d->numItems--;
  return 0;
}
/*--------------------------------------------------------------------*/
int
UnregisterCustomerByName(DB_T d, const char *name)
{
  struct UserInfo* delUsr=NULL; /* Pointer to item that is being unregistered*/
  struct UserInfo *next, *curr; /* For traversing the linked list */                           
  int iKey,nKey; /* Keeps hash keys */

  if (d == NULL || name == NULL) return -1; /* Nothing to delete */

  /*Find the hash value for the id*/
  nKey=hash_function(name, d->iBucketCount);
  
  if(d->nTable[nKey]==NULL) return -1; /* name doesn't exist */

  /* Check front of list */
  if(strcmp(d->nTable[nKey]->name,name)==0){ 
    delUsr=d->nTable[nKey];
    d->nTable[nKey]=delUsr->nNext;  /* Adjusting the iTable */
  }
  else{   
    curr=d->nTable[nKey];
    next=curr->nNext;
    while(next){  /* Iterate the linked list until finding the item*/
      if (strcmp(next->name ,name)==0){
        delUsr=next;
        curr->nNext=delUsr->nNext;
        break;
      }
      curr=next;
      next=curr->nNext;
    }
  }
  if(!delUsr) return -1; /* Item to be deleted is not found */

  /*Find the hash value for the id */
  iKey=hash_function(delUsr->id, d->iBucketCount);

  /* Adjusting the iTable before releasing the memory */
  curr=d->iTable[iKey];
  if(curr==delUsr) { /* The item to be deleted is at front */
      d->iTable[iKey]=delUsr->iNext;
  }
  else{ /* Moving curr's next untill it is equal delUsr */
      while(curr->iNext!=delUsr){
        curr=curr->iNext;
      }
      curr->iNext=delUsr->iNext; /* Adjust the list */
  }
  /* Freeing the memory of to be deleted item */
  free(delUsr->id);
  free(delUsr->name);
  free(delUsr);

  /* Adjusting the database's number of items */
  d->numItems--;
  return 0;
}
/*--------------------------------------------------------------------*/
int
GetPurchaseByID(DB_T d, const char* id)
{  
  struct UserInfo* curr; /* Iterator */
  int iKey;
  if (d == NULL || id == NULL) return -1; /* Invalid inputs */

  /* Hash values of the id */
  iKey=hash_function(id, d->iBucketCount);

  curr=d->iTable[iKey];
  while(curr){ /* Iterating the id list */
    if (strcmp(curr->id,id)==0) return curr->purchase;
    curr=curr->iNext;
  }
  return -1; /* No item of such id */ 
}

/*--------------------------------------------------------------------*/
int
GetPurchaseByName(DB_T d, const char* name)
{ 
  struct UserInfo* curr; /* Iterator */
  int nKey;
  if (d == NULL || name == NULL) return -1; /* Invalid inputs */

  /* Hash values of the name */
  nKey=hash_function(name, d->iBucketCount);

  curr=d->nTable[nKey];
  while(curr){ /* Iterating the name list */
    if (strcmp(curr->name,name)==0) return curr->purchase;
    curr=curr->nNext;
  }
  return -1; /* No item of such name */
}

/*--------------------------------------------------------------------*/
int GetSumCustomerPurchase(DB_T d, FUNCPTR_T fp) {
  struct UserInfo *curr; /* Iterator */
  int processedItems = 0, i = 0, total = 0;

  if (d == NULL || fp == NULL) return -1; /* Invalid inputs */

  /* Iterate through each bucket until all filled items are processed */
  while (processedItems < d->numItems) {
    curr = d->iTable[i];
    while (curr) {
      total += fp(curr->id, curr->name, curr->purchase);
      processedItems++; /* Increment count for each valid entry processed */
      curr = curr->iNext;
    }
    i++;
  }
  return total;
}