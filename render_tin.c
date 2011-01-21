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
 * render_tin.c render a TIN structure to an Open GL window
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: We need to use a lot of global with Open GL since there
 * is no way to pass variables into the display function
 *
 *****************************************************************************/

#include "render_tin.h"

// Globals
GLfloat red[3] = {1.0, 0.0, 0.0};
GLfloat green[3] = {0.0, 1.0, 0.0};
GLfloat blue[3] = {0.0, 0.0, 1.0};
GLfloat black[3] = {0.0, 0.0, 0.0};
GLfloat white[3] = {1.0, 1.0, 1.0};
GLfloat gray[3] = {0.5, 0.5, 0.5};
GLfloat yellow[3] = {1.0, 1.0, 0.0};
GLfloat magenta[3] = {1.0, 0.0, 1.0};
GLfloat cyan[3] = {0.0, 1.0, 1.0};

GLint fillmode = 0;


int colorStyle = 0; // Specifies the color to display
float changeFactor = 0.1;
float maxZ = 0.0;

int displayValid = 0;

GRID *gridGlobal;
TIN *tinGlobal;

double delta_x, delta_y, scaleFactor; 

//
// Initialize the camera view
//
void OGL_init_view() {
 
  /* camera is at (0,0,0) looking along negative y axis */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, 1 /* aspect */, 0.0001, 20.0); 
  /* the frustrum is from z=-1 to z=-10 */ 

  glMatrixMode(GL_MODELVIEW);
  if(1) {
    /* set up a different initial viewpoint */
    glLoadIdentity();
    gluLookAt(0, -1, 1,/* eye */  0, 0, 0,/* center */ 0, 0, 1);/* up */
  }
  if(0)glTranslatef(0,0,-5);
  glRotatef(5.0 * 9.0, 1, 0, 0);
  glTranslatef(0,0, -changeFactor * 4.0);

}


//
// Initialize the Open GL window
//
void OGL_init(int *argc, char** argv){
   // open a window
  glutInit(argc,argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(500, 500);
  glutInitWindowPosition(100,100);
  glutCreateWindow("tin");

  // OpenGL init
  glClearColor(0, 0, 0, 0); // set background color black
    
  // callback functions
  glutDisplayFunc(display); 
  glutIdleFunc(idle); 
  glutKeyboardFunc(keypress);

  // creat menu
  glutCreateMenu(mainMenu);
  glutAddMenuEntry("Fill/Outline", 1);
  glutAddMenuEntry("Change Color", 2);
  glutAddMenuEntry("Quit", 3);
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  delta_x = 0;
  delta_y = 0; 
  scaleFactor = 1;

  OGL_init_view(); 
}


//
// run display loop
//
void OGL_render(int argc, char** argv){
  // event handler
  glutMainLoop();

}


// 
// map elevations to values to specified level h, this allows the user
// to control elevation display so that the terrain could appear 2D or
// 3D
//
float z_map(int h){
  if(h == tinGlobal->nodata)
    return 0;
  else
    return (maxZ/(tinGlobal->max - tinGlobal->min)) * (h - tinGlobal->min);
}


//
// Traverse a tin_tile rendering each triangle
//
void drawTinTile(TIN_TILE *tt){
  double interval,x0,y0;
  
  // Decide whether to fill the triangles
  if (fillmode) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  } 
  else {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }
  

  // set x1 and y1 to the upper left corner of the data
  // rows > cols
  if(tinGlobal->nrows > tinGlobal->ncols){
    interval = 2.0/ (tinGlobal->nrows-1);
    // from JT's code
    x0 = -((interval*(tinGlobal->ncols-1))/2.0);
    y0 = 1;
    /* printf("interval %f, nrows %d, ncols %d\n", */
/* 	   interval, tinGlobal->nrows, tinGlobal->ncols); */
  }
   // cols > rows
  else{
    interval = 2.0/(tinGlobal->ncols-1);
    // from JT's
    x0 = -1;
    y0 = ((interval*(tinGlobal->nrows-1))/2.0);
    

    
  }
  
  // loop through the data and draw the triangles in the tin
  //printf("interval %f, X0 %f, Y0 %f\n", interval, x0, y0);
 
  // if the tin only has two init tris don't print it
  if(1){
    
    TRIANGLE *curT = tt->t;
    TRIANGLE *prevT = curT;
    
    // Create an edge which 
    EDGE curE;
    curE.type = IN;
    curE.t1 = NULL;
    curE.t2 = tt->t;
    curE.p1 = tt->e.p1;
    curE.p2 = tt->e.p2;
    
    // Traverse the tin and draw triangles till we get back to the start edge
    do {
      // Print triangle if it we are on an IN edge
      if(curE.type == IN && prevT != NULL){
	// Don't print tri if it has a nodata corner point
	if(prevT->p1->z != tt->nodata &&
	   prevT->p2->z != tt->nodata &&
	   prevT->p3->z != tt->nodata &&
	   prevT->p1->z != tt->min-1 &&
	   prevT->p2->z != tt->min-1 &&
	   prevT->p3->z != tt->min-1){
	//if(1){
	  
       	  // Draw the triangle
	  glBegin(GL_POLYGON);
	  
	  

	    colorMap(prevT->p1->z);
	  glVertex3f(((double) prevT->p1->y)*interval+x0,
		     y0 - ((double)prevT->p1->x)*interval,
		     z_map(prevT->p1->z));
	  
	  colorMap(prevT->p2->z);
	  glVertex3f(((double) prevT->p2->y)*interval+x0,
		     y0 - ((double) prevT->p2->x)*interval,
		     z_map(prevT->p2->z));

	  colorMap(prevT->p3->z);
	  glVertex3f(((double) prevT->p3->y)*interval+x0,
		     y0 - ((double) prevT->p3->x)*interval,
		     z_map(prevT->p3->z));
	  
	  glEnd();
	
	}
      }
      // Go to next edge
      prevT = curT;
      curT = nextEdge(curT,tt->v,&curE,tt);
      assert(curT);
      
    } while((curE.p1 != tt->e.p1 || curE.p2 != tt->e.p2) && 
	    (curE.p1 != tt->e.p2 || curE.p2 != tt->e.p1));
  }
}


