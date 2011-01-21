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
 * point.h declares point type and its components 
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#ifndef POINT_H
#define POINT_H

#include <limits.h>
#include <float.h>

// Definition of coordinate type for x,y values. This can be changed
// as required by the data
//
typedef short COORD_TYPE;

#define COORD_TYPE_MAX SHRT_MAX
#define COORD_TYPE_MIN SHRT_MIN
#define COORD_TYPE_PRINT_CHAR "%hd"

// Definition of elevation type for z values. This can be changed as
// required by the data
typedef short ELEV_TYPE;

#define ELEV_TYPE_MAX SHRT_MAX  // Fix me
#define ELEV_TYPE_MIN SHRT_MIN // Fix me
#define ELEV_TYPE_PRINT_CHAR "%hd"

// Point structurexs
typedef struct Point {
  COORD_TYPE x,y;
  ELEV_TYPE z;
} R_POINT;


#endif
