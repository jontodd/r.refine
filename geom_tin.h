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
 * geom_tin.h contains geometric functions used to determin various
 * properties of the triangulation
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#ifndef __geom_tin_h
#define __geom_tin_h

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "point.h"
#include "triangle.h"
#include "constants.h"

//
// Computes the determinant of a 2x2 matrix
//
long determinant(ELEV_TYPE a, ELEV_TYPE b,ELEV_TYPE c,ELEV_TYPE d);

//
// Given and triangle and x,y; interpolate z
//
long interpolate(R_POINT* p1, R_POINT* p2, R_POINT* p3,COORD_TYPE px,
		 COORD_TYPE py);

//
// Find the error of a given point in a triangle
//
ELEV_TYPE findError(COORD_TYPE row,COORD_TYPE col,ELEV_TYPE height,
		    TRIANGLE* t);

#endif
