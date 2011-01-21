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
 * refine_tin.c can take a tiled grid file and create a refined tin.
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-11
 *
 * COMMENTS:
 *
 *****************************************************************************/

#include "refine_tin.h"

#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <strings.h>

#include "tin.h"

#ifdef __GRASS__
#include "grass.h"
#endif

extern TIN *tinGlobal;

#define DEBUG if(0)

//enables printing points that are refined
//#define REFINE_DEBUG 

// This is a special point which will be used to mark that the max
// error for a given triangle is less than e and thus it is 'done'
extern R_POINT *DONE;

//
// Initialize TIN structure, returns a pointer to lower left tri. This
// will not initialize the points in the triangles, just the two
// staring triangles. Points will be added later in refinement.
//
TIN_TILE* initTinTile(TIN_TILE *tt,TIN_TILE *leftTile, TIN_TILE *topTile,
		      TRIANGLE *leftTri, TRIANGLE *topTri, 
		      R_POINT *nw, R_POINT *sw, R_POINT *ne, int iOffset, 
		      int jOffset, short useNodata, TIN *tin){

  // Create a pointer to the lower left tri in the tin
  TRIANGLE *first, *second;

  // Create a PQ of the error of the triangles. We want to give the PQ
  // an initial size which is the lowest power of 2 which will fit all
  // the triangles in a tile (3 * numPoints in tile) = (3 * tin->tl *tin->tl)
  //
  // If TL is larger than 4000 then we are probably running untiled
  // and we should start the pq small and just let it grow as needed
  unsigned int initPQSize = 1048576;
  if(tin->tl < 4000)
    initPQSize = (unsigned int)	pow(2,ceil(log10(3 * tin->tl * tin->tl)
					   /log10(2)));
  tt->pq = PQ_initialize( initPQSize );


  // Set Offset
  tt->iOffset = iOffset;
  tt->jOffset = jOffset;
  
  // Set neighbors
  tt->top = topTile;
  tt->left = leftTile;

  // Number of triangles and points
  tt->numTris = 2;
  tt->numPoints = 4;
  
  // Point neighbors to me
  pointNeighborTileTo(tt,DIR_BOTTOM,topTile);
  pointNeighborTileTo(tt,DIR_RIGHT,leftTile);

  // We need to create at least 1 and up to 4 corner points for the
  // intial triangulation. They have incorrect z value for now and
  // will be updated later
  if(nw == NULL){
    nw = (R_POINT*)malloc(sizeof(R_POINT));
    nw->x=iOffset;
    nw->y=jOffset;
    nw->z=0;
  }
  if(ne == NULL){
    ne = (R_POINT*)malloc(sizeof(R_POINT));
    ne->x=iOffset;
    ne->y=tt->ncols-1+jOffset;
    ne->z=0;
  }
  if(sw == NULL){
    sw = (R_POINT*)malloc(sizeof(R_POINT));
    sw->x=tt->nrows-1+iOffset;
    sw->y=jOffset;
    sw->z=0;
  }
  
  R_POINT *se = (R_POINT*)malloc(sizeof(R_POINT));
  se->x=tt->nrows-1+iOffset;
  se->y=tt->ncols-1+jOffset;
  se->z=0;

  // Store the 4 corner points for later reference
  tt->nw = nw;
  tt->ne = ne;
  tt->sw = sw;
  tt->se = se;

  assert(pointOnBoundary(nw,tt) && pointOnBoundary(ne,tt) &&
	 pointOnBoundary(sw,tt) && pointOnBoundary(se,tt) );


  // Create the first two triangles. 
  tt->t = first = addTri(tt,nw,sw,se,leftTri,NULL,NULL);
  second = addTri(tt,nw,ne,se,topTri,first,NULL);
  assert(tt->t);
  assert(first);
  assert(second);

  // Point the tins lower left corner to sw
  tt->v = sw;
  
  // Lower left edge nw-sw gets stored in tin
  tt->e.t1 = NULL;
  tt->e.t2 = NULL;
  tt->e.p1 = nw;
  tt->e.p2 = sw;
  tt->e.type = IN;

  return tt;
}


//
// Find neighbor tile to tt and point ttn to it. If tt is under ttn
// then dir would be bottom
//
void pointNeighborTileTo(TIN_TILE *tt, short dir, TIN_TILE *ttn){
  assert(tt);
  if(ttn != NULL){
    if(dir == DIR_BOTTOM)
      ttn->bottom = tt;
    else if(dir == DIR_RIGHT )
      ttn->right = tt;
    else if (dir == DIR_TOP)
      ttn->top = tt;
    else{
      assert(dir == DIR_LEFT);
      ttn->left = tt;
    }
  }
}


