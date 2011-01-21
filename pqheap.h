/* ************************************************************
* 
*  MODULE:	r.refine
*
*  Authors:	Jon Todd <jonrtodd@gmail.com>,  Laura Toma <ltoma@bowdoin.edu>
 * 		        Bowdoin College, USA
*
*  Purpose:	convert grid data to TIN 
*
*  COPYRIGHT:  
*			This program is free software under the GNU General Public
*	       		License (>=v2). Read the file COPYING that comes with GRASS
*              	for details.
*
*
************************************************************  */

/******************************************************************************
 * 
 * pqheap.h functions the create add and remove elements from a PQ heap
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *            Laura Toma - <ltoma@bowdoin.edu>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: Based on Laura Toma's PQ
 *
 *****************************************************************************/

#ifndef _PQHEAP_H
#define _PQHEAP_H


/* includes the definition of a pqueue element */
#include "pqelement.h" 

//
//Priority queue of elements of type elemType.  
//elemType assumed to have defined a function getPriority(elemType x)

// Define PQ structure
typedef struct {
  /* A pointer to an array of elements */
  PQ_elemType* elements;
  
  /* The number of elements currently in the queue */
  unsigned int cursize;
  
  /* The maximum number of elements the queue can currently hold */
  unsigned int maxsize;

} PQueue; 


//
// Create and initialize a pqueue and return it
//
PQueue* PQ_initialize(unsigned int initSize);

//
// Delete the pqueue and free its space 
//
void PQ_free(PQueue* pq);

//
// Is it empty? 
//
int  PQ_isEmpty(PQueue* pq);

//
// Return the nb of elements currently in the queue
//
unsigned int PQ_size(PQueue* pq);

//
// Set *elt to the min element in the queue; return value: 1 if exists
// a min, 0 if not
//
int PQ_min(PQueue* pq, PQ_elemType* elt);

//
// Set *elt to the min element in the queue and delete it from queue;
// return value: 1 if exists a min, 0 if not
//
int PQ_extractMin(PQueue* pq, PQ_elemType* elt);

//
// Delete the min element; same as PQ_extractMin, but ignore the value
// extracted; return value: 1 if exists a min, 0 if not
//
int  PQ_deleteMin(PQueue* pq);

//
// Insert the element into the PQ
//
void PQ_insert(PQueue* pq, PQ_elemType elt);

//
// Delete the min element and insert the new item x; by doing a delete
// and an insert together you can save a heapify() call
//
void PQ_deleteMinAndInsert(PQueue* pq, PQ_elemType elt);

//
// Delete an element from the PQ
//
int PQ_delete(PQueue* pq,unsigned int index);

//
// print the elements in the queue
//
void PQ_print(PQueue* pq);

#endif // _PQUEUE_HEAP_H 
