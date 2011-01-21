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
 * pqheap.c functions the create add and remove elements from a PQ heap
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *            Laura Toma - <ltoma@bowdoin.edu>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: Based on Laura Toma's PQ
 *
 *****************************************************************************/

#include <assert.h>
#include <stdlib.h>

#include "pqheap.h"

// setting this enables printing pq debug info 
#define PQ_DEBUG if(0)

// This is a special point which will be used to mark that the max
// error for a given triangle is less than e and thus it is 'done'
extern R_POINT *DONE;

// Initial size of the PQ (factor of 2)
const int  PQINITSIZE = 16384;


// arrays in C start at 0.  For simplicity the heap structure is
// slightly modified as:
/*    0
//    |
//    1
//    /\
//   2  3
//  /\  /\
// 4 5 6  7 */


//
// The left children of an element of the heap.
//
static inline unsigned int heap_lchild(unsigned int index) {
  return 2 * index;
}


//
// The right children of an element of the heap.
//
static inline unsigned int heap_rchild(unsigned int index) {
  return 2 * index + 1;
}


//
// The parent of an element.
//
static inline unsigned int heap_parent(unsigned int index) {
  return index >> 1;
}


//
// Return minimum of two integers 
//
static unsigned int mymin(unsigned int a, unsigned int b) {
  return (a<=b)? a:b;
}

//
// Enforce heap property on a path in the tree
//
static void heapify(PQueue* pq, unsigned int root) {

  unsigned int min_index = root;
  unsigned int lc = heap_lchild(root);
  unsigned int rc = heap_rchild(root);

  assert(pq && pq->elements);
  if ((lc < pq->cursize) && 
      (getPriority(pq->elements[lc]) < getPriority(pq->elements[min_index]))) {
    min_index = lc;
  }
  if ((rc < pq->cursize) && 
      (getPriority(pq->elements[rc]) < getPriority(pq->elements[min_index]))) {
    min_index = rc;
  }
  
  if (min_index != root) {
    PQ_elemType tmp_q = pq->elements[min_index];
    
    pq->elements[min_index] = pq->elements[root];
    pq->elements[root] = tmp_q;

    // This is triangle specific: Need to update triangle's pointer to
    // itself in the PQ
    pq->elements[min_index]->pqIndex = min_index;
    pq->elements[root]->pqIndex = root;
    
    heapify(pq, min_index);
  }
}   


//
// Double the size of the pq by mallocing a new array
//
static void PQ_grow(PQueue* pq) {

  PQ_elemType* elements;
  unsigned int i;
  PQ_DEBUG{printf("PQ: doubling size to %d\n",pq->maxsize*2); fflush(stdout);}
  
  assert(pq && pq->elements);
  pq->maxsize *= 2; 
  elements = (PQ_elemType*)malloc(pq->maxsize*sizeof(PQ_elemType));
  if (!elements) {
    printf("PQ_grow: could not reallocate priority queue: insufficient memory..\n");
    exit(1);
  }
  /* should use realloc..*/
  for (i=0; i<pq->cursize; i++) {
    elements[i] = pq->elements[i];
    //Update triangle
    elements[i]->pqIndex = i;
  }
  free(pq->elements);
  pq->elements = elements;
}


//
// Create and initialize a pqueue and return it
//
PQueue* PQ_initialize(unsigned int initSize) {
  PQueue *pq; 

  initSize = PQINITSIZE;

  PQ_DEBUG{printf("PQ-initialize: initializing heap with %ud elements\n",
		  initSize); fflush(stdout);}
  pq = (PQueue*)malloc(sizeof(PQueue));
  assert(pq);
  pq->elements = (PQ_elemType*)malloc(initSize*sizeof(PQ_elemType));
  if (!pq->elements) {
    printf("PQ_initialize: could not allocate priority queue: insufficient memory..\n");
    exit(1);
  }
  assert(pq->elements);
  
  pq->maxsize = initSize;
  pq->cursize = 0;
  return pq;
}