//
// Take in the big grid, and splits into tiles, initialize all tiles
// and return tin
//
TIN *initTin(TILED_GRID *fullGrid, double e, double mem, short useNodata,
	     char *name){
  // Create the TIN
  TIN *tin = (TIN*)malloc(sizeof(TIN));
  tin->nrows = fullGrid->nrows;
  tin->ncols = fullGrid->ncols;
  tin->nodata = fullGrid->nodata;
  tin->min = fullGrid->min;
  tin->max = fullGrid->max;
  tin->x = fullGrid->x;
  tin->y = fullGrid->y;
  tin->cellsize = fullGrid->cellsize;
  tin->numTris = 0;
  tin->numPoints = 0;
  tin->name = name;

  // create a dummy head and tail for the TIN_TILE list
  TIN_TILE *head = (TIN_TILE*)malloc(sizeof(TIN_TILE));
  assert(head);
  TIN_TILE *dummytail = (TIN_TILE*)malloc(sizeof(TIN_TILE));
  assert(dummytail);
  TIN_TILE *curTT = head;
  head->next = dummytail;
  dummytail->next = NULL;
  tin->tt = head;
  


  int TL = fullGrid->TL; // compute length of tile for mem
  tin->tl = TL;

  int jNumTiles, iNumTiles, i, j;
  
  
  // Find number of tiles needed in i and j direction
  jNumTiles = ceil( ((double) tin->ncols)/ ((double) TL-1));
  iNumTiles = ceil( ((double) tin->nrows)/ ((double) TL-1));

  tin->numTiles = jNumTiles * iNumTiles;

  // In order to link the triangles together in the global TIN
  // structure we will need to know for any given tile, it's left
  // neighbor and it's top neighbor. We will need an array for the top
  // neighbors
  TIN_TILE *leftNeighbor = NULL;
  TIN_TILE **topNeighbor = (TIN_TILE**)malloc(jNumTiles*sizeof(TIN_TILE*));
  assert(topNeighbor); 

  // Initialize topNeighbor array
  for(j = 0; j < jNumTiles; j++)
    topNeighbor[j] = NULL;


  for (i = 0; i < iNumTiles; i++){

    for (j = 0; j < jNumTiles; j++){

      int startI = (TL-1)*i;
      int startJ = (TL-1)*j;

       TIN_TILE *tt = (TIN_TILE*)malloc(sizeof(TIN_TILE));
       assert(tt);

      // Determine the number of row and cols. Make sure we don't go
      // off the big grid's edges
      if((startI + TL) > fullGrid->nrows)
	tt->nrows = fullGrid->nrows - startI;
      else
	tt->nrows = TL;
      if((startJ + TL) > fullGrid->ncols)
	tt->ncols = fullGrid->ncols - startJ;
      else 
	tt->ncols = TL;
 
      tt->min = fullGrid->min;
      tt->max = fullGrid->max;
      tt->nodata = fullGrid->nodata;
      tt->gridFile = fullGrid->files[i][j];

      // We need to pass some info to initTinTile so that it knows      
      // about it's neighbor triangles and shared corner points. This
      // can be done without conditionals because every tin is
      // initialize in the same way so these points and triangles are
      // in predictable locations. If changes are made to initTinTile
      // these assumptions could change!
             
      TRIANGLE *leftTri = NULL;
      TRIANGLE *topTri = NULL;
      TIN_TILE *left = NULL;
      TIN_TILE *top = NULL;
      R_POINT *nw = NULL;
      R_POINT *sw = NULL;
      R_POINT *ne = NULL;
      
      
      // - The right top tri is tt->t->p1p3
      // - The sw point of this triangle is the se point of its left 
      //   tri which is p3
      // - Then nw is the ne point of the left neighbor which is point p2
      if(leftNeighbor != NULL){
	left = leftNeighbor;
	leftTri = leftNeighbor->t->p1p3;
	sw = leftNeighbor->t->p1p3->p3;
	nw = leftNeighbor->t->p1p3->p2;
      }    
      
      // - The bottom left triangle of any TIN_TILE tt is tt->t
      // - The nw point of this tri is the sw point of its top neighbor
      //   which is v (lower left corner)
      // - The ne point of this tri is the se point of it's top neighbor 
      //   which is p3
      if(topNeighbor[j] != NULL){
	top =  topNeighbor[j];
	topTri = topNeighbor[j]->t;
	nw = topNeighbor[j]->v;
	ne = topNeighbor[j]->t->p3;
      }

      // Initialize the TIN_TILE with two triangles. initTin will
      // return the lower left triangle.
      //tt = initTinTile(gridTile,e,left,top,NULL,NULL,
      //nw,sw,ne,(TL-1)*i,(TL-1)*j);
      tt = initTinTile(tt,left,top,leftTri,topTri,
		       nw,sw,ne,(TL-1)*i,(TL-1)*j,useNodata,tin);

      // link the list
      tt->next = curTT->next;
      curTT->next = tt;
      curTT = tt;
      
      // This is the new left neighbor
      leftNeighbor = tt;
      topNeighbor[j] = tt;
    }
    leftNeighbor = NULL;
  }

  return tin;
}


