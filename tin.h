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
 * tin.h handles all creation,manipulation, and output of TIN and
 * TIN_TIlE structure
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#ifndef __tin_h
#define __tin_h

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <string.h>


#include "grid.h"
#include "geom_tin.h"
#include "triangle.h"
#include "pqelement.h"
#include "pqheap.h"
#include "constants.h"
#include "qsort.h"


///////////////////////////////////////////////////////////////////////////////
// Type Definitions ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef char BOOL;

typedef struct Edge {
  TRIANGLE *t1, *t2;
  R_POINT *p1, *p2;
  short type;
} EDGE;

typedef struct Tin_Tile {
  TRIANGLE *t;       // lower left most tri
  R_POINT* v;          // lower left vertex of t
  PQueue *pq;        // pq used in refinement
  COORD_TYPE nrows;  // number of rows
  COORD_TYPE ncols;  // number of cols
  EDGE e;            // lower left edge
  ELEV_TYPE nodata;  // value for nodata points
  COORD_TYPE iOffset;     // i offset for tile
  COORD_TYPE jOffset;     // j offset for tile
  ELEV_TYPE min;         // minimum height in this tile
  ELEV_TYPE max;         // max height in this tile
  struct Tin_Tile *top;     // Top neighbor tile
  struct Tin_Tile *bottom;  // Bottom neighbor tile
  struct Tin_Tile *right;   // Right neighbor tile
  struct Tin_Tile *left;    // Left neighbor tile
  struct Tin_Tile *next;    // pointer to next tile in the list
  // pointer to the four corners
  R_POINT* nw;              
  R_POINT* ne;
  R_POINT* sw;
  R_POINT* se;
  unsigned int numTris;     // number of triangles in the tile
  unsigned int numPoints;   // number of points in the tile
  // Points should not contain any points from bPoints or rPoints,
  // bPoints and rPoints should have one point in common
  R_POINT **points;          // 1D array of pointers to points in this tile
  R_POINT **bPoints;         // 1D array of points on bottom boundary
  R_POINT **rPoints;         // 1D array of points on right boundary
  // Counters for above arrays
  unsigned int pointsCount;
  unsigned int bPointsCount;
  unsigned int rPointsCount;
  FILE *gridFile;

} TIN_TILE;

typedef struct Tin {
  FILE *fp;               // pointer to file
  char *name;             // name of tin file
  TIN_TILE *tt;           // pointer to first tile in the list
  COORD_TYPE nrows;       // number of rows
  COORD_TYPE ncols;       // number of cols
  ELEV_TYPE nodata;       // value for nodata points
  unsigned int numTris;
  unsigned int numTiles;
  unsigned int numPoints; 
  double x;               // x lat lon corner 
  double y;               // y lat lon corner
  double cellsize;        // Cell size
  ELEV_TYPE min;
  ELEV_TYPE max;
  unsigned int tl;        // Length of the side of a tile
} TIN;


///////////////////////////////////////////////////////////////////////////////
// Declare Functions //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


//
// add trinagle to a TIN and return pointer to it
//
TRIANGLE* addTri(TIN_TILE *tt, R_POINT *p1, R_POINT *p2, R_POINT *p3, 
		 TRIANGLE *t12, TRIANGLE *t13, TRIANGLE *t23);

//
// Given a triangle and 2 pts return the neighbor triangle to that edge
//
TRIANGLE* whichTri(TRIANGLE *tn,R_POINT *pa,R_POINT *pb,TIN_TILE *tt);

//
// Which of p1-p3 is same as pa
//
R_POINT* whichPoint(R_POINT *pa, R_POINT *p1, R_POINT *p2, R_POINT *p3);

//
// Find neighbor (t) to tn and point proper edge to t.
//
void pointNeighborTriTo(TRIANGLE *t,R_POINT *pa,R_POINT *pb,TRIANGLE *tn);

//
// Print the triangles in a TIN_TILE for debugging
//
void printTinTile(TIN_TILE* tinTile);

//
// Print a tin list for debugging
//
void printTin(TIN *tin);

//
// Print the points of a triangle and pointers for debugging
//
void printTriangle(TRIANGLE* t);

//
// Print triangle coordiantes
//
void printTriangleCoords(TRIANGLE* t);

