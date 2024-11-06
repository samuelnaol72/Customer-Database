/*
 * Program: customer_manager1.c
 * Student Name: Naol Samuel Erega
 * Student ID: 20210889
 * Assignment: 3
 *
 * Description:
 * ------------
 * This program implements a simple customer management database that allows 
 * registration, unregistration, and retrieval of customer data using dynamic array.
 * The database  supports dynamic resizing to accommodate a growing number of customers 
 * and stores each customer's name, ID, and purchase amount.
 * 
 * Functionality:
 * --------------
 * 1. Provides functions to create and destroy a customer database (`CreateCustomerDB` 
 *    and `DestroyCustomerDB`).
 * 2. Supports registering new customers (`RegisterCustomer`) and expanding the database 
 *    when necessary.
 * 3. Allows unregistration of customers by either ID or name (`UnregisterCustomerByID` 
 *    and `UnregisterCustomerByName`).
 * 4. Offers retrieval functions to check purchase amounts by either ID or name (`GetPurchaseByID`
 *    and `GetPurchaseByName`).
 * 5. Includes a utility to calculate the total sum of customer purchases (`GetSumCustomerPurchase`), 
 *    using a function pointer to allow customized calculations.
*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "customer_manager.h"
#define UNIT_ARRAY_SIZE 1024

struct UserInfo {
  char *name;                // customer name
  char *id;                  // customer id
  int purchase;              // purchase amount (> 0)
};
struct DB {
  struct UserInfo *pArray;   // pointer to the array
  int curArrSize;            // current array size (max # of elements)
  int numItems;              // # of stored items, needed to determine
};
/*--------------------------------------------------------------------*/
DB_T
CreateCustomerDB(void)
{ 
  DB_T d;
  d = (DB_T) calloc(1, sizeof(struct DB));
  if (d == NULL) {
    fprintf(stderr, "Error: Can't allocate a memory for DB_T\n");
    return NULL;
  }
  d->curArrSize = UNIT_ARRAY_SIZE; // start with 1024 elements
  d->pArray = (struct UserInfo *)calloc(d->curArrSize,
               sizeof(struct UserInfo));
  if (d->pArray == NULL) {
    fprintf(stderr, "Error: Can't allocate a memory for array of size %d\n",
	    d->curArrSize);   
    free(d);
    return NULL;
  }
  return d;
  return NULL;
}
/*--------------------------------------------------------------------*/
void DestroyCustomerDB(DB_T d) {
    struct UserInfo* curr; /* Iterator */
    if (d == NULL) return; /* Nothing to do if d is NULL */
    int processedItems = 0; /* Tracks the number of valid items processed */

    for (int i = 0; i < d->curArrSize && processedItems < d->numItems; i++) {
        curr = &d->pArray[i];
        /* Check if the current entry is valid */
        if (curr->id != NULL && curr->name != NULL) {
          /* Free allocated memory for the valid user's name and id */
          free(curr->name);
          free(curr->id);
          processedItems++; /* Increment the count of processed valid items */
        }
    }

    /* Free the array and the database structure */
    free(d->pArray);
    free(d);
}

/*--------------------------------------------------------------------*/
int 
RegisterCustomer(DB_T d, const char *id, const char *name, const int purchase)
{
  /* Treat invalid input as failure */
  if (d == NULL || id == NULL || name == NULL || purchase <= 0) return -1; 

  struct UserInfo *curr,*temp; /* Iterator, temporary pointer for expansion*/
  int n=d->numItems;  /*  To check whether the user already exist */
  int flag=0; /* flag that last position for the new user found*/
  int last=n; 
  if(d->numItems>=d->curArrSize){ /* The database is full */ 
    /* Checking whether the user already exist */ 
    for (int i = 0; i < d->numItems; i++) {
      curr = &d->pArray[i];
      if (strcmp(curr->id, id) == 0 || strcmp(curr->name, name) == 0) {
        return -1;  /* Duplicate id or name found */
      }
    }

    /* Expand and store */
    temp = realloc(d->pArray, (d->curArrSize + UNIT_ARRAY_SIZE) * sizeof(struct UserInfo));
    
    /* Check the allocaton status */
    if (temp == NULL) {
      fprintf(stderr, "Error: Can't allocate a memory for expansion of the array\n");
      return -1; 
    }
    /* Update the array pointer and size */ 
    d->pArray = temp;
    d->curArrSize += UNIT_ARRAY_SIZE;
  }
  else{ /* Array not full */ 

    /* 
    Checking whether the user already exist && keeping track of 
    the first null pointer for later use when registering !
    */
    for (int i = 0; i < n; i++) {
      curr = &d->pArray[i];
      if(curr->id==NULL || curr->name==NULL) { /* Last position to store found! */
        n++; /* Expanding the search space */
        if(flag==0) {
          last=i; /* Keep position to store */
          flag++; /* flag that the position found */
        }
      }
      else if (strcmp(curr->id, id) == 0 || strcmp(curr->name, name) == 0) {
        return -1;  /* Duplicate ID or name found */
      }
    }
      
  }
  /* Registering new item */ 
  d->pArray[last].name= strdup(name);
  if (d->pArray[last].name == NULL) {  /* strdup failed */ 
    fprintf(stderr, "Error: Can't allocate a memory for name of the new item\n");
    return -1;
  } 
      
  d->pArray[last].id = strdup(id);
  if (d->pArray[last].id == NULL) {
    fprintf(stderr, "Error: Can't allocate a memory for id of the new item\n");
    free(d->pArray[last].name);  // Free previously allocated name
    d->pArray[last].name = NULL;
    return -1;  /* strdup failed */
  }
  d->pArray[last].purchase = purchase;

  /* Size adjustement for the database */
  d->numItems++; 

  return 0; /* Register success! */
}
/*--------------------------------------------------------------------*/
int UnregisterCustomerByID(DB_T d, const char *id) {
  struct UserInfo* curr; /* Current iterator */

  if (d == NULL || id == NULL) return -1; /* Treat invalid input as failure */

  int processedItems = 0; /* Tracks the number of valid items processed */
  for (int i = 0; i < d->curArrSize && processedItems < d->numItems; i++) {
    curr = &d->pArray[i];

    if (curr->id == NULL || curr->name == NULL) continue;

    /* Increment processed items count for each valid entry checked */
    processedItems++;

    if (strcmp(curr->id, id) == 0) { /* ID found */ 
      /* Free the allocated memory for id and name */
      free(curr->id);
      free(curr->name);

      /* Nullifying for efficient re-registration later */
      curr->id = NULL;
      curr->name = NULL;
      curr->purchase = 0;

      /* Update the number of items */ 
      d->numItems--;  

      return 0; /* User unregistered successfully */
    }
  }
  return -1; /* User ID doesn't exist */
}