//
// Add the points two the two initial triangles of a Tin tile from
// file. Also add points from neighbor boundary arrays to the
// triangulation to have boundary consistancy
//
TIN_TILE *initTilePoints(TIN_TILE *tt, double e, short useNodata){

  // First two tris
  TRIANGLE *first = tt->t;
  TRIANGLE *second = tt->t->p1p3;

  // Get back to the beginning of the tile data file
  rewind(tt->gridFile);

  // Now build list of points in the triangle
  // Create a dummy tail for both point lists
  first->points = Q_init();
  second->points = Q_init();
  first->maxE = DONE;
  second->maxE = DONE;
  
  // Build the two point lists
  register int row, col;
  ELEV_TYPE maxE_first=0;
  ELEV_TYPE maxE_second=0;
  ELEV_TYPE tempE = 0;
  
  R_POINT temp;
  
  // iterate through all points and distribute them to the 
  // two triangles
  for(row=0;row<tt->nrows;row++) {
    temp.x=row+tt->iOffset;
    for(col=0;col<tt->ncols;col++) {
      temp.y=col+tt->jOffset;
      fread(&temp.z,sizeof(ELEV_TYPE), 1, tt->gridFile);
      // Only set Z values for corner points since they already exist
      if(row==0 && col==0){
	tt->nw->z = temp.z;
	continue;
      }
      if(row==0 && col==tt->ncols-1){
	tt->ne->z = temp.z;
	continue;
      }
      if(row==tt->nrows-1 && col==tt->ncols-1){
	tt->se->z = temp.z;
	continue;
      } 
      if(row==tt->nrows-1 && col==0){
	tt->sw->z = temp.z;
	continue;
      }	
      //Ignore edge points if internal tile
      if(tt->iOffset != 0 && row == 0)
	continue;
      if(tt->jOffset != 0 && col == 0)
	continue;
      
      //Skip nodata or change it to min-1
      if(temp.z == tt->nodata){
	if(!useNodata)
      	  continue;
	else
	  temp.z = tt->min-1;
      }

      // Add to the first triangle's list
      if(inTri2D(first->p1, first->p2, first->p3, &temp)) {
	
	Q_insert_elem_head(first->points, temp);

	//Update max error
	tempE = findError(temp.x,temp.y,temp.z,first);
	if (tempE > maxE_first) {
	  maxE_first = tempE;
	  assert(Q_first(first->points));
	  // store pointer to triangle w/ max err
	  first->maxE = &Q_first(first->points)->e;
	  first->maxErrorValue = tempE;
	}
      }
      // Add to the second triangle's list
      else {

	assert(inTri2D(second->p1, second->p2, second->p3, &temp));

	Q_insert_elem_head(second->points, temp);

	//Update max error
	tempE = findError(temp.x,temp.y,temp.z,second);
	if (tempE > maxE_second) {
	  maxE_second = tempE;
	  assert(Q_first(second->points));
	  // store pointer to triangle w/ max err
	  second->maxE = &Q_first(second->points)->e; 
	  second->maxErrorValue = tempE;
	}

      }

    }//for col
  }//for row
  //end distribute points among initial triangles

  DEBUG {checkPointList(first); checkPointList(second);}

  // First triangle has no points with error > e, mark as done
  if (first->maxE == DONE){
    Q_free_queue(first->points);
    first->points = NULL;
    first->maxErrorValue = 0;
  }
  // Insert max error point into the PQ
  else
    PQ_insert(tt->pq,first);

  // Second triangle has no points with error > e, mark as done
  if (second->maxE == DONE){
    Q_free_queue(second->points);
    second->points = NULL;
    second->maxErrorValue = 0;
  }
  // Insert max error point into the PQ
  else
    PQ_insert(tt->pq,second);


  // Initialize point pointer arrays
  // At most points can have (tl*tl)-2*tl points in it
  // At most bPoints and rPoints can have tl points
  tt->points = (R_POINT **)malloc( ((tt->nrows * tt->ncols) - 
				  (tt->nrows + tt->ncols)) * 
				 sizeof(R_POINT*));
  tt->bPoints = (R_POINT **)malloc( tt->ncols * sizeof(R_POINT*));
  tt->rPoints = (R_POINT **)malloc( tt->nrows * sizeof(R_POINT*));  

  // Add points to point pointer array
  tt->points[0]=tt->nw;//nw
  tt->bPoints[0]=tt->sw;//sw
  tt->bPoints[1]=tt->se;//se
  tt->rPoints[0]=tt->ne;//ne
  tt->rPoints[1]=tt->se;//se
  tt->pointsCount = 1;
  tt->bPointsCount = 2;
  tt->rPointsCount = 2;

  //
  // Add boundary points to the triangulation
  //
  int i;
  COORD_TYPE prevX = 0,prevY = 0;
  TRIANGLE *t1,*t2, *s, *sp;
  s = tt->t;
  sp = tt->t->p1p3;

  if(tt->left != NULL){
    // The first & last point in this array are corner points for this
    // tile so we ignore them
 
    for(i = 1; i < tt->left->rPointsCount-1; i++){
      assert(s && tt->pq && tt->left->rPoints[i]);
      assert(prevX <=  tt->left->rPoints[i]->x &&
	     prevY <=  tt->left->rPoints[i]->y);
      assert(tt->left->rPoints[i] != tt->nw &&
	     tt->left->rPoints[i] != tt->ne &&
	     tt->left->rPoints[i] != tt->sw &&
	     tt->left->rPoints[i] != tt->se);
      
      // add 2 tris in s
      t1 = addTri(tt, s->p1, tt->left->rPoints[i], s->p3,
		  NULL,whichTri(s,s->p1,s->p3,tt),NULL);
      assert(t1);
      t2 = addTri(tt, tt->left->rPoints[i], s->p2, s->p3,
		  NULL,t1,whichTri(s,s->p2,s->p3,tt));
      assert(t2);
      // Verify that t1 and t2 are really inside s
      triangleCheck(s,t1,t2,NULL);
      // Distribute points in the 2 triangles
      if(s->maxE != DONE){
	s->p1p2 = s->p1p3 = s->p2p3 = NULL;
	distrPoints(t1,t2,NULL,s,NULL,e,tt);
	//Mark triangle s for deletion from pq before distrpoints so we
	//can include its maxE in the newly created triangle
	
	PQ_delete(tt->pq,s->pqIndex);
      }
      else{
	// Since distrpoints normally fixes corner we need to do it here
	if(s == tt->t){
	  updateTinTileCorner(tt,t1,t2,NULL);
	}
	t1->maxE = t2->maxE = DONE;
	t1->points = t2->points = NULL;
      }
      
      
      removeTri(s);
      tt->numTris++;
      tt->numPoints++;
      // Now split the next lowest boundary triangle
      s = t2;
      
      // Should we enforce Delaunay here?

      prevX = tt->left->rPoints[i]->x;
      prevY = tt->left->rPoints[i]->y;
    }
    
  }
  
  if(tt->top != NULL){
    // The first & last point in this array are corner points for this
    // tile so we ignore them
    s = sp;
    prevX = prevY = 0;
    for(i = 1; i < tt->top->bPointsCount-1; i++){
      assert(s && tt->pq && tt->top->bPoints[i]);
      assert(prevX <=  tt->top->bPoints[i]->x &&
	     prevY <=  tt->top->bPoints[i]->y);
      
      // add 2 tris in s
      t1 = addTri(tt, s->p1, tt->top->bPoints[i], s->p3,
		  NULL,whichTri(s,s->p1,s->p3,tt),NULL);
      assert(t1);
      t2 = addTri(tt, tt->top->bPoints[i], s->p2, s->p3,
		  NULL,t1,whichTri(s,s->p2,s->p3,tt));
      assert(t2);
      // Verify that t1 and t2 are really inside s
      triangleCheck(s,t1,t2,NULL);
      // Distribute points in the 2 triangles
      if(s->maxE != DONE){
	distrPoints(t1,t2,NULL,s,NULL,e,tt);
	//Mark triangle sp for deletion from pq before distrpoints so we
	//can include its maxE in the newly created triangle
	s->p1p2 = s->p1p3 = s->p2p3 = NULL;
	PQ_delete(tt->pq,s->pqIndex);

      }
      else{
	// Since distrpoints normally fixes corner we need to do it here
	if(s == tt->t){
	  updateTinTileCorner(tt,t1,t2,NULL);
	}
	t1->maxE = t2->maxE = DONE;
	t1->points = t2->points = NULL;
      }
      
      
      removeTri(s);
      tt->numTris++;
      tt->numPoints++;
      // Now split the next lowest boundary triangle
      s = t2;
      
      // Should we enforce Delaunay here?
      
      prevX = tt->top->bPoints[i]->x;
      prevY = tt->top->bPoints[i]->y;
    }
    
  }
  return tt;
}


