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
 * queue.h has all functions to support a queue of elements
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *            Laura Toma - <ltoma@bowdoin.edu>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: Based on Laura Toma's PQ
 *
 *****************************************************************************/

#ifndef QUEUE_H
#define QUEUE_H

#include "point.h"

typedef R_POINT queueElem;

typedef struct queueNode {
  queueElem e;
  struct queueNode* next;
} QNODE;

typedef QNODE* QUEUE;

//
//create a head, point it to itself and return a pointer to the head
//of the list
//
QUEUE Q_init();

//
// insert element e at the head of the list by copying its contents into QNODE
//
void Q_insert_elem_head(QUEUE h, queueElem e);

//
// insert QNODE at head of the list
//
void Q_insert_qnode_head(QUEUE h, QNODE* n);

//
// remove the first element in the queue
//
QNODE* Q_remove_first(QUEUE h);

//
// remove the first element and free it from memory
//
short Q_delete_first(QUEUE h);

//
// Free all the elements in the queue
//
void Q_free_queue(QUEUE h);

//
// Print all the elements in a queue
//
void Q_print(QUEUE h);

//
// returns first element after head, or NULL if queue is empty (dummy only)
//
QNODE* Q_first(QUEUE h);

//
//returns p->next
//
QNODE* Q_next(QUEUE h, QNODE* p);

//
//returns 1 if p is the last element in the queue else 0
//
short Q_isEnd(QUEUE h, QNODE* p);

#endif // QUEUE_H
