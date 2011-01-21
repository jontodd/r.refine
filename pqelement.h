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
 * pqelement.h functions for an element in a priority queue
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#ifndef PQELEMENT_H
#define PQELEMENT_H


#include <stdio.h>
#include "triangle.h"

//
// this is the type for an element in the priority queue; should be
// defined appropriately
//
typedef TRIANGLE* PQ_elemType; 

//
// determine the priority of an element
//
double getPriority(PQ_elemType t);

//
// print a elements
//
void printElem(PQ_elemType t);


#endif