/*--------------------------------------------------------------------*/
int
UnregisterCustomerByName(DB_T d, const char *name)
{
  struct UserInfo* curr; /* Current iterator */

  if (d == NULL || name == NULL) return -1; /* Treat invalid input as failure */

  int processedItems = 0; /* Tracks the number of valid items processed */
  for (int i = 0; i < d->curArrSize && processedItems < d->numItems; i++) {
    curr = &d->pArray[i];

    if (curr->id == NULL || curr->name == NULL) continue;

    /* Increment processed items count for each valid entry checked */
    processedItems++;

    if (strcmp(curr->name, name) == 0) { /* ID found */ 
      /* Free the allocated memory for id and name */
      free(curr->id);
      free(curr->name);

      /* Nullifying for efficient re-registration later */
      curr->id = NULL;
      curr->name = NULL;
      curr->purchase = 0;

      /* Update the number of items */ 
      d->numItems--;  

      return 0; /* User unregistered successfully */
    }
  }
  return -1; /* User name doesn't exist */
}

/*--------------------------------------------------------------------*/
int GetPurchaseByID(DB_T d, const char* id) {  
  struct UserInfo* curr; /* Current iterator */
  if (d == NULL || id == NULL) return -1; /* Treat invalid input as failure */

  int processedItems = 0; /* Tracks the number of valid items processed */
  for (int i = 0; i < d->curArrSize && processedItems < d->numItems; i++) {
    curr = &d->pArray[i];

    if (curr->id == NULL || curr->name == NULL) continue;

    /* Increment processed items count for each valid entry checked */
    processedItems++;

    /* Check if the current entry matches the given ID */
    if (strcmp(curr->id, id) == 0) { /* User ID exists */
      return curr->purchase; /* Return the purchase amount */
    }
  }
  return -1; /* No user with such ID */
}

/*--------------------------------------------------------------------*/
int
GetPurchaseByName(DB_T d, const char* name)
{
  struct UserInfo* curr; /* Current iterator */
  if (d == NULL || name == NULL) return -1; /* Treat invalid input as failure */

  int processedItems = 0; /* Tracks the number of valid items processed */
  for (int i = 0; i < d->curArrSize && processedItems < d->numItems; i++) {
    curr = &d->pArray[i];

    if (curr->id == NULL || curr->name == NULL) continue;

    /* Increment processed items count for each valid entry checked */
    processedItems++;

    /* Check if the current entry matches the given ID */
    if (strcmp(curr->name, name) == 0) { /* User name exists */
      return curr->purchase; /* Return the purchase amount */
    }
  }
  return -1; /* No user with such name */
}
/*--------------------------------------------------------------------*/
int GetSumCustomerPurchase(DB_T d, FUNCPTR_T fp) {
  struct UserInfo* curr; /* Current iterator */
  if (d == NULL || fp == NULL) return -1; /* Treat invalid input as failure */

  int total = 0; /* Purchase accumulated here */
  int processedItems = 0; /* Tracks the number of valid items processed */

  for (int i = 0; i < d->curArrSize && processedItems < d->numItems; i++) {
    curr = &d->pArray[i];

    if (curr->id == NULL || curr->name == NULL) continue;

    /* Increment processed items count for each valid entry checked */
    processedItems++;

    /* Accumulate the result of fp applied to the current entry */
    total += fp(curr->id, curr->name, curr->purchase);
  }
  return total; /* Return accumulated sum */
}