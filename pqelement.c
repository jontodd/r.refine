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
 * pqelement.c functions for an element in a priority queue
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#include <assert.h>
#include "pqelement.h"
#include "geom_tin.h"
#include "constants.h"


//
// determine the priority of an element
//
double getPriority(PQ_elemType t) {
  assert(t);
  //return -error;
  return -t->maxErrorValue;
} 

//
// print a elements
//
void printElem(PQ_elemType t) {
  assert(t);
  char str[100];
  sprintf(str,"[%s] x:%%d y:%%d z:%s ptr:%%p",
	  ELEV_TYPE_PRINT_CHAR,ELEV_TYPE_PRINT_CHAR);
  printf(str, t->maxErrorValue,t->maxE->x,t->maxE->y,
	 t->maxE->z,t->maxE);
} 
