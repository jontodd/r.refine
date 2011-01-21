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
 * qsort.h contains compare function for use with the GNU quicksort function
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: man qsort for more info
 *
 *****************************************************************************/

#ifndef QSORT_H
#define QSORT_H

#include "point.h"
#include <assert.h>
#include "constants.h"

//
// comare point a and b to determine order, sort first by x then by y
//
int QS_compPoints( R_POINT **a, R_POINT **b);
	     
#endif // QSORT_H
