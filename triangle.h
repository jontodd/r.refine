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
 * triangle.h defines the triangle structure
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "queue.h"
#include "point.h"


typedef struct Triangle {
  R_POINT *maxE;            // Pointer to the point with the max error
  R_POINT *p1,*p2,*p3;      // Three corner points
  ELEV_TYPE maxErrorValue;  // Value of the max error point
  struct Triangle* p1p2;    // Neighbor triangle
  struct Triangle* p1p3;    // Neighbor triangle
  struct Triangle* p2p3;    // Neighbor triangle
  unsigned int pqIndex;     // Pointer to this triangles in the PQ
  QUEUE points;             // Queue of points inside the tri (NULL if done)
} TRIANGLE; 

#endif
