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
 * grass.h has functions that handle the output of sites and vectors
 * and the input of rasters to both grids and tiled grids
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-11
 *
 * COMMENTS:
 *
 *****************************************************************************/

#ifndef __grass_h
#define __grass_h

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <grass/gis.h>
#include <grass/glocale.h>

//#include <site.h>
//#include <Vect.h>

#include "refine_tin.h"


/* write the header for a sites file */
void writeSitesHeader(FILE *sitesFile, char *siteFileName);

/* write a tile to a sites file */
void writeSitesTile(TIN_TILE *tt,FILE *sitesFile, char* sitesFileName);

/* GRASS function for writing a default file header */
static void _set_default_head_info (struct dig_head *head);

/* GRASS function for writing a default file header */
void set_default_head_info (struct dig_head *head);

/* write a tile to a vector file */
void writeVectorTile(struct Map_info *Map, TIN_TILE *tt);

/* read a raster into a grid */
GRID* raster2grid(char* gridname, int nrows, int ncols);

/* read a raster into a tiled grid */
TILED_GRID* raster2tiledGrid(char* gridname, int nrows, int ncols,
			     unsigned int TL);


#endif /* __grass_h */