//
// Delete the pqueue and free its space 
//
void PQ_free(PQueue* pq) { 

  PQ_DEBUG{printf("PQ-delete: deleting heap\n"); fflush(stdout);}
  assert(pq && pq->elements); 
  if (pq->elements) free(pq->elements);
}
 
//
// Is it empty? 
//
int  PQ_isEmpty(PQueue* pq) {
  assert(pq && pq->elements); 
  return (pq->cursize == 0);
}

//
// Return the nb of elements currently in the queue
//
unsigned int PQ_size(PQueue* pq) {
  assert(pq && pq->elements);
  return pq->cursize;
}


//
// Set *elt to the min element in the queue; return value: 1 if exists
// a min, 0 if not
//
int PQ_min(PQueue* pq, PQ_elemType* elt) {

  assert(pq && pq->elements); 
  if (!pq->cursize) {
    return 0;
  }
  *elt = pq->elements[0];
  return 1;
}


//
// Set *elt to the min element in the queue and delete it from queue;
// return value: 1 if exists a min, 0 if not
//
int PQ_extractMin(PQueue* pq, PQ_elemType* elt) {

  assert(pq && pq->elements);
  if (!pq->cursize) {
    return 0;
  }
  *elt = pq->elements[0];
  pq->elements[0] = pq->elements[--pq->cursize];
  //Update triangle
  pq->elements[0]->pqIndex = 0;
  heapify(pq, 0);

  PQ_DEBUG {printf("PQ_extractMin: "); printElem(*elt); printf("\n"); 
  fflush(stdout);}
  return 1;
}


//
// Delete the min element; same as PQ_extractMin, but ignore the value
// extracted; return value: 1 if exists a min, 0 if not
//
int  PQ_deleteMin(PQueue* pq) {
    
  assert(pq && pq->elements); 
  PQ_elemType dummy;
  return PQ_extractMin(pq, &dummy);
}


//
// Insert the element into the PQ
//
void PQ_insert(PQueue* pq, PQ_elemType elt) {
  
  unsigned int ii;
  assert(pq && pq->elements); 
  assert(elt->maxE != DONE);
 
  PQ_DEBUG {printf("PQ_insert: "); printElem(elt); printf("\n"); fflush(stdout);}
  if (pq->cursize==pq->maxsize) {
    PQ_grow(pq);
  }
  assert(pq->cursize < pq->maxsize);
  for (ii = pq->cursize++;
       ii && (getPriority(pq->elements[heap_parent(ii)]) > getPriority(elt));
       ii = heap_parent(ii)) {
    pq->elements[ii] = pq->elements[heap_parent(ii)];
    //Update triangle
    pq->elements[ii]->pqIndex = ii;
  }
  pq->elements[ii] = elt;
  //Update triangle
  pq->elements[ii]->pqIndex = ii;
}                                       


//
// Delete the min element and insert the new item x; by doing a delete
// and an insert together you can save a heapify() call
//
void PQ_deleteMinAndInsert(PQueue* pq, PQ_elemType elt) {
  
  assert(pq && pq->elements); 
  PQ_DEBUG {printf("PQ_deleteMinAndinsert: "); printElem(elt); 
  printf("\n"); fflush(stdout);}
  pq->elements[0] = elt;
  //Update triangle
  pq->elements[0]->pqIndex = 0;
  heapify(pq, 0);
}

//
// Delete an element from the PQ
//
int PQ_delete(PQueue* pq,unsigned int index){

  assert(pq && pq->elements);
  if (!pq->cursize || index >= pq->cursize) {
    return 0;
  }

  // Put the last element in the place of the deleted element
  pq->elements[index] = pq->elements[--pq->cursize];
  //Update triangle
  pq->elements[index]->pqIndex = index;
  heapify(pq, index);

  return 1;

}


//
// print the elements in the queue
//
void PQ_print(PQueue* pq) {
  printf("PQ: "); fflush(stdout);
  unsigned int i;
  for (i=0; i< mymin(10, pq->cursize); i++) {
    printElem(pq->elements[i]);
    printf("\n");fflush(stdout);
  }
  printf("\n");fflush(stdout);
}
   


