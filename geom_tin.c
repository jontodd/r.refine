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
 * geom_tin.c contains geometric functions used to determin various
 * properties of the triangulation
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#include "geom_tin.h"


//
// Computes the determinant of a 2x2 matrix
//
long determinant(ELEV_TYPE a, ELEV_TYPE b,ELEV_TYPE c,ELEV_TYPE d){
  return ((a*d) - (b*c));
}


//
// Given and triangle and x,y; interpolate z
//
long interpolate(R_POINT* p1, R_POINT* p2, R_POINT* p3,COORD_TYPE px,
		 COORD_TYPE py) {
  long d1 = determinant((p2->y-p1->y), (p2->z-p1->z), 
			  (p3->y-p1->y), (p3->z-p1->z)); 
  long d2 = determinant((p2->x-p1->x), (p2->z-p1->z), 
			  (p3->x-p1->x), (p3->z-p1->z)); 
  long d3 = determinant((p2->x-p1->x), (p2->y-p1->y), 
			  (p3->x-p1->x), (p3->y-p1->y)); 
                                                                               
  // d3 should never be 0                                                   
  if (d3 == 0){                                                                
    printf("determinant should never be 0\n");                                 
    exit(1);                                                                   
  }                                                                            
                                                                               
 return ((( ((py - p1->y) * d2) - ((px - p1->x) * d1) ) / d3) + p1->z);       
                                                                               
}


//
// Find the error of a given point in a triangle
//
ELEV_TYPE findError(COORD_TYPE row,COORD_TYPE col,ELEV_TYPE height,
		    TRIANGLE* t){
  long err = interpolate(t->p1,t->p2,t->p3, row, col);
  return  fabs((ELEV_TYPE)height-err);
}