//
//   Return TRUE if a point (xp,yp) is inside the circumcircle made up
//   of the points (x1,y1), (x2,y2), (x3,y3)
//   The circumcircle centre is returned in (xc,yc) and the radius r
//   NOTE: A point on the edge is inside the circumcircle
//
int CircumCircle(double xp,double yp,double x1,double y1,
		 double x2,double y2,double x3,double y3){
   
  double m1,m2,mx1,mx2,my1,my2;
  double dx,dy,rsqr,drsqr;
  double xc,yc; //r
  
  /* Check for coincident points */
  if (ABS(y1-y2) < EPSILON && ABS(y2-y3) < EPSILON)
    return(FALSE);
  
  if (ABS(y2-y1) < EPSILON) {
    m2 = - (x3-x2) / (y3-y2);
    mx2 = (x2 + x3) / 2.0;
    my2 = (y2 + y3) / 2.0;
    xc = (x2 + x1) / 2.0;
    yc = m2 * (xc - mx2) + my2;
  } 
  else if (ABS(y3-y2) < EPSILON) {
    m1 = - (x2-x1) / (y2-y1);
    mx1 = (x1 + x2) / 2.0;
    my1 = (y1 + y2) / 2.0;
    xc = (x3 + x2) / 2.0;
    yc = m1 * (xc - mx1) + my1;
  } 
  else {
    m1 = - (x2-x1) / (y2-y1);
    m2 = - (x3-x2) / (y3-y2);
    mx1 = (x1 + x2) / 2.0;
    mx2 = (x2 + x3) / 2.0;
    my1 = (y1 + y2) / 2.0;
    my2 = (y2 + y3) / 2.0;
    xc = (m1 * mx1 - m2 * mx2 + my2 - my1) / (m1 - m2);
    yc = m1 * (xc - mx1) + my1;
  }
  
  dx = x2 - xc;
  dy = y2 - yc;
  rsqr = dx*dx + dy*dy;
  //*r = sqrt(rsqr);
  
  dx = xp - xc;
  dy = yp - yc;
  drsqr = dx*dx + dy*dy;
  
  //return((drsqr <= rsqr) ? TRUE : FALSE);
  // Suggested
  return((drsqr <= rsqr + EPSILON) ? TRUE : FALSE);

}


//
// Swap the common edge between two triangles t1 and t2. The common
// edge should always be edge ac, abc are part of t1 and acd are part
// of t2.
//
void edgeSwap(TRIANGLE *t1, TRIANGLE *t2, 
	      R_POINT *a, R_POINT *b, R_POINT *c, R_POINT *d,
	      double e, TIN_TILE *tt){

  // Common edge must be ac
  assert(isEndPoint(t1,a) && isEndPoint(t1,b) && isEndPoint(t1,c) && 
	 isEndPoint(t2,a) && isEndPoint(t2,c) && isEndPoint(t2,d));

  assert(a != b && a != c && a != d && b != c && b != d && c != d);

  assert(t1 != t2);
  
  // Add the two new triangles with the swapped edge 
  TRIANGLE *tn1, *tn2;
  tn1 = addTri(tt,a, b, d,whichTri(t1,a,b,tt),whichTri(t2,a,d,tt),NULL);
  tn2 = addTri(tt,c, b, d,whichTri(t1,c,b,tt),whichTri(t2,c,d,tt),tn1);
  
  assert(isEndPoint(tn1,a) && isEndPoint(tn1,b) && isEndPoint(tn1,d) && 
	 isEndPoint(tn2,b) && isEndPoint(tn2,c) && isEndPoint(tn2,d));

  assert(tn1->p2p3 == tn2 && tn2->p2p3 == tn1);

 
  if(tn1->p1p2 != NULL)
    assert(whichTri(tn1->p1p2,a,b,tt) == tn1);
  if(tn1->p1p3 != NULL)
    assert(whichTri(tn1->p1p3,a,d,tt) == tn1);
  if(tn2->p1p2 != NULL)
    assert(whichTri(tn2->p1p2,c,b,tt) == tn2);
  if(tn2->p1p3 != NULL)
    assert(whichTri(tn2->p1p3,c,d,tt) == tn2);

  // Debug
  DEBUG{
  printf("EdgeSwap: \n");
  printTriangle(t1);
  printTriangle(t2);
  printTriangle(tn1);
  printTriangle(tn2);
  } 

  // Distribute point list from t1 and t2 to tn1 and tn2. Distribute
  // points requires that the fourth argument (s) be a valid triangle
  // with a point list so we must check that t1 and t2 are not already
  // done and thus do not have a point list. If at least one does then
  // call distribute points with s = the trinagle with the valid point
  // list. If both are done then the newly created triangles are done
  // tooand need to be marked accordingly
  if(t1->maxE != DONE && t2->maxE != DONE)
    distrPoints(tn1,tn2,NULL,t1,t2,e,tt);
  else if(t1->maxE != DONE){
    distrPoints(tn1,tn2,NULL,t1,NULL,e,tt);
  }
  else if(t2->maxE != DONE){
    distrPoints(tn1,tn2,NULL,t2,NULL,e,tt);
  }
  else{
    tn1->maxE = tn2->maxE = DONE;
    tn1->points = tn2->points= NULL;
  }

  // Update the corner if the corner is being swapped. Distrpoints
  // will not always catch this so it must be done here
  if(t1 == tt->t || t2 == tt->t){
    updateTinTileCorner(tt,tn1,tn2,NULL);    
  }

  // mark triangles for deletion from the PQ
  t1->p1p2 = t1->p1p3 = t1->p2p3 = NULL;
  t2->p1p2 = t2->p1p3 = t2->p2p3 = NULL;
  PQ_delete(tt->pq,t1->pqIndex);
  PQ_delete(tt->pq,t2->pqIndex);
  removeTri(t1);
  removeTri(t2);

  DEBUG{checkPointList(tn1); checkPointList(tn2);}

  // We have created two different triangles, we need to check
  // delaunay on their 2 edges
  enforceDelaunay(tn1,a,d,b,e,tt);
  enforceDelaunay(tn2,c,d,b,e,tt);

}


//
// Enforce delaunay on triangle t. Assume that p1 & p2 are the
// endpoints to the edge that is being checked for delaunay
//
void enforceDelaunay(TRIANGLE *t, R_POINT *p1, R_POINT *p2, R_POINT *p3,
		     double e, TIN_TILE *tt){
  assert(t);

  // Since we have tiles we cannot garauntee global delaunay. We must
  // not enforce delaunay on boundary edges, that is edges that are on
  // the same boundary together.
  if(!(edgeOnBoundary(p1,p2,tt))){
    
    // Find the triangle on the other side of edge p1p2
    TRIANGLE *tn = whichTri(t,p1,p2,tt);  
    
    // If tn is NULL then we are done, otherwise we need to check delaunay
    if(tn != NULL){
      
      // Find point across from edge p1p2 in tn
      R_POINT *d = findThirdPoint(tn->p1,tn->p2,tn->p3,p1,p2);
      
      if(CircumCircle(d->x,d->y,p1->x,p1->y,p2->x,p2->y,p3->x,p3->y)){
	edgeSwap(t,tn,p1,p3,p2,d,e,tt);
      }
    }
  }
}


