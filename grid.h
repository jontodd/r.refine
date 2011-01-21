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
 * grid.h contains functions for reading in and outputing arc-ascii
 * grids (rasters)
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#ifndef __grid_h
#define __grid_h

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "point.h"

//
// grid structure
//
typedef struct grid_t {
  char*name;      // File name (path)
  ELEV_TYPE **data;    // Data
  unsigned long ncols;  // Number of columns
  unsigned long nrows;  // Number of rows
  double x;        // x lat lon corner 
  double y;        // y lat lon corner
  double cellsize;   // Cell size
  ELEV_TYPE nodata;     // No data value
  ELEV_TYPE max;        // Max elevation
  ELEV_TYPE min;        // Min elevation
} GRID;

//
// tiled grid structure with file pointers instead of data in memory
//
typedef struct tiled_grid {
  char*name;      // File name (path)
  FILE ***files;    // Tiled set of files
  unsigned long ncols;  // Number of columns
  unsigned long nrows;  // Number of rows
  double x;        // x lat lon corner 
  double y;        // y lat lon corner
  double cellsize;   // Cell size
  ELEV_TYPE nodata;     // No data value
  ELEV_TYPE max;        // Max elevation
  ELEV_TYPE min;        // Min elevation
  unsigned int TL;
} TILED_GRID;


//
// Write an elevation value to a file
//
void writeElevToTile(FILE *fp,ELEV_TYPE z);

//
// Read a arc-ascii grid file into a set of tile files. This way we
// don't read the data into memory but instead seperate it into tile
// which we can work on one by one
//
TILED_GRID *readGrid2Tile(char *path, unsigned int TL);

//
// bring grid file into an array
//
GRID *importGrid(char *path);

//
// write grid structure to file path
//
void writeGrid(GRID *g,char *path);

//
// Print the grid info 
//
void printGrid(GRID *g);

//
// free memory from grid
//
void freeGrid(GRID *g);


#endif
