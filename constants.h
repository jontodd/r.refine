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
 * constants.h defines constants used in many of the files
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#ifndef __constants_h
#define __constants_h


#define X 0
#define Y 1
#define Z 2

#define FALSE 0
#define TRUE 1

#define DIR_TOP 2
#define DIR_BOTTOM 3
#define DIR_LEFT 4
#define DIR_RIGHT 5

#define IN 1
#define INBACK 3
#define OUT 2
#define OUTBACK 4

#define EDGE12 0
#define EDGE13 1
#define EDGE23 2

#define EPSILON 0.0001 // small double for vertex closeness 

// This is a spcial point which is used only for corners which have no
// data values
extern R_POINT NODATA_CORNER;

#define ABS(x) ( (x) < 0 ? -(x) : (x) )
#define MIN(x,y) ( x < y ? x : y )
#define MAX(x,y) ( x < y ? y : x )
#define MODULUS(p) (sqrt((p).x * (p).x + (p).y * (p).y + (p).z * (p).z) )
#define SIGN(x) ( x < 0 ? -1 : 1 )
#define DOTPRODUCT(v1,v2) ( v1.x*v2.x + v1.y*v2.y + v1.z*v2.z )
#define INVERTVECTOR(p1) p1.x = -p1.x; p1.y = -p1.y; p1.z = -p1.z






#endif