// 
// Refine each tile individually, write it to disk, and free it from
// memory. This way only one tile and boundary arrays are in memory at
// one time.
//
void refineTin(double e, short delaunay, TIN *tin,char *path,
	       char *siteFileName, char *vectFileName, short useNodata){

  TIN_TILE *tt;
  printf("refining..\n"); fflush(stdout);
  
  // write tin file headers
  if(path != NULL)
    writeTin(tin,path,1);

#ifdef __GRASS__
  // Write site file headers
  //
  FILE *sitesFile = NULL;
  struct Map_info Map;
    
  if(siteFileName != NULL){
    char errbuf[100];
    if ((sitesFile = G_fopen_sites_new(siteFileName)) == NULL){
      sprintf(errbuf,"Not able to open sitesfile for [%s]\n", siteFileName);
      G_fatal_error(errbuf);
    }
    writeSitesHeader(sitesFile,siteFileName);
    assert(sitesFile); 
  }  
  if (vectFileName != NULL) {
    // Write vector file headers
    //
    // Create new digit file
    if ((Vect_open_new (&Map, vectFileName)) < 0){
      G_fatal_error("Creating new vector file.\n") ;
    }
    
    // Write vector header
    set_default_head_info (&(Map.head));
  }
#endif
  
  // Skip the dummy head
  tt = tin->tt->next;
  while(tt->next != NULL){
    refineTile(tt,e,delaunay,useNodata);
    tin->numTris += tt->numTris;
    tin->numPoints += tt->numPoints;
#ifdef __GRASS__
    if(siteFileName != NULL) {
      writeSitesTile(tt,sitesFile, siteFileName);
      assert(sitesFile);
    }
    if(vectFileName != NULL) {
      writeVectorTile(&Map,tt);
    }
#endif
    if(path != NULL)
      writeTinTile(tt,path,1);
    
    // Go to next tile
    tt = tt->next;
  }
  // If there is only one tile then get info from it
  if(tin->tt == tt && tt->next != NULL){ // Fix me ?
    tin->numTris += tt->numTris;
    tin->numPoints += tt->numPoints;
  }

#ifdef __GRASS__
  if (siteFileName != NULL) {
    assert(sitesFile);
    fclose(sitesFile);
  }
  if (vectFileName != NULL) {
    Vect_close (&Map);
  }
#endif
  
  tinGlobal = tin;
  printf("done refining\n"); fflush(stdout);
}


//
// Refine a grid into a TIN_TILgE with error < e
//
void refineTile(TIN_TILE *tt, double e, short delaunay, short useNodata) {  
  BOOL complete;    // is maxE < e
  complete = 0;
  TRIANGLE *t1, *t2, *t3, *s;

  int refineCount = 0;

  // Read points for initial two triangles into a file
  initTilePoints(tt,e,useNodata);
  
  // While there still is a triangle with max error > e
  while(PQ_extractMin(tt->pq, &s)){

    // Triangles should no longer be marked for deletion since they
    // are being deleted from the PQ
    if(s->p1p2 == NULL && s->p1p3 == NULL && s->p2p3 == NULL){
      printf(strcat("skipping deleted triangle: err=",ELEV_TYPE_PRINT_CHAR),
	     s->maxErrorValue);
      fflush(stdout);
      assert(0);
      removeTri(s);
    }

    assert(s); 
    refineCount++;

    // malloc the point with max error as it will become a corner
    R_POINT* maxError = (R_POINT*)malloc(sizeof(R_POINT));
    assert(maxError);	
    maxError->x = s->maxE->x;
    maxError->y = s->maxE->y;
    maxError->z = s->maxE->z;

    // Add point to the correct point pointer array
    assert(tt->bPointsCount < tt->ncols && 
	   tt->rPointsCount < tt->nrows && 
	   tt->bPointsCount < (tt->ncols * tt->nrows)-(tt->ncols + tt->nrows));
    if(maxError->x == (tt->iOffset + tt->nrows-1) ){
      tt->bPoints[tt->bPointsCount]=maxError;
      tt->bPointsCount++;
    }
    else if(maxError->y == (tt->jOffset + tt->ncols-1) ){
      tt->rPoints[tt->rPointsCount]=maxError;
      tt->rPointsCount++;
    }
    else{
      tt->points[tt->pointsCount]=maxError;
      tt->pointsCount++;
    }

    // Debug - print the point being added
#ifdef REFINE_DEBUG 
    {
      TRIANGLE *snext;
      R_POINT err = findError(s->maxE->x, s->maxE->y, s->maxE->z, s); 
      printf("Point (%6d,%6d,%6d) error=%10ld \t", 
	     s->maxE->x, s->maxE->y, s->maxE->z, err );
      printTriangleCoords(s);
      fflush(stdout);

      if(err != s->maxErrorValue){
	printf("Died err= %ld maxE= %ld \n",err,s->maxErrorValue);
	exit(1);
      }

      PQ_min(tt->pq, &snext); 
      assert(s->maxErrorValue >= snext->maxErrorValue); 
    }
#endif
    // Check for collinear points. We make the valid assumption that
    // MaxE cannot be collinear with > 1 tri
    int area12,area13,area23;

    area12 = areaSign(s->p1, s->p2, maxError);
    area13 = areaSign(s->p1, maxError, s->p3);
    area23 = areaSign(maxError, s->p2, s->p3);

    // If p1 p2 is collinear with MaxE
    if (!area12){
      fixCollinear(s->p1,s->p2,s->p3,s,e,maxError,tt,delaunay);
      tt->numTris++;
    }
    else if (!area13){
      fixCollinear(s->p1,s->p3,s->p2,s,e,maxError,tt,delaunay);
      tt->numTris++;
    }
    else if (!area23){
      fixCollinear(s->p2,s->p3,s->p1,s,e,maxError,tt,delaunay);
      tt->numTris++;
    }
    else {
      // add three new triangles
      t1 = addTri(tt,s->p1, s->p2, maxError,s->p1p2,NULL,NULL);
      t2 = addTri(tt,s->p1, maxError, s->p3,t1,s->p1p3,NULL);
      t3 = addTri(tt,maxError, s->p2, s->p3,t1,t2,s->p2p3);
      DEBUG{triangleCheck(s,t1,t2,t3);}

      tt->numTris += 2;

      // create poinlists from the original tri (this will yeild the max error)
      distrPoints(t1,t2,t3,s,NULL,e,tt);
      DEBUG{checkPointList(t1);checkPointList(t2);checkPointList(t3);}  

      // Enforce delaunay on three new edges of the new triangles if
      // specified
      if(delaunay){
	// we enforce on the edge that does not have maxE as an
	// endpoint, so the 4th argument to enforceDelaunay should
	// always be the maxE point to s for that particular tri
	enforceDelaunay(t1,t1->p1,t1->p2,t1->p3,e,tt);
	enforceDelaunay(t2,t2->p1,t2->p3,t2->p2,e,tt);
	enforceDelaunay(t3,t3->p2,t3->p3,t3->p1,e,tt);
      }
    }
    
    // remove original tri
    removeTri(s);
    //DEBUG{printTin(tt);}

    extern int displayValid;
    displayValid = 0;

  } 
  s = tt->t;
 
  // The number of points added is equal to the number of refine loops
  tt->numPoints += refineCount;
  
  /* The number of points should be equal to the sum of all points
     arrays (one center and possible 4 boundary). Since the arrays
     have overlap of corner points we subtract the overlaps */
  int pts = 0;
  pts = tt->pointsCount + tt->rPointsCount + tt->bPointsCount - 1;
  if(tt->top != NULL)
    pts += tt->top->bPointsCount - 2;
  if(tt->left != NULL)
    pts += tt->left->rPointsCount - 2;
  //assert(tt->numPoints == pts); fix

  // Sort the point arrays for future use and binary searching
  qsort(tt->rPoints,tt->rPointsCount,sizeof(R_POINT*),(void *)QS_compPoints);
  qsort(tt->bPoints,tt->bPointsCount,sizeof(R_POINT*),(void *)QS_compPoints);
  qsort(tt->points,tt->pointsCount,sizeof(R_POINT*),(void *)QS_compPoints);

  // We are done with the pq
  PQ_free(tt->pq);

}


