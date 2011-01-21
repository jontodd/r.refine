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
 * refine_tin.h can take a tiled grid file and create a refined tin.
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-11
 *
 * COMMENTS:
 *
 *****************************************************************************/

#ifndef __refine_tin_h
#define __refine_tin_h

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "tin.h"
#include "constants.h"
#include "qsort.h"


//
// Initialize TIN structure, returns a pointer to lower left tri. This
// will not initialize the points in the triangles, just the two
// staring triangles. Points will be added later in refinement.
//
TIN_TILE* initTinTile(TIN_TILE *tt,TIN_TILE *leftTile, TIN_TILE *topTile,
		      TRIANGLE *leftTri, TRIANGLE *topTri, 
		      R_POINT *nw, R_POINT *sw, R_POINT *ne, int iOffset, 
		      int jOffset, short useNodata, TIN *tin);

//
// Find neighbor tile to tt and point ttn to it. If tt is under ttn
// then dir would be bottom
//
void pointNeighborTileTo(TIN_TILE *tt, short dir, TIN_TILE *ttn);

//
// Take in the big grid, and splits into tiles, initialize all tiles
// and return tin
//
TIN *initTin(TILED_GRID *fullGrid, double e, double mem, short useNodata,
	     char *name);

//
// Add the points two the two initial triangles of a Tin tile from
// file. Also add points from neighbor boundary arrays to the
// triangulation to have boundary consistancy
//
TIN_TILE *initTilePoints(TIN_TILE *tt, double e, short useNodata);

//
//   Return TRUE if a point (xp,yp) is inside the circumcircle made up
//   of the points (x1,y1), (x2,y2), (x3,y3)
//   The circumcircle centre is returned in (xc,yc) and the radius r
//   NOTE: A point on the edge is inside the circumcircle
//
int CircumCircle(double xp,double yp,double x1,double y1,
		 double x2,double y2,double x3,double y3);

//
// Swap the common edge between two triangles t1 and t2. The common
// edge should always be edge ac, abc are part of t1 and acd are part
// of t2.
//
void edgeSwap(TRIANGLE *t1, TRIANGLE *t2, 
	      R_POINT *a, R_POINT *b, R_POINT *c, R_POINT *d,
	      double e, TIN_TILE *tt);
//
// Enforce delaunay on triangle t. Assume that p1 & p2 are the
// endpoints to the edge that is being checked for delaunay
//
void enforceDelaunay(TRIANGLE *t, R_POINT *p1, R_POINT *p2, R_POINT *p3,
		     double e, TIN_TILE *tt);

// 
// Refine each tile individually, write it to disk, and free it from
// memory. This way only one tile and boundary arrays are in memory at
// one time.
//
void refineTin(double e, short delaunay, TIN *tin,char *path,
	       char *siteFileName, char *vectFileName, short useNodata);

//
// Refine a grid into a TIN_TILgE with error < e
//
void refineTile(TIN_TILE *tt, double e, short delaunay, short useNodata);

//
// Add triangles and distribute points when there are two collinear
// triangles. Assumes that maxE is on line pa pb
//
void fixCollinear(R_POINT *pa, R_POINT *pb, R_POINT *pc, TRIANGLE* s, 
		  double e, R_POINT *maxError, TIN_TILE *tt, short delaunay);

//
// Divide a pointlist of s and sp into the three pointlists of the new
// tris. This assumes that s exists and has a point list. Most of the
// time sp will be NULL because we are distributing points from a
// parent triangle. sp may not be NULL when edge swapping for
// delaunay. Assume that if sp is not null then it has a valid point list
//
 void distrPoints(TRIANGLE* t1, TRIANGLE* t2, TRIANGLE* t3, TRIANGLE* s, 
		  TRIANGLE* sp, double e, TIN_TILE *tt);

//
// This function is called when we are refining the lower left most
// triangle since we need to choose the new lower left triangle
//
void updateTinTileCorner(TIN_TILE *tt, TRIANGLE* t1, TRIANGLE* t2, 
			 TRIANGLE* t3);

#endif
