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
 * qsort.c contains compare function for use with the GNU quicksort function
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: man qsort for more info
 *
 *****************************************************************************/

#include "qsort.h"

//
// comare point a and b to determine order, sort first by x then by y
//	
int QS_compPoints(R_POINT **p1, R_POINT **p2){
  R_POINT *a = *p1;
  R_POINT *b = *p2;
  if(a->x < b->x)
    return -1;
  else if(a->x > b->x)
    return 1;
  else{ // x's are the same so compare y's
    if(a->y < b->y)
      return -1;
    else if(a->y > b->y)
      return 1;
    else{
      return 0;
    }
  }
}  