//
// Add triangles and distribute points when there are two collinear
// triangles. Assumes that maxE is on line pa pb
//
void fixCollinear(R_POINT *pa, R_POINT *pb, R_POINT *pc, TRIANGLE* s, double e,
		  R_POINT *maxError, TIN_TILE *tt, short delaunay){

  assert(s && tt->pq && maxError);
  TRIANGLE *sp, *t1, *t2, *t3, *t4;
  
  // add 2 tris in s
  t1 = addTri(tt,pa, maxError, pc,NULL,whichTri(s,pa,pc,tt),NULL);
  assert(t1);
  t2 = addTri(tt,maxError, pc, pb,t1,NULL,whichTri(s,pb,pc,tt));
  assert(t2);
  DEBUG{triangleCheck(s,t1,t2,NULL);}
  
  // Distribute points in the 2 triangles
  distrPoints(t1,t2,NULL,s,NULL,e,tt);

  DEBUG{checkPointList(t1); checkPointList(t2);}  

  // Find other triangle Note: there may not be another triangle if s
  // is on the edge
  sp = whichTri(s,pa,pb,tt);
  
  // If there is a triangle on the other side of the collinear edge
  // then we need to split that triangle into two
  if(sp != NULL){

    // Find the unknown point of the triangle on the other side of the line
    R_POINT *pd;
    pd = findThirdPoint(sp->p1,sp->p2,sp->p3,pa,pb);
    assert(pd);

    // If this triangle is in the other tile we need to make sure we
    // work with that tile
    TIN_TILE *ttn;
    ttn = whichTileTri(pa,pb,pd,tt);  
    assert(ttn);

    assert(pointInTile(pd,ttn));
    
    // add 2 tris adjacent to s on pa pb
    t3 = addTri(ttn,pa,maxError,pd,t1,whichTri(sp,pd,pa,ttn),NULL);
    assert(t3);
    t4 = addTri(ttn,pb,maxError,pd,t2,whichTri(sp,pd,pb,ttn),t3);
    assert(t4);
    DEBUG{triangleCheck(sp,t3,t4,NULL);}
    
    // 1 more tri 
    ttn->numTris++;


    //If this triangle was already "done" meaning it has no more
    //points in it's point list > MaxE then we don't distribute points
    //because sp has no point list
    if(sp->maxE != DONE){
      distrPoints(t3,t4,NULL,sp,NULL,e,ttn);
      //Mark triangle sp for deletion from pq before distrpoints so we
      //can include its maxE in the newly created triangle
      sp->p1p2 = sp->p1p3 = sp->p2p3 = NULL;
      PQ_delete(ttn->pq,sp->pqIndex);
    }
    else{
      // Since distrpoints normally fixes corner we need to do it here
      if(sp == tt->t){
	updateTinTileCorner(ttn,t3,t4,NULL);    
      }
      t3->maxE = t4->maxE = DONE;
      t3->points = t4->points = NULL;
    }
    DEBUG{checkPointList(t3); checkPointList(t4);}
    
    
    removeTri(sp);
    

    // Enforce delaunay on two new edges of the new triangles if
    // specified
    if(delaunay){
      // we enforce on the edge that does not have maxE as an
      // endpoint, so the 4th argument to enforceDelaunay should
      // always be the maxE point to s for that particular tri
      enforceDelaunay(t1,t1->p1,t1->p3,t1->p2,e,tt);
      enforceDelaunay(t2,t2->p2,t2->p3,t2->p1,e,tt);
      if(ttn == tt){
	enforceDelaunay(t3,t3->p1,t3->p3,t3->p2,e,ttn);
	enforceDelaunay(t4,t4->p1,t4->p3,t4->p2,e,ttn);
      }
    }
  }
  else{
    // Enforce delaunay on two new edges of the new triangles if
    // specified
    if(delaunay){
      // we enforce on the edge that does not have maxE as an
      // endpoint, so the 4th argument to enforceDelaunay sho uld
      // always be the maxE point to s for that particular tri
      enforceDelaunay(t1,t1->p1,t1->p3,t1->p2,e,tt);
      enforceDelaunay(t2,t2->p2,t2->p3,t2->p1,e,tt);
    }
  }
}


