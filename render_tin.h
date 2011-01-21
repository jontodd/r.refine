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
 * render_tin.h render a TIN structure to an Open GL window
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: We need to use a lot of global with Open GL since there
 * is no way to pass variables into the display function
 *
 *****************************************************************************/

#ifndef __render_tin_h
#define __render_tin_h

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "refine_tin.h"

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


//
// Initialize the camera view
//
void OGL_init_view();

//
// Initialize the Open GL window
//
void OGL_init(int *argc, char** argv);

//
// run display loop
//
void OGL_render(int argc, char** argv);

// 
// map elevations to values to specified level h, this allows the user
// to control elevation display so that the terrain could appear 2D or
// 3D
//
float z_map(int h);

//
// Traverse a tin_tile rendering each triangle
//
void drawTinTile(TIN_TILE *tt);

//
// set the color based on a given height
//
void colorMap(int h);

//
// Loop through the tiles in a tin and output each one
//
void drawTin(TIN_TILE *tt);

//
// handle idle window
//
void idle(void);

//
// display - controls what is drawn to the screen
//
void display(void);

//
// handle keypress events
//
void keypress(unsigned char key, int x, int y);

//
//  creates a menu
//
void mainMenu(int value);

#endif //__render_tin_h