//
// Returns TRUE if is in tile otherwise it returns the direction of
// the tile it is in
//
TIN_TILE *whichTileTri(R_POINT *p1, R_POINT *p2, R_POINT *p3, TIN_TILE *tt);

//
// Is a point on the boundary of its tile?
//
short pointOnBoundary(R_POINT *p, TIN_TILE *tt);

//
// Is the point in the given tile?
//
short pointInTile(R_POINT *p, TIN_TILE *tt);

//
// Is the triangle in the given tile
//
short triangleInTile(TRIANGLE *t, TIN_TILE *tt);

//
// Does the triangle have an edge on the boundary of a given tile?
//
short triOnBoundary(R_POINT *p1, R_POINT *p2, R_POINT *p3, TIN_TILE *tt);

//
// Is an edge on the boundary?
//
short edgeOnBoundary(R_POINT *p1, R_POINT *p2, TIN_TILE *tt);

//
// This can be used to traverse the tin. Since this will cross every
// triangle 3 times, it is is important to only use next tri when the
// returned edge is of type IN
//
TRIANGLE *nextEdge(TRIANGLE *t, R_POINT *v, EDGE *edge, TIN_TILE *tt);

//
// Remove triangle from TIN_TILE and free memory
//
void removeTri(TRIANGLE* t);

//
// printPointList - given a triangle,print its point list
//
void printPointList(TRIANGLE* t);

//
// getTileLength finds the dimesions for a sub grid given a memory allocation
//
int getTileLength (double MEM);

//
// compute the signed area of a triangle, positive if the
// third point is off to the left of ab
//
int areaSign(R_POINT *a, R_POINT *b, R_POINT *c);

//
// Is point z in triangle abc?
//
int inTri2D(R_POINT *a, R_POINT *b, R_POINT *c, R_POINT *z);

//
// Validate that all points in a triangle's point list are actually in
// that triangle
//
void checkPointList(TRIANGLE* t);

//
// Find the third point given two known points
//
R_POINT *findThirdPoint(R_POINT *p1, R_POINT *p2, R_POINT *p3, R_POINT *pa, 
			R_POINT *pb);

//
// Is a point an end point of a particular triangle
//
int isEndPoint(TRIANGLE* t, R_POINT *p);

//
// Find the edge opposite to a point p
//
EDGE findOpposite(TRIANGLE* t, R_POINT *p);

//
// Is a given edge in a triangle t?
//
int edgeInTriangle(TRIANGLE* t, EDGE e);

//
// Do two edges have equal valued points?
//
int edgePointsEqual(EDGE e1, EDGE e2);

//
// Validate that at least t1 and t2 are part of t. This is used to
// debug that the triangles being created inside t are actually inside
// triangle t
//
void triangleCheck(TRIANGLE* t,TRIANGLE* t1,TRIANGLE* t2,TRIANGLE* t3);

//
// traverse through a TIN_TILE and free each triangle. Since the
// traversal visits every triangle three times, we free each triangle
// the third time we see it.
//
void deleteTinTile(TIN_TILE *tt);

///////////////////////////////////////////////////////////////////////////////
//
// Tin file handling: import and write
//
///////////////////////////////////////////////////////////////////////////////


//
// Given a path to a tin file, read in and return a full tin
// structure. This assumes that the entire TIN can fit into memory
//
TIN *readTinFile(char *path);

// 
// Read the header info from a tin file which is need to create the
// TIN structure
//
TIN *readTinFileHeader(char *path);

//
// Read in the next tile from the already open tin file (tin->fp) and
// return null if eof
//
TIN_TILE *readNextTile(TIN *tin);

//
// Get the index of a given point. First try the three previously
// checked points and then Binary search the sorted points array to
// find the index of the point. This assumes the array is sorted and
// each point is unique!
//
unsigned int getPointsIndex(R_POINT *p,TIN_TILE *tt,unsigned int p1,
			    unsigned int p2, unsigned int p3);

//
// Write tile to a file
//
void writeTinTile(TIN_TILE *tt, char *path, short freeTriangles);

//
// write tin file to a given filename, if headerOnly is set to one
// then don't wrtie the tile just write the header.
//
void writeTin(TIN *tin,char *path,short headerOnly);


#endif