//
// Divide a pointlist of s and sp into the three pointlists of the new
// tris. This assumes that s exists and has a point list. Most of the
// time sp will be NULL because we are distributing points from a
// parent triangle. sp may not be NULL when edge swapping for
// delaunay. Assume that if sp is not null then it has a valid point list
//
 void distrPoints(TRIANGLE* t1, TRIANGLE* t2, TRIANGLE* t3, TRIANGLE* s, 
		  TRIANGLE* sp, double e, TIN_TILE *tt) {

  //at most one can be null
  assert((t1 && t2) || (t1 && t3) || (t2 && t3));
  // Distribute points should never be called on a triangle that does
  // not have a list of points
  assert(s->points);

  // Assume that if sp is not null then it has a valid point list
  if(sp != NULL)
    assert(sp->points);

  ELEV_TYPE max1=e;
  ELEV_TYPE max2=e;
  ELEV_TYPE max3=e;
  ELEV_TYPE tempE=0;

  register QUEUE h;
  register QNODE* cur;
  h = s->points;



  // Add a queue of points for each triangle that is not NULL and that
  // does not already have a point list
  if(t1 != NULL){
    t1->points = Q_init();
    t1->maxE = DONE; // this will change if not actually done
  }
  if(t2 != NULL){
    t2->points = Q_init();
    t2->maxE = DONE; // this will change if not actually done
  }
  if(t3 != NULL){
    t3->points = Q_init();
    t3->maxE = DONE; // this will change if not actually done
  }
     
  int doneDistr = 0;
  
  // Have we seen only nodata points?
  short hasNoData1 = 0;
  short hasNoData2 = 0;
  short hasNoData3 = 0;

  short pointAdded = 0;    

  // Build point list from s and sp also update lower left corner if nessesary
  while(doneDistr == 0){

    DEBUG{checkPointList(s);} 

    // if s is the lower leftmost tri then update the lower left tri
    if(s == tt->t){
      updateTinTileCorner(tt,t1,t2,t3);    
    }

    //Loop and distribute points until the dummy head->next == NULL
    while((cur = Q_remove_first(h)) != NULL) {
      assert(inTri2D(s->p1, s->p2, s->p3, &cur->e));
      
      // We have not yet added a point this loop
      pointAdded = 0;

      // skip the point with the maxE if this triangle is not marked for
      // deletion. If it is marked for deletion then it needs to be
      // added to one of the triangles being created
      if(&cur->e == s->maxE && 
	 !(s->p1p2 == NULL && s->p1p3 == NULL && s->p2p3 == NULL)){
	free(cur);
	continue;
      } 

      // Triangle 1
      if(t1 != NULL){
	if(inTri2D(t1->p1, t1->p2, t1->p3, &cur->e)) {
	  // Add point to point list
	  Q_insert_qnode_head(t1->points,cur);
	  pointAdded = 1;
	  // Don't calc maxE for nodata points
	  tempE = findError(cur->e.x, cur->e.y, cur->e.z, t1);
	  // Update max error
	  if (tempE>=max1) {
	    max1 = tempE;
	    t1->maxE = &cur->e;
	    t1->maxErrorValue = tempE;
	  }
	  continue;
	}
      }

      // Triangle 2
      if(t2 != NULL){
	if(inTri2D(t2->p1, t2->p2, t2->p3, &cur->e)) {
	  // Don't calc maxE for nodata points
	  Q_insert_qnode_head(t2->points,cur);
	  tempE = findError(cur->e.x, cur->e.y, cur->e.z, t2);
	  //Update max error
	  if (tempE>=max2) {
	    max2 = tempE;
	    t2->maxE = &cur->e;
	    t2->maxErrorValue = tempE;
	  }
	  continue;
	}
      }

      // Triangle 3
      if(t3 != NULL){
	if(inTri2D(t3->p1, t3->p2, t3->p3, &cur->e)){ 
	  Q_insert_qnode_head(t3->points,cur);
	  tempE = findError(cur->e.x, cur->e.y, cur->e.z, t3);
	  // Update max error
	  if (tempE>=max3) {
	    max3 = tempE;
	    t3->maxE = &cur->e;
	    t3->maxErrorValue = tempE;
	  }  
	  continue;
	}
      }

      //should never get here if point is not nodata
      if(cur->e.z != tt->nodata){
	assert(0);
	exit(1);
      }

    }//while

    // If we have an sp, go through and distribute points for sp
    if(sp != NULL){
      s = sp;
      h = s->points;
      sp = NULL;
    }
    // No sp? then we are done distributing the points
    else
      doneDistr = 1;

  }//while

  // Free any unused point lists
  if(t1 != NULL && t1->maxE == DONE){
    // If all the points in t1 are nodata and there are points in t1
    // then this triangle should be marked to not be drawn
    if(hasNoData1){
      //allNoData1 == 1 && Q_first(t1->points) != NULL
      Q_free_queue(t1->points);
      t1->points = NULL;
      t1->maxErrorValue = 0;
    }
    else{
      Q_free_queue(t1->points);
      t1->points = NULL;
      t1->maxErrorValue = 0;
    }
  }
  else if(t1 != NULL){
    assert(triangleInTile(t1,tt));
    PQ_insert(tt->pq,t1);
  }

  if(t2 != NULL && t2->maxE == DONE){
    // If all the points in t1 are nodata and there are points in t1
    // then this triangle should be marked to not be drawn
    if(hasNoData2){
      Q_free_queue(t2->points);
      t2->points = NULL;
      t2->maxErrorValue = 0;
    }
    else{
      Q_free_queue(t2->points);
      t2->points = NULL;
      t2->maxErrorValue = 0;
    }
  }
  else if(t2 != NULL){
    assert(triangleInTile(t2,tt));
    PQ_insert(tt->pq,t2);
  }

  if(t3 != NULL && t3->maxE == DONE){ 
    // If all the points in t1 are nodata and there are points in t1
    // then this triangle should be marked to not be drawn
    if(hasNoData3){
      Q_free_queue(t3->points);
      t3->points = NULL;
      t3->maxErrorValue = 0;
    }
    else{
      Q_free_queue(t3->points);
      t3->points = NULL;
      t3->maxErrorValue = 0;
    }
  }
  else if(t3 != NULL){ 
    assert(triangleInTile(t2,tt));
    PQ_insert(tt->pq,t3);
  }
}


