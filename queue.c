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
 * queue.c has all functions to support a queue of elements
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *            Laura Toma - <ltoma@bowdoin.edu>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: Based on Laura Toma's PQ
 *
 *****************************************************************************/

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define DEBUG if(0)

//
// NOTE:
// Queue has a dummy head and the end node points back to the head.
//


//
//create a head, point it to itself and return a pointer to the head
//of the list
//
QUEUE Q_init(){ 
  QNODE* n = (QNODE*)malloc(sizeof(QNODE));
  assert(n);
  n->next = n;
  DEBUG{printf("Q_init %p\n",n);fflush(stdout);}
  return n;
}

//
// insert element e at the head of the list by copying its contents into QNODE
//
void Q_insert_elem_head(QUEUE h, queueElem e){
  assert(h);
  assert(&e);
  DEBUG{
    char str[100];
    sprintf(str,"Q_insert_elem_head: %s %s %s\n",
	    COORD_TYPE_PRINT_CHAR,COORD_TYPE_PRINT_CHAR,ELEV_TYPE_PRINT_CHAR);
    printf(str,e.x,e.y,e.z);
    fflush(stdout);
  }

  QNODE* n = (QNODE*)malloc(sizeof(QNODE));
  assert(n);
  n->e.x = e.x;
  n->e.y = e.y;
  n->e.z = e.z;
  n->next = h->next;
  h->next = n;
}


//
// insert QNODE at head of the list
//
void Q_insert_qnode_head(QUEUE h, QNODE* n){
  assert(h && n);
  DEBUG{printf("Q_insert_qnode_head: %p\n",n);fflush(stdout);}
  
  n->next = h->next;
  h->next = n;
}


//
// remove the first element in the queue
//
QNODE* Q_remove_first(QUEUE h){
  assert(h);
  DEBUG{printf("Q_remove_first %p\n",h);fflush(stdout);}
  
  // If the head is first don't remove it
  if(Q_first(h) == NULL)
    return NULL;
  else{
    QNODE* f;
    f = Q_first(h);
    assert(f);
    h->next = f->next;
    return f;
  }
}


//
// remove the first element and free it from memory
//
short Q_delete_first(QUEUE h){
  assert(h);
  DEBUG{printf("Q_delete_first\n");fflush(stdout);}
  
  // If the head is first don't remove it
  if(Q_first(h) == NULL)
    return 0;
  
  else{
    QNODE* f;
    f = Q_first(h);
    assert(f);
    h->next = f->next;
    free(f);
    return 1;
  }
} 


//
// Free all the elements in the queue
//
void Q_free_queue(QUEUE h){
  assert(h);
  while(Q_delete_first(h));
  //free(h);
  h = NULL;
}


//
// Print all the elements in a queue
//
void Q_print(QUEUE h){
  assert(h);
  QNODE *n;
  n = Q_first(h);
  printf("Printing queue: %p\n",h);
  char str[100];
  sprintf(str,"\t(%s %s %s)\n",
	  COORD_TYPE_PRINT_CHAR,COORD_TYPE_PRINT_CHAR,ELEV_TYPE_PRINT_CHAR);
   
  while(n != NULL){
    printf(str,n->e.x,n->e.y,n->e.z);
    n = Q_next(h,n);
  }
    
}


//
// returns first element after head, or NULL if queue is empty (dummy only)
//
QNODE* Q_first(QUEUE h){
  assert(h);
  return (h->next == h)? NULL: h->next;
}

//
//returns p->next
//
QNODE* Q_next(QUEUE h, QNODE* p){
  assert(h && p);
  return (p->next == h)? NULL: p->next;
} 

//
//returns 1 if p is the last element in the queue else 0
//
short Q_isEnd(QUEUE h, QNODE* p){
  assert(h && p);
  return (p->next == h);
}