//
// set the color based on a given height
//
void colorMap(int h){
  double c;
  c = (1.0/(tinGlobal->max - tinGlobal->min))*(h-tinGlobal->min);
  if(c < (1.0/5))
      glColor3f(1-(5*c),1,0);
    if(c < 2.0/5 && c >= 1.0/5)
      glColor3f(0,1,(c-(1.0/5))*5);
    if(c < 3.0/5 && c >= 2.0/5)
      glColor3f(0,((3.0/5)-c)*5,1);
    if(c < 4.0/5 && c >= 3.0/5)
      glColor3f((c-(3.0/5))*5,0,1);
    if(c <= 5.0/5 && c >= 4.0/5)
      glColor3f(1,0,(1-c)*5);
  
}


//
// Loop through the tiles in a tin and output each one
//
void drawTin(TIN_TILE *tt){

  //Skip dummy head
  tt = tt->next;
  while(tt->next != NULL){
    drawTinTile(tt);
    tt = tt->next;
  }
}


//
// handle idle window
//
void idle(void) {
  if(!displayValid) {
    glutPostRedisplay();
    displayValid = 1;
  }
}


//
// display - controls what is drawn to the screen
//
void display(void) {
  // clear window
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Draw tin
  drawTin(tinGlobal->tt);
  
  // execute the drawing commands 
  glFlush();
}


//
// handle keypress events
//
void keypress(unsigned char key, int x, int y) {
  switch(key) {

  case 'i':// reset to start view 
    OGL_init_view();
    glutPostRedisplay();
    break;
  case 'x':
    glRotatef(5.0, 1, 0, 0);
    glutPostRedisplay();
    break;
  case 'y':
    glRotatef(5.0, 0, 1, 0);
    glutPostRedisplay();
    break;
  case 'z':
    glRotatef(5.0, 0, 0, 1);
    glutPostRedisplay();
    break;
  case 'X':
    glRotatef(-5.0, 1, 0, 0);
    glutPostRedisplay();
    break;
  case 'Y':
    glRotatef(-5.0, 0, 1, 0);
    glutPostRedisplay();
    break;
  case 'Z':
    glRotatef(-5.0, 0, 0, 1);
    glutPostRedisplay();
    break;

  case 'u':
    glTranslatef(0,changeFactor, 0);
    glutPostRedisplay();
    break;

 case 'd':
    glTranslatef(0,-changeFactor, 0);
    glutPostRedisplay();
    break;

 case 'l':
    glTranslatef(-changeFactor, 0,0);
    glutPostRedisplay();
    break;


 case 'r':
    glTranslatef(changeFactor,0,0);
    glutPostRedisplay();
    break;

  case 'b':
    glTranslatef(0,0, -changeFactor);
    glutPostRedisplay();
    break;
  case 'f':
    glTranslatef(0,0, changeFactor);
    glutPostRedisplay();
    break;
  case '>':
    maxZ += 0.1;
    glutPostRedisplay();
    break;
  case '<':
    maxZ -= 0.1;
    glutPostRedisplay();
    break;
  case 'q':
    exit(0);
    break;
  } 
}


//
//  creates a menu
//
void mainMenu(int value)
{
  switch (value){
  case 1: 
    /* toggle outline/fill */
    fillmode = !fillmode;
    break;
  case 2: 
    // Change color style
    colorStyle = !colorStyle;
    break;
  case 3: 
    exit(0);
  }
  glutPostRedisplay();
}