//
// This function is called when we are refining the lower left most
// triangle since we need to choose the new lower left triangle
//
void updateTinTileCorner(TIN_TILE *tt, TRIANGLE* t1, TRIANGLE* t2,
			 TRIANGLE* t3){
  if(t1 != NULL){
    if(t1->p1->y==tt->jOffset && t1->p2->y==tt->jOffset && 
       t1->p1->x==tt->nrows-1+tt->iOffset){
      tt->t = t1;
      tt->v = t1->p1;
      tt->e.p1 = t1->p1;
      tt->e.p2 = t1->p2;
    }
    else if(t1->p1->y==tt->jOffset && t1->p2->y==tt->jOffset && 
	    t1->p2->x==tt->nrows-1+tt->iOffset){
      tt->t = t1;
      tt->v = t1->p2;
      tt->e.p1 = t1->p1;
      tt->e.p2 = t1->p2;
    }
    else if(t1->p1->y==tt->jOffset && t1->p3->y==tt->jOffset && 
	    t1->p1->x==tt->nrows-1+tt->iOffset){
      tt->t = t1;
      tt->v = t1->p1;
      tt->e.p1 = t1->p1;
      tt->e.p2 = t1->p3;
    }
    else if(t1->p1->y==tt->jOffset && t1->p3->y==tt->jOffset && 
	    t1->p3->x==tt->nrows-1+tt->iOffset){
      tt->t = t1;
      tt->v = t1->p3;
      tt->e.p1 = t1->p1;
      tt->e.p2 = t1->p3;
    }
    else if(t1->p2->y==tt->jOffset && t1->p3->y==tt->jOffset && 
	    t1->p2->x==tt->nrows-1+tt->iOffset){
      tt->t = t1;
      tt->v = t1->p2;
      tt->e.p1 = t1->p2;
      tt->e.p2 = t1->p3;
    }
    else if(t1->p2->y==tt->jOffset && t1->p3->y==tt->jOffset && 
	    t1->p3->x==tt->nrows-1+tt->iOffset){
      tt->t = t1;
      tt->v = t1->p3;
      tt->e.p1 = t1->p2;
      tt->e.p2 = t1->p3;
    }
  }
  if(t2 != NULL){
    if(t2->p1->y==tt->jOffset && t2->p2->y==tt->jOffset && 
       t2->p1->x==tt->nrows-1+tt->iOffset){
      tt->t = t2;
      tt->v = t2->p1;
      tt->e.p1 = t2->p1;
      tt->e.p2 = t2->p2;
    }
    else if(t2->p1->y==tt->jOffset && t2->p2->y==tt->jOffset && 
	    t2->p2->x==tt->nrows-1+tt->iOffset){
      tt->t = t2;
      tt->v = t2->p2;
      tt->e.p1 = t2->p1;
      tt->e.p2 = t2->p2;
    }
    else if(t2->p1->y==tt->jOffset && t2->p3->y==tt->jOffset && 
	    t2->p1->x==tt->nrows-1+tt->iOffset){
      tt->t = t2;
      tt->v = t2->p1;
      tt->e.p1 = t2->p1;
      tt->e.p2 = t2->p3;
    }
    else if(t2->p1->y==tt->jOffset && t2->p3->y==tt->jOffset && 
	    t2->p3->x==tt->nrows-1+tt->iOffset){
      tt->t = t2;
      tt->v = t2->p3;
      tt->e.p1 = t2->p1;
      tt->e.p2 = t2->p3;
    }
    else if(t2->p2->y==tt->jOffset && t2->p3->y==tt->jOffset && 
	    t2->p2->x==tt->nrows-1+tt->iOffset){
      tt->t = t2;
      tt->v = t2->p2;
      tt->e.p1 = t2->p2;
      tt->e.p2 = t2->p3;
    }
    else if(t2->p2->y==tt->jOffset && t2->p3->y==tt->jOffset && 
	    t2->p3->x==tt->nrows-1+tt->iOffset){
      tt->t = t2;
      tt->v = t2->p3;
      tt->e.p1 = t2->p2;
      tt->e.p2 = t2->p3;
    }
  }
  if(t3 != NULL){
    if(t3->p1->y==tt->jOffset && t3->p2->y==tt->jOffset && 
       t3->p1->x==tt->nrows-1+tt->iOffset){
      tt->t = t3;
      tt->v = t3->p1;
      tt->e.p1 = t3->p1;
      tt->e.p2 = t3->p2;
    }
    else if(t3->p1->y==tt->jOffset && t3->p2->y==tt->jOffset && 
	    t3->p2->x==tt->nrows-1+tt->iOffset){
      tt->t = t3;
      tt->v = t3->p2;
      tt->e.p1 = t3->p1;
      tt->e.p2 = t3->p2;
    }
    else if(t3->p1->y==tt->jOffset && t3->p3->y==tt->jOffset && 
	    t3->p1->x==tt->nrows-1+tt->iOffset){
      tt->t = t3;
      tt->v = t3->p1;
      tt->e.p1 = t3->p1;
      tt->e.p2 = t3->p3;
    }
    else if(t3->p1->y==tt->jOffset && t3->p3->y==tt->jOffset && 
	    t3->p3->x==tt->nrows-1+tt->iOffset){
      tt->t = t3;
      tt->v = t3->p3;
      tt->e.p1 = t3->p1;
      tt->e.p2 = t3->p3;
    }
    else if(t3->p2->y==tt->jOffset && t3->p3->y==tt->jOffset && 
	    t3->p2->x==tt->nrows-1+tt->iOffset){
      tt->t = t3;
      tt->v = t3->p2;
      tt->e.p1 = t3->p2;
      tt->e.p2 = t3->p3;
    }
    else if(t3->p2->y==tt->jOffset && t3->p3->y==tt->jOffset && 
	    t3->p3->x==tt->nrows-1+tt->iOffset){
      tt->t = t3;
      tt->v = t3->p3;
      tt->e.p1 = t3->p2;
      tt->e.p2 = t3->p3;
    }
  }
  DEBUG{printf("Change LLC: (%d,%d)(%d,%d)(%d,%d) \n",
	       tt->t->p1->x,tt->t->p1->y,tt->t->p2->x,tt->t->p2->y,
	       tt->t->p3->x,tt->t->p3->y);}
}

