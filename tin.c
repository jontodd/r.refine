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
 * tin.c handles all creation,manipulation, and output of TIN and
 * TIN_TIlE structure
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-11
 *
 * COMMENTS:
 *
 *****************************************************************************/

#include "tin.h"

// Debug mode
#define DEBUG if(0)

// dummy pointer for all triangles with maxE < e
R_POINT DONEVar;
R_POINT *DONE = &DONEVar;


//
// add trinagle to a TIN and return pointer to it
//
TRIANGLE* addTri(TIN_TILE *tt, R_POINT *p1, R_POINT *p2, R_POINT *p3, 
		 TRIANGLE *t12, TRIANGLE *t13, TRIANGLE *t23) {
  assert(p1 && p2 && p3 && tt && tt->pq);
  
  // Validate that the triangle is not collinear
  assert(areaSign(p1,p2,p3));

  // Validate that all points should be in this tile
  assert(pointInTile(p1,tt) && pointInTile(p2,tt) && pointInTile(p3,tt));

  // Allocate space for this triangle and give it values
  TRIANGLE *tn;
  tn = (TRIANGLE*)malloc(sizeof(TRIANGLE));
  assert(tn);
  
  // Assign values to the triangle
  tn->maxE = NULL;
  tn->points = NULL;
  tn->pqIndex = UINT_MAX;
  tn->p1=p1;
  tn->p2=p2;
  tn->p3=p3;
  tn->p1p2 = t12;
  tn->p1p3 = t13;
  tn->p2p3 = t23;
 
  // point neighbor triangles to this triangle if not on boundary
  if(!edgeOnBoundary(p1,p2,tt))
    pointNeighborTriTo(tn,p1,p2,t12);
  if(!edgeOnBoundary(p1,p3,tt))
    pointNeighborTriTo(tn,p1,p3,t13);
  if(!edgeOnBoundary(p2,p3,tt))
    pointNeighborTriTo(tn,p2,p3,t23);

  return tn;
}


//
// Given a triangle and 2 pts return the neighbor triangle to that edge
//
TRIANGLE* whichTri(TRIANGLE *tn,R_POINT *pa,R_POINT *pb,TIN_TILE *tt){
  assert(pa && pb);
  
  // If we are on the boundary then our neighbor is assumed NULL
  if(edgeOnBoundary(pa,pb,tt))
    return NULL;

  assert(tn);

  if( (tn->p1 == pa || tn->p2 == pa) && (tn->p1 == pb || tn->p2 == pb) ){
    assert(tn->p1p2 != tn);
    return tn->p1p2;
  }
  if( (tn->p1 == pa || tn->p3 == pa) && (tn->p1 == pb || tn->p3 == pb) ){
    assert(tn->p1p3 != tn);
    return tn->p1p3;
  }
  else if ( (tn->p2 == pa || tn->p3 == pa) && (tn->p2 == pb || tn->p3 == pb) ){
    assert(tn->p2p3 != tn);
    return tn->p2p3;
  }
  else {
    assert(0);
    exit(1);
    return NULL;
  }
}


//
// Which of p1-p3 is same as pa
//
R_POINT* whichPoint(R_POINT *pa, R_POINT *p1, R_POINT *p2, R_POINT *p3){
  if(pa->x == p1->x && pa->y == p1->y)
    return p1;
  else if(pa->x == p2->x && pa->y == p2->y)
    return p2;
  else{
    assert(pa->x == p3->x && pa->y == p3->y);
    return p3;
  }
}


//
// Find neighbor (t) to tn and point proper edge to t.
//
void pointNeighborTriTo(TRIANGLE *t,R_POINT *pa,R_POINT *pb,TRIANGLE *tn){
  assert(t || t == NULL);
  assert(t != tn || (t == NULL && tn == NULL));
  if(tn != NULL){
    if( (tn->p1 == pa || tn->p2 == pa) && (tn->p1 == pb || tn->p2 == pb) )
      tn->p1p2 = t;
    else if( (tn->p1 == pa || tn->p3 == pa) && (tn->p1 == pb || tn->p3 == pb) )
      tn->p1p3 = t;
    else if ( (tn->p2 == pa || tn->p3 == pa) && (tn->p2 == pb || tn->p3 == pb))
      tn->p2p3 = t;
    else{
      assert(0);
      exit(1);
    }
  }
}


//
// Print the triangles in a TIN_TILE for debugging
//
void printTinTile(TIN_TILE* tinTile){
  TRIANGLE *curT = tinTile->t;
  TRIANGLE *prevT = curT;
  
  // Create an edge which 
  EDGE curE;
  curE.type = IN;
  curE.t1 = NULL;
  curE.t2 = tinTile->t;
  curE.p1 = tinTile->e.p1;
  curE.p2 = tinTile->e.p2;
  printf("\n\n PRINTING TIN_TILE \n\n");

  do{
    // Print tri if we are on its IN edge
    if(curE.type == IN && prevT != NULL){
      printTriangle(prevT);
    }

    // Go to next edge
    prevT=curT;
    curT = nextEdge(curT,tinTile->v,&curE,tinTile);
    assert(curT);
    
  }while((curE.p1 != tinTile->e.p1 || curE.p2 != tinTile->e.p2) && 
	 (curE.p1 != tinTile->e.p2 || curE.p2 != tinTile->e.p1));
  
}


//
// Print a tin list for debugging
//
void printTin(TIN *tin){

  TIN_TILE *tt;
  tt = tin->tt;

  // Skip dummy head
  tt = tt->next;
  while(tt->next != NULL){
    printf("\n\n Print TNODE iOffSet: %d jOffSet: %d \n\n",
	   tt->iOffset,tt->jOffset);
    printTinTile(tt);
    tt = tt->next;
  }
}


//
// Print the points of a triangle and pointers for debugging
//
void printTriangle(TRIANGLE* t){
  printf("Tri: t:%p Points: %p(%d,%d) %p(%d,%d) %p(%d,%d) Tris: %p %p %p\n",
	 t,t->p1,t->p1->x,t->p1->y,t->p2,t->p2->x,t->p2->y,t->p3,t->p3->x,
	 t->p3->y,t->p1p2,t->p1p3, t->p2p3);
  //printPointList(t);
}


//
// Print triangle coordiantes
//
void printTriangleCoords(TRIANGLE* t){
  char str[100];
  sprintf(str,"Triangle t: Points: (%s,%s,%s) (%s,%s,%s) (%s,%s,%s)\n",
	  COORD_TYPE_PRINT_CHAR,COORD_TYPE_PRINT_CHAR,ELEV_TYPE_PRINT_CHAR,
	  COORD_TYPE_PRINT_CHAR,COORD_TYPE_PRINT_CHAR,ELEV_TYPE_PRINT_CHAR,
	  COORD_TYPE_PRINT_CHAR,COORD_TYPE_PRINT_CHAR,ELEV_TYPE_PRINT_CHAR);
  printf(str,t->p1->x,t->p1->y,t->p1->z, t->p2->x,t->p2->y,t->p2->z,
	 t->p3->x, t->p3->y,t->p3->z);
}

//
// Returns TRUE if is in tile otherwise it returns the direction of
// the tile it is in
//
TIN_TILE *whichTileTri(R_POINT *p1, R_POINT *p2, R_POINT *p3, TIN_TILE *tt){
  // If there is a point below tt
  if(p1->x < tt->iOffset ||
     p2->x < tt->iOffset || 
     p3->x < tt->iOffset)
    return tt->top;
  // If there is a point  left of tt
  else if( p1->y < tt->jOffset ||
	   p2->y < tt->jOffset || 
	   p3->y < tt->jOffset )
    return tt->left;
     
  // If there is a point right of tt
  else if( p1->y >= (tt->jOffset + tt->ncols) ||
	   p2->y >= (tt->jOffset + tt->ncols) ||
	   p3->y >= (tt->jOffset + tt->ncols) )
    return tt->right;
  // If there is a point above tt
  else if( p1->x >= (tt->iOffset + tt->nrows) ||
	   p2->x >= (tt->iOffset + tt->nrows) ||
	   p3->x >= (tt->iOffset + tt->nrows) )
    return tt->bottom;
  else
    return tt;
}


//
// Is a point on the boundary of its tile?
//
short pointOnBoundary(R_POINT *p, TIN_TILE *tt){
  if(p->x == tt->iOffset || 
     p->y == tt->jOffset ||
     p->y == (tt->jOffset + tt->ncols-1) || 
     p->x == (tt->iOffset + tt->nrows-1) )
    return TRUE;
  else
    return FALSE;
}


//
// Is the point in the given tile?
//
short pointInTile(R_POINT *p, TIN_TILE *tt){
  if((p->x >= tt->iOffset && p->x <= (tt->iOffset + tt->nrows-1)) &&
     (p->y >= tt->jOffset && p->y <= (tt->jOffset + tt->ncols-1)) )
    return TRUE;
  else
    return FALSE;
}


//
// Is the triangle in the given tile
//
short triangleInTile(TRIANGLE *t, TIN_TILE *tt){
  if(pointInTile(t->p1,tt) && pointInTile(t->p2,tt) && pointInTile(t->p3,tt))
    return TRUE;
  else
    return FALSE;
}


//
// Does the triangle have an edge on the boundary of a given tile?
//
short triOnBoundary(R_POINT *p1, R_POINT *p2, R_POINT *p3, TIN_TILE *tt){
  
  short ob1 = pointOnBoundary(p1,tt);
  short ob2 = pointOnBoundary(p2,tt);
  short ob3 = pointOnBoundary(p3,tt);

  if( (ob1 && ob2) || (ob1 && ob3) || (ob2 && ob3) )
    return TRUE;
  else
    return FALSE;

}


//
// Is an edge on the boundary?
//
short edgeOnBoundary(R_POINT *p1, R_POINT *p2, TIN_TILE *tt){
  // An edge is on the boundary if both points are on a boundary and
  // if both points are on the SAME boundary

  // Both Points are on the top boundary
  if(p1->x == tt->iOffset && p2->x == tt->iOffset)
    return TRUE;
  // Both Points are on the bottom boundary
  if(p1->x == (tt->iOffset + tt->nrows-1) &&
     p2->x == (tt->iOffset + tt->nrows-1))
    return TRUE;
  // Both Points are on the left boundary
  if(p1->y == tt->jOffset && p2->y == tt->jOffset)
    return TRUE;
  // Both Points are on the right boundary
  if( p1->y == (tt->jOffset + tt->ncols-1) &&
      p2->y == (tt->jOffset + tt->ncols-1))
    return TRUE;
  
  return FALSE;

 
}


//
// This can be used to traverse the tin. Since this will cross every
// triangle 3 times, it is is important to only use next tri when the
// returned edge is of type IN
//
TRIANGLE *nextEdge(TRIANGLE *t, R_POINT *v, EDGE *edge, TIN_TILE *tt){
  assert(t && v && edge && edge->p1 && edge->p2);
  EDGE e12, e13, e23;
  short inCount,outCount;
  int areaOp, areaV;
  EDGE *in, *out, *inback, *outback;
  
  in = out = inback = outback = NULL;
  
  inCount = outCount = 1;
  
  //
  // Define the three edges of this triangle to work with them. If t1
  // is NULL then t1 should point to this triangle since the traversal
  // calls for boundary edges to point to the triangle they came
  // from. Some boundary points may not be null so we need to check
  // for them specifically.
  //
  
  if(t->p1p2 == NULL || edgeOnBoundary(t->p1,t->p2,tt) ) //
    e12.t1 = t;
  else
    e12.t1 = t->p1p2;
  e12.t2 = t;
  e12.p1 = t->p1;
  e12.p2 = t->p2;
  if(t->p1p3 == NULL  || edgeOnBoundary(t->p1,t->p3,tt)) //
    e13.t1 = t;
  else
    e13.t1 = t->p1p3;
  e13.t2 = t;
  e13.p1 = t->p1;
  e13.p2 = t->p3;
  if(t->p2p3 == NULL || edgeOnBoundary(t->p2,t->p3,tt) ) //
    e23.t1 = t;
  else
    e23.t1 = t->p2p3;
  e23.t2 = t;
  e23.p1 = t->p2;
  e23.p2 = t->p3;
  
  //
  // This code figures out which edges are IN and which are OUT. It
  // assumes that if areaV is 0 (collinear) then the edge is IN, we
  // will handle this later in the next section of code
  //
  
  // Edge p1,p2
  areaOp = areaSign(t->p1,t->p2,t->p3);
  areaV = areaSign(t->p1,t->p2,v);
  if(areaOp > 0 && areaV <= 0)
    e12.type = IN;
  else if(areaOp < 0 && areaV >= 0 )
    e12.type = IN;
  else
    e12.type = OUT;
  // Edge p1,p3
  areaOp = areaSign(t->p1,t->p3,t->p2);
  areaV = areaSign(t->p1,t->p3,v);
  if(areaOp > 0 && areaV <= 0)
    e13.type = IN;
  else if(areaOp < 0 && areaV >= 0 )
    e13.type = IN;
  else
    e13.type = OUT;
  // Edge p2,p3
  areaOp = areaSign(t->p2,t->p3,t->p1);
  areaV = areaSign(t->p2,t->p3,v);
  if(areaOp > 0 && areaV <= 0)
    e23.type = IN;
  else if(areaOp < 0 && areaV >= 0 )
    e23.type = IN;
  else
    e23.type = OUT;

  //
  // At this point edges are just marked in or out, we need to handle
  // 2 outs or 2 ins. If there are 2 in's mark the left one as the
  // real in, change the count and backward link the variables. There
  // is also the chance that one of two degenerate cases. The first is
  // that v may be one of the point of the trianle which means that we
  // are opperating on the lower left most triangle. This is unique
  // because there will be two collinear edge with v. The other case
  // is where only one of the edges is collinear with v. In the either
  // case there is a perscribed solution to the problem in the code
  // below.
  //

  if(e12.type == IN && e13.type == IN){
    areaV = areaSign(v,t->p1,t->p2);
    areaOp = areaSign(v,t->p1,t->p3);
    // e12 is the real in edge and there are no collinears
    if( areaV > 0  && areaOp < 0){
      inback = &e13;
      in = &e12;
      out = &e23;
      e13.type = INBACK;
      inCount++;
    }
    // e13 is the real in edge and there are no collinears
    else if ( areaV < 0 && areaOp > 0){
      inback = &e12;
      in = &e13;
      out = &e23;
      e12.type = INBACK;
      inCount++;
    }
    // e13 is collinear and p2 is right of e13
    else if ( areaV < 0 && areaOp == 0){
      in = &e12;
      out = &e13;
      outback = &e23;
      e13.type = OUT;
      e23.type = OUTBACK;
      outCount++;
    }
    // e13 is collinear and p2 is left of e13
    else if ( areaV > 0 && areaOp == 0){
      inback = &e13;
      in = &e12;
      out = &e23;
      e13.type = INBACK;
      inCount++;
    }
    // e12 is collinear and p3 is right of e12
    else if ( areaV == 0 && areaOp < 0){
      in = &e13;
      out = &e12;
      outback = &e23;
      e12.type = OUT;
      e23.type = OUTBACK;
      outCount++;
    }
    // e12 is collinear and p3 is left of e12
    else if ( areaV == 0 && areaOp > 0){
      inback = &e12;
      in = &e13;
      out = &e23;
      e12.type = INBACK;
      inCount++;
    }
    // They are both collinear, this must be the lower left corner
    else {
      assert(areaV == 0 && areaOp == 0);
      // Determine which edge is left most
      areaV = areaSign(t->p1,t->p2,t->p3);
      // areaV should not be 0
      assert(areaV); 
      // if p3 is left of p1p2 then e13 is OUT
      if(areaV > 0){
	in = &e12;
	out = &e13;
	outback = &e23;
	e13.type = OUT;
	e23.type = OUTBACK;
	outCount++;
      }
      // if p3 is right of p1p2 then e12 is OUT
      else {
	in = &e13;
	out = &e12;
	outback = &e23;
	e12.type = OUT;
	e23.type = OUTBACK;
	outCount++;
      }
    }
  }

  if(e12.type == IN && e23.type == IN){
    areaV = areaSign(v,t->p2,t->p1);
    areaOp = areaSign(v,t->p2,t->p3);
    // e12 is the real in edge and there are no collinears
    if( areaV > 0  && areaOp < 0){
      inback = &e23;
      in = &e12;
      out = &e13;
      e23.type = INBACK;
      inCount++;
    }
    // e23 is the real in edge and there are no collinears
    else if ( areaV < 0 && areaOp > 0){
      inback = &e12;
      in = &e23;
      out = &e13;
      e12.type = INBACK;
      inCount++;
    }
    // e23 is collinear and p1 is right of e23
    else if ( areaV < 0 && areaOp == 0){
      in = &e12;
      out = &e23;
      outback = &e13;
      e23.type = OUT;
      e13.type = OUTBACK;
      outCount++;
    }
    // e23 is collinear and p1 is left of e23
    else if ( areaV > 0 && areaOp == 0){
      inback = &e23;
      in = &e12;
      out = &e13;
      e23.type = INBACK;
      inCount++;
    }
    // e12 is collinear and p3 is right of e12
    else if ( areaV == 0 && areaOp < 0){
      in = &e23;
      out = &e12;
      outback = &e13;
      e12.type = OUT;
      e13.type = OUTBACK;
      outCount++;
    }
    // e12 is collinear and p3 is left of e12
    else if ( areaV == 0 && areaOp > 0){
      inback = &e12;
      in = &e23;
      out = &e13;
      e12.type = INBACK;
      inCount++;
    }
    // They are both collinear, this must be the lower left corner
    else {
      assert(areaV == 0 && areaOp == 0);
      // Determine which edge is left most
      areaV = areaSign(t->p2,t->p1,t->p3);
      // areaV should not be 0
      assert(areaV); 
      // if p3 is left of p2p1 then e23 is OUT
      if(areaV > 0){
	in = &e12;
	out = &e23;
	outback = &e13;
	e23.type = OUT;
	e13.type = OUTBACK;
	outCount++;
      }
      // if p3 is right of p2p1 then e12 is OUT
      else {
	in = &e23;
	out = &e12;
	outback = &e13;
	e12.type = OUT;
	e13.type = OUTBACK;
	outCount++;
      }
    }
  }

  if(e13.type == IN && e23.type == IN){
    areaV = areaSign(v,t->p3,t->p1);
    areaOp = areaSign(v,t->p3,t->p2);
    // e13 is the real in edge and there are no collinears
    if( areaV > 0  && areaOp < 0){
      inback = &e23;
      in = &e13;
      out = &e12;
      e23.type = INBACK;
      inCount++;
    }
    // e23 is the real in edge and there are no collinears
    else if ( areaV < 0 && areaOp > 0){
      inback = &e13;
      in = &e23;
      out = &e12;
      e13.type = INBACK;
      inCount++;
    }
    // e23 is collinear and p1 is right of e23
    else if ( areaV < 0 && areaOp == 0){
      in = &e13;
      out = &e23;
      outback = &e12;
      e23.type = OUT;
      e12.type = OUTBACK;
      outCount++;
    }
    // e23 is collinear and p1 is left of e23
    else if ( areaV > 0 && areaOp == 0){
      inback = &e23;
      in = &e13;
      out = &e12;
      e23.type = INBACK;
      inCount++;
    }
    // e13 is collinear and p2 is right of e13
    else if ( areaV == 0 && areaOp < 0){
      in = &e23;
      out = &e13;
      outback = &e12;
      e13.type = OUT;
      e12.type = OUTBACK;
      outCount++;
    }
    // e13 is collinear and p2 is left of e13
    else if ( areaV == 0 && areaOp > 0){
      inback = &e13;
      in = &e23;
      out = &e12;
      e13.type = INBACK;
      inCount++;
    }
    // They are both collinear, this must be the lower left corner
    else {
      assert(areaV == 0 && areaOp == 0);
      // Determine which edge is left most
      areaV = areaSign(t->p3,t->p1,t->p2);
      // areaV should not be 0
      assert(areaV); 
      // if p2 is left of p3p1 then e23 is OUT
      if(areaV > 0){
	in = &e13;
	out = &e23;
	outback = &e12;
	e23.type = OUT;
	e12.type = OUTBACK;
	outCount++;
      }
      // if p2 is right of p3p1 then e13 is OUT
      else {
	in = &e23;
	out = &e13;
	outback = &e12;
	e13.type = OUT;
	e12.type = OUTBACK;
	outCount++;
      }
    }
  }


  // If there are 2 out's mark the left one as the real out
  if(e12.type == OUT && e13.type == OUT){
    areaV = areaSign(v,t->p1,t->p2);
    outCount++;
    if( areaV >= 0 ){
      outback = &e13;
      out = &e12;
      in = &e23;
      e13.type = OUTBACK;
    }
    else{
      outback = &e12;
      out = &e13;
      in = &e23;
      e12.type = OUTBACK;
    }
  }
  if(e12.type == OUT && e23.type == OUT){
    areaV = areaSign(v,t->p2,t->p1);
    outCount++;
    if( areaV >= 0 ){
      outback = &e23;
      out = &e12;
      in = &e13;
      e23.type = OUTBACK;
    }
    else{
      outback = &e12;
      out = &e23;
      in = &e13;
      e12.type = OUTBACK;
    }
  }
  if(e13.type == OUT && e23.type == OUT){
    areaV = areaSign(v,t->p3,t->p1);
    outCount++;
    if( areaV >= 0 ){
      outback = &e23;
      out = &e13;
      in = &e12;
      e23.type = OUTBACK;
    }
    else{
      outback = &e13;
      out = &e23;
      in = &e12;
      e13.type = OUTBACK;
    }
  }

  
  // Make sure in/out count is not messed up
  assert((inCount == 1 && outCount == 2) || (inCount == 2 && outCount == 1));

  // Determine the new type of edge
  if((t->p1 == edge->p1 || t->p1 == edge->p2) && 
     (t->p2 == edge->p1 || t->p2 == edge->p2))
    edge->type = e12.type; 
  if((t->p1 == edge->p1 || t->p1 == edge->p2) && 
     (t->p3 == edge->p1 || t->p3 == edge->p2))
    edge->type = e13.type; 
  if((t->p2 == edge->p1 || t->p2 == edge->p2) && 
     (t->p3 == edge->p1 || t->p3 == edge->p2))
    edge->type = e23.type; 

  DEBUG{ 
    printf("e12: %d e13: %d e23: %d edge: %d t: %p\n",e12.type,e13.type,
	   e23.type,edge->type,t);
    fflush(stdout);}
  

  //
  // Decide which triangle is next
  //
  
  // Only one in edge
  if(inCount == 1 && edge->type == IN){
    // Change e to be the out edge of t but keep t the same so we
    // return the next tri
    edge->p1 = out->p1;
    edge->p2 = out->p2;
    DEBUG{printf("Case1 - 1 IN\n");fflush(stdout);}
    return out->t1;
  }
  else if(edge->type == IN){
    edge->p1 = out->p1;
    edge->p2 = out->p2;
    DEBUG{printf("Case2 - 2 IN\n");fflush(stdout);}
    return out->t1;
  }
  else if(edge->type == INBACK){
    DEBUG{printf("Case3 - INBACK\n");fflush(stdout);}
    return inback->t1;
  }
  else if(outCount == 2 && edge->type == OUT){
    edge->p1 = outback->p1;
    edge->p2 = outback->p2;
    DEBUG{printf("Case4 - 2 OUT\n");fflush(stdout);}
    return outback->t1;
  }
  else if(outCount == 2 && edge->type == OUTBACK){
    edge->p1 = in->p1;
    edge->p2 = in->p2;
    DEBUG{printf("Case5 - 2 OUT: Outback\n");fflush(stdout);}
    return in->t1;
  }
  // if there is only one out edge
  else{
    edge->p1 = in->p1;
    edge->p2 = in->p2;
    DEBUG{printf("Case6 - 1 Out\n");fflush(stdout);}
    return in->t1;
  }
}


//
// Remove triangle from TIN_TILE and free memory
//
void removeTri(TRIANGLE* t) {
  assert(t);
  free(t);
}


//
// printPointList - given a triangle,print its point list
//
void printPointList(TRIANGLE* t){
  // loop through each node in the list and free the points
  if(t != NULL){
    if(t->points == NULL)
      printf("\t Triangle: %p has no point list to print\n",t);
    else {
      printf("Point list for: %p \n",t);
      QUEUE h;
      QNODE *n;
      h = t->points;
      assert(h);
      n = Q_first(h);
      
      while(n){
	char str[100];
	sprintf(str,"\t x: %s y: %s z: %s ptr:%%p\n",
		COORD_TYPE_PRINT_CHAR,COORD_TYPE_PRINT_CHAR,
		ELEV_TYPE_PRINT_CHAR);
	printf(str,
	       n->e.x,
	       n->e.y,
	       n->e.z,
	       &n->e);
	n = Q_next(h,n);
      }
    }
  }
}


//
// getTileLength finds the dimesions for a sub grid given a memory allocation
//
int getTileLength (double MEM){

  // At anyone time in memory one may need:
  // - 1 Grid with R points
  // - 1 TIN_TILE with R points and 2R Triangles
  // - 1 PQ with 2R elements
  //
  // Max memory for one tile:
  // Let R be number of points in tile and sqrt(R) be length of tile side
  // Let TL be tile length
  // MEM = 2R * (sizeOf(Triangle) + sizeOf(PQelement) + sizeOf(Point))
  //
  // TL = sqrt(MEM/(2*(sizeOf(Triangle) + sizeOf(PQelement) + sizeOf(Point))))

  double TL;
  MEM = MEM * 1048576.0; //convert MB into B
  TL = sqrt(MEM/
	    (2*((double)sizeof(TRIANGLE)+
		(double)sizeof(PQ_elemType)+
		(double)sizeof(R_POINT)))); 
  
  printf("total size per point=%d\n", sizeof(TRIANGLE) +sizeof(PQ_elemType) + sizeof(R_POINT));
  printf("TL = %d\n", (int)TL);
  return TL;
}

//
// compute the signed area of a triangle, positive if the
// third point is off to the left of ab
//
int areaSign(R_POINT *a, R_POINT *b, R_POINT *c){
  
  // Rouding error for area computation. areaX should always be an integer
  double error = 0.5;

  double areaX;
  areaX = (b->x-a->x) * (double)(c->y - a->y) -
          (c->x-a->x) * (double)(b->y - a->y);

  // c is left of ab
  if ( areaX > error)
    return 1;
  // c is right of ab
  else if ( areaX < -error)
    return -1;
  // c is on ab
  else 
    return 0;
}


//
// Is point z in triangle abc?
//
int inTri2D(R_POINT *a, R_POINT *b, R_POINT *c, R_POINT *z){
  
  int area0, area1, area2;
  
  // Assume no collinear triangles
  assert(areaSign(a, b, c) != 0);

  area0 = areaSign( a, b, z);
  area1 = areaSign( b, c, z);
  area2 = areaSign( c, a, z);
  
  if ( ((area0 == 0) && (area1 > 0) && (area2 > 0)) ||
       ((area1 == 0) && (area0 > 0) && (area2 > 0)) ||
       ((area2 == 0) && (area0 > 0) && (area1 > 0)) )
      // on an edge
      return 1;

    if ( ((area0 == 0) && (area1 < 0) && (area2 < 0)) ||
	 ((area1 == 0) && (area0 < 0) && (area2 < 0)) ||
	 ((area2 == 0) && (area0 < 0) && (area1 < 0)) )
      // on an edge
      return 1;

    if ( ((area0 < 0) && (area1 < 0) && (area2 < 0)) ||
	 ((area1 > 0) && (area0 > 0) && (area2 > 0)) )
      // inside tri
      return 1;
    
    if ( (area0 == 0) && (area1 == 0) && (area2 == 0)){
      // a,b,c,z are collinear
      printf("failure");
      assert(0);
      exit(1);
    }
    
    if ( ((area0 == 0) && ( area1 == 0)) ||
	 ((area0 == 0) && ( area2 == 0)) ||
	 ((area2 == 0) && ( area1 == 0)) )
      // z is either a, b, or c this should not happen as we remove points from
      // point lists when they become part of the triangle.
      return 1;

    else 
      return 0;
}


//
// Validate that all points in a triangle's point list are actually in
// that triangle
//
void checkPointList(TRIANGLE* t){
  if(t != NULL){
    if(t->points == NULL)
      printf("triangle %p is DONE\n",t);
    else{
      QNODE *h, *n;
      h = t->points;
      assert(h);
      n = Q_first(h);
      
      int badpoints=0, npoints=0;
      while(n !=  NULL){
	npoints++;
	assert(inTri2D(t->p1, t->p2, t->p3, &n->e));
	if (!inTri2D(t->p1, t->p2, t->p3, &n->e)) {
	  badpoints++;
	}
	//printf("Point %p in triangle %p\n",n->e,t);
	n = Q_next(h,n);
      }
      printf("triangle %p: npoints=%d, badpoints=%d maxE=%p \n",t, npoints, 
	     badpoints,t->maxE);
    }
  }
}


//
// Find the third point given two known points
//
R_POINT *findThirdPoint(R_POINT *p1, R_POINT *p2, R_POINT *p3, R_POINT *pa, 
			R_POINT *pb){
  if((pa == p1 && pb == p2) ||
     (pa == p2 && pb == p1))
    return p3;
  else if((pa == p1 && pb == p3) ||
     (pa == p3 && pb == p1))
    return p2;
  else
    return p1;

}


//
// Is a point an end point of a particular triangle
//
int isEndPoint(TRIANGLE* t, R_POINT *p){
  if(t->p1 == p || t->p2 == p || t->p3 == p)
    return 1;
  else
    return 0;
}


//
// Find the edge opposite to a point p
//
EDGE findOpposite(TRIANGLE* t, R_POINT *p){
  EDGE e={0,0};
  if(t->p1 == p){
    e.p1 = t->p2;
    e.p2 = t->p3;
    return e;
  }
  if(t->p2 == p){
    e.p1 = t->p1;
    e.p2 = t->p3;
    return e;
  }
  else{
    assert(t->p3 == p);
    e.p1 = t->p1;
    e.p2 = t->p2;
    return e;
  }
}


//
// Is a given edge in a triangle t?
//
int edgeInTriangle(TRIANGLE* t, EDGE e){ 
  if((t->p1 == e.p1 && t->p2 == e.p2) || 
     (t->p1 == e.p2 && t->p2 == e.p1) || // edge12
     (t->p1 == e.p1 && t->p3 == e.p2) || 
     (t->p1 == e.p2 && t->p3 == e.p1) || // edge13
     (t->p2 == e.p1 && t->p3 == e.p2) || 
     (t->p2 == e.p2 && t->p3 == e.p1))   // edge23
    return 1;
  else
    return 0;
}


//
// Do two edges have equal valued points?
//
int edgePointsEqual(EDGE e1, EDGE e2){
  if((e1.p1 == e2.p1 || e1.p1 == e2.p2) &&
     (e1.p2 == e2.p1 || e1.p2 == e2.p2) )
    return 1;
  else
    return 0;
}


//
// Validate that at least t1 and t2 are part of t. This is used to
// debug that the triangles being created inside t are actually inside
// triangle t
//
void triangleCheck(TRIANGLE* t,TRIANGLE* t1,TRIANGLE* t2,TRIANGLE* t3){
  assert(t1 != NULL && t2 != NULL);

  // Check s with 3 internal tris
  if(t3 != NULL){
    // Find center point
    R_POINT *cp;
    cp = (R_POINT*) malloc(sizeof(R_POINT));
    if (!isEndPoint(t,t1->p1))
      cp = t1->p1;
    if (!isEndPoint(t,t1->p2))
      cp = t1->p2;
    if (!isEndPoint(t,t1->p3))
      cp = t1->p3;

    // Find opposite edges to Center point
    EDGE e1,e2,e3;
    e1 = findOpposite(t1,cp);
    e2 = findOpposite(t2,cp);
    e3 = findOpposite(t3,cp);

    //for every t1, t2, t3 the edge opposite cp is an edge of t
    // Bad asserts!!!
    assert(edgeInTriangle(t, e1));
    assert(edgeInTriangle(t, e2));
    assert(edgeInTriangle(t, e3));
    // No two edges are the same
    assert(!edgePointsEqual(e1,e2) && !edgePointsEqual(e1,e3) && 
	   !edgePointsEqual(e2,e3));
  }
  else {
    // Find center point (the point on t1 not on s)
    R_POINT *cp;
    cp = (R_POINT*) malloc(sizeof(R_POINT));
    if (!isEndPoint(t,t1->p1))
      cp = t1->p1;
    if (!isEndPoint(t,t1->p2))
      cp = t1->p2;
    if (!isEndPoint(t,t1->p3))
      cp = t1->p3;

    // Find opposite edges to Center point
    EDGE e1,e2;
    e1 = findOpposite(t1,cp);
    e2 = findOpposite(t2,cp);

    //for every t1, t2, t3 the edge opposite cp is an edge of t
    assert(edgeInTriangle(t, e1));
    assert(edgeInTriangle(t, e2));
    // No two edges are the same
    assert(!edgePointsEqual(e1,e2));
  }
  
}


//
// traverse through a TIN_TILE and free each triangle. Since the
// traversal visits every triangle three times, we free each triangle
// the third time we see it.
//
void deleteTinTile(TIN_TILE *tt){

  TRIANGLE *curT = tt->t;
  TRIANGLE *prevT = curT;
  
  // Create an edge which 
  EDGE curE;
  curE.type = IN;
  curE.t1 = NULL;
  curE.t2 = tt->t;
  curE.p1 = tt->e.p1;
  curE.p2 = tt->e.p2;

  // Remove the triangle the third time its seen.
  do{
    
    if( (prevT->maxErrorValue >= -30000.0 || prevT->maxErrorValue <= -30003.0 )
      && prevT != NULL){
      prevT->maxErrorValue = -30001.0;
    }
    else if (prevT->maxErrorValue == -30001.0 && prevT != NULL){
      prevT->maxErrorValue--;
    }
    else if (prevT->maxErrorValue == -30002.0 && prevT != NULL){
      removeTri(prevT);
      prevT->maxErrorValue--;
    }
    else {
      assert(0);
      char str[100];
      sprintf(str,"Possible bad tin output %s\n",ELEV_TYPE_PRINT_CHAR);
      printf(str, prevT->maxErrorValue);
      exit(1);
    }

    // Go to next edge
    prevT=curT;
    curT = nextEdge(curT,tt->v,&curE,tt);
    assert(curT);
    
  }while((curE.p1 != tt->e.p1 || curE.p2 != tt->e.p2) && 
	 (curE.p1 != tt->e.p2 || curE.p2 != tt->e.p1));
  
  removeTri(tt->t);

}

///////////////////////////////////////////////////////////////////////////////
//
// Tin file handling: import and write
//
///////////////////////////////////////////////////////////////////////////////


//
// Given a path to a tin file, read in and return a full tin
// structure. This assumes that the entire TIN can fit into memory
//
TIN *readTinFile(char *path){
  
  // Read in the tin header
  TIN *tin = readTinFileHeader(path);

  TIN_TILE *curTT = tin->tt;
  TIN_TILE *tt = NULL;
  
  // Now loop through and read each tile and link them together
  while((tt = readNextTile(tin)) != NULL){
    // Link the next tile
    tt->next = curTT->next;
    curTT->next = tt;
    curTT = tt;
  }
  return tin;
}


// 
// Read the header info from a tin file which is need to create the
// TIN structure
//
TIN *readTinFileHeader(char *path){
  FILE *inputf;
  R_POINT p;
  unsigned int pi;
  unsigned int nindex;

  // Validate input file
  if ((inputf = fopen(path, "rb"))== NULL){
     printf("tin: can't open %s\n",path);
     exit(1);
  }

  TIN *tin = (TIN*) malloc(sizeof(TIN));
   
  fread(&tin->ncols,sizeof(COORD_TYPE), 1, inputf);
  fread(&tin->nrows,sizeof(COORD_TYPE), 1, inputf);
  fread(&tin->x,sizeof(double), 1, inputf);
  fread(&tin->y,sizeof(double), 1, inputf);
  fread(&tin->cellsize,sizeof(double), 1, inputf);
  fread(&tin->numTiles,sizeof(unsigned int), 1, inputf);
  fread(&tin->numTris,sizeof(unsigned int), 1, inputf);
  fread(&tin->numPoints,sizeof(unsigned int), 1, inputf);
  fread(&tin->tl,sizeof(unsigned int), 1, inputf);
  fread(&tin->min,sizeof(ELEV_TYPE), 1, inputf);
  fread(&tin->max,sizeof(ELEV_TYPE), 1, inputf);
  fread(&tin->nodata,sizeof(ELEV_TYPE), 1, inputf);

  tin->fp = inputf;

  // create a dummy head and tail for the TIN_TILE list
  TIN_TILE *head = (TIN_TILE*)malloc(sizeof(TIN_TILE));
  assert(head);
  TIN_TILE *dummytail = (TIN_TILE*)malloc(sizeof(TIN_TILE));
  assert(dummytail);
  head->next = dummytail;
  dummytail->next = NULL;
  tin->tt = head;

  //Get to the the first tile
  fread(&p.x,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p.y,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p.z,sizeof(ELEV_TYPE), 1, tin->fp);
  fread(&pi,sizeof(unsigned int), 1, tin->fp);
  
  fread(&p.x,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p.y,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p.z,sizeof(ELEV_TYPE), 1, tin->fp);
  fread(&pi,sizeof(unsigned int), 1, tin->fp);
  
  fread(&p.x,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p.y,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p.z,sizeof(ELEV_TYPE), 1, tin->fp);
  fread(&pi,sizeof(unsigned int), 1, tin->fp);
  
  fread(&nindex,sizeof(unsigned int), 1, tin->fp);

  return tin;
}


//
// Read in the next tile from the already open tin file (tin->fp) and
// return null if eof
//
TIN_TILE *readNextTile(TIN *tin){
 
  // Don't go any further if the file is done
  if(feof(tin->fp))
    return NULL;

  R_POINT p1;
  R_POINT p2;
  R_POINT p3;
  int p1i,p2i,p3i,np1i,np2i,np3i;
  R_POINT np1;
  R_POINT np2;
  R_POINT np3;
  unsigned int index;
  unsigned int nindex;
  TRIANGLE *prevTri = NULL;
  TRIANGLE *t;
  R_POINT *pt1;
  R_POINT *pt2;
  R_POINT *pt3;
  TRIANGLE **tris;
  TIN_TILE *tt;
  R_POINT **pts; 
  int tileNull = 1;


  // Create a new tile
  //
  tt = (TIN_TILE*)malloc(sizeof(TIN_TILE));

  fread(&tt->iOffset,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&tt->jOffset,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&tt->nrows,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&tt->ncols,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&tt->numTris,sizeof(unsigned int), 1, tin->fp);
  fread(&tt->numPoints,sizeof(unsigned int), 1, tin->fp);

  tt->nodata = tin->nodata;

  // Array of triangles
  tris = (TRIANGLE**)malloc(tt->numTris*sizeof(TRIANGLE*));
  assert(tris);
  // Initialize to NULL
  int i;
  for(i=0;i<tt->numTris;i++)
    tris[i]=NULL;

  // Array of points
  pts = (R_POINT**)malloc(tt->numPoints*sizeof(R_POINT*));
  assert(pts);
  // Initialize to NULL
  for(i=0;i<tt->numPoints;i++)
    pts[i]=NULL;
      
  fread(&p1.x,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p1.y,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p1.z,sizeof(ELEV_TYPE), 1, tin->fp);
  fread(&p1i,sizeof(unsigned int), 1, tin->fp);
  
  fread(&p2.x,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p2.y,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p2.z,sizeof(ELEV_TYPE), 1, tin->fp);
  fread(&p2i,sizeof(unsigned int), 1, tin->fp);
  
  fread(&p3.x,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p3.y,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&p3.z,sizeof(ELEV_TYPE), 1, tin->fp);
  fread(&p3i,sizeof(unsigned int), 1, tin->fp);
  
  fread(&index,sizeof(unsigned int), 1, tin->fp);

  assert(p1i < tt->numPoints && p2i < tt->numPoints && p3i < tt->numPoints &&
	 index < tt->numTris);

  // Setup the first triangle
  t = (TRIANGLE*)malloc(sizeof(TRIANGLE));
  pt1 = (R_POINT*)malloc(sizeof(R_POINT));
  pt2 = (R_POINT*)malloc(sizeof(R_POINT));
  pt3 = (R_POINT*)malloc(sizeof(R_POINT));
  tris[index] = t;
  pts[p1i] = pt1;
  pts[p2i] = pt2;
  pts[p3i] = pt3;
  pt1->x = p1.x;
  pt1->y = p1.y;
  pt1->z = p1.z;
  pt2->x = p2.x;
  pt2->y = p2.y;
  pt2->z = p2.z;
  pt3->x = p3.x;
  pt3->y = p3.y;
  pt3->z = p3.z;
  t->p1 = pt1;
  t->p2 = pt2;
  t->p3 = pt3;
  t->p1p2 = NULL;
  t->p1p3 = NULL;
  t->p2p3 = NULL;

  //Find the nw and sw points of the first tri
  R_POINT *nw,*sw;
  if(pt1->x == tt->nrows-1+tt->iOffset && pt1->y == tt->jOffset){
    sw = pt1;
    if(pt2->y <= pt3->y)
      nw = pt2;
    else
      nw = pt3;
  }
  else if(pt2->x == tt->nrows-1+tt->iOffset && pt2->y == tt->jOffset){
    sw = pt2;
    if(pt1->y <= pt3->y)
      nw = pt1;
    else
      nw = pt3;
  }
  else{
    assert(pt3->x == tt->nrows-1+tt->iOffset && pt3->y == tt->jOffset);
    sw = pt3;
    if(pt2->y <= pt1->y)
      nw = pt2;
    else
      nw = pt1;
  }
      
  // Point tile to this first (lower left triangle)
  tt->t = t;
  tt->v = sw;
  tt->e.t1 = NULL;
  tt->e.t2 = NULL;
  tt->e.p1 = nw;
  tt->e.p2 = sw;
  tt->e.type = IN;

  prevTri = t;

  fread(&np1.x,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&np1.y,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&np1.z,sizeof(ELEV_TYPE), 1, tin->fp);
  fread(&np1i,sizeof(unsigned int), 1, tin->fp);
  
  fread(&np2.x,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&np2.y,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&np2.z,sizeof(ELEV_TYPE), 1, tin->fp);
  fread(&np2i,sizeof(unsigned int), 1, tin->fp);
  
  fread(&np3.x,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&np3.y,sizeof(COORD_TYPE), 1, tin->fp);
  fread(&np3.z,sizeof(ELEV_TYPE), 1, tin->fp);
  fread(&np3i,sizeof(unsigned int), 1, tin->fp);
  fread(&nindex,sizeof(unsigned int), 1, tin->fp);

    
  while(!feof(tin->fp) && !(np1i == 0 && np2i == 0 && np3i == 0)){

    assert(np1i < tt->numPoints && np2i < tt->numPoints && 
	   np2i < tt->numPoints && nindex < tt->numTris);

    tileNull = 0;
    
    // Update current points and index
    p1.x = np1.x;
    p1.y = np1.y;
    p1.z = np1.z;
    p2.x = np2.x;
    p2.y = np2.y;
    p2.z = np2.z;
    p3.x = np3.x;
    p3.y = np3.y;
    p3.z = np3.z;
    index = nindex;
    p1i = np1i;
    p2i = np2i;
    p3i = np3i;

    // Read in triangle
    fread(&np1.x,sizeof(COORD_TYPE), 1, tin->fp);
    fread(&np1.y,sizeof(COORD_TYPE), 1, tin->fp);
    fread(&np1.z,sizeof(ELEV_TYPE), 1, tin->fp);
    fread(&np1i,sizeof(unsigned int), 1, tin->fp);
    
    fread(&np2.x,sizeof(COORD_TYPE), 1, tin->fp);
    fread(&np2.y,sizeof(COORD_TYPE), 1, tin->fp);
    fread(&np2.z,sizeof(ELEV_TYPE), 1, tin->fp);
    fread(&np2i,sizeof(unsigned int), 1, tin->fp);
    
    fread(&np3.x,sizeof(COORD_TYPE), 1, tin->fp);
    fread(&np3.y,sizeof(COORD_TYPE), 1, tin->fp);
    fread(&np3.z,sizeof(ELEV_TYPE), 1, tin->fp);
    fread(&np3i,sizeof(unsigned int), 1, tin->fp);
    
    fread(&nindex,sizeof(unsigned int), 1, tin->fp);


    // Does tri exist?
    if(tris[index]==NULL){
      t = (TRIANGLE*)malloc(sizeof(TRIANGLE));

      // Find the points that already exist
      //
      if(pts[p1i]==NULL){
	pt1 = (R_POINT*)malloc(sizeof(R_POINT));
	pt1->x = p1.x;
	pt1->y = p1.y;
	pt1->z = p1.z;
	pts[p1i]=pt1;
      }
      else
	pt1 = pts[p1i];

      if(pts[p2i]==NULL){
	pt2 = (R_POINT*)malloc(sizeof(R_POINT));
	pt2->x = p2.x;
	pt2->y = p2.y;
	pt2->z = p2.z;
	pts[p2i]=pt2;
      }
      else
	pt2 = pts[p2i];

      if(pts[p3i]==NULL){
	pt3 = (R_POINT*)malloc(sizeof(R_POINT));
	pt3->x = p3.x;
	pt3->y = p3.y;
	pt3->z = p3.z;
	pts[p3i]=pt3;
      }
      else
	pt3 = pts[p3i];

      

      // Mark the triangle array
      tris[index] = t;

 
      t->p1 = pt1;
      t->p2 = pt2;
      t->p3 = pt3;
      t->p1p2 = NULL;
      t->p1p3 = NULL;
      t->p2p3 = NULL;
    }
    else{
      assert(p1.x == tris[index]->p1->x && 
	     p1.y == tris[index]->p1->y &&
	     p2.x == tris[index]->p2->x && 
	     p2.y == tris[index]->p2->y &&
	     p3.x == tris[index]->p3->x && 
	     p3.y == tris[index]->p3->y );
      t = tris[index];
      
    }

    // find which edge previous triangle shares and link them
    //
  
    // Skip edges boundary edges where the last triangle is the same
    // as the current triangle
    if(prevTri != t){
      
      // p1p2
      if(((prevTri->p1->x == p1.x && prevTri->p1->y == p1.y) &&
	  (prevTri->p2->x == p2.x && prevTri->p2->y == p2.y)) ||
	 
	 ((prevTri->p2->x == p1.x && prevTri->p2->y == p1.y) &&
	  (prevTri->p1->x == p2.x && prevTri->p1->y == p2.y)) ||

	 ((prevTri->p1->x == p1.x && prevTri->p1->y == p1.y) &&
	  (prevTri->p3->x == p2.x && prevTri->p3->y == p2.y)) ||

	 ((prevTri->p3->x == p1.x && prevTri->p3->y == p1.y) &&
	  (prevTri->p1->x == p2.x && prevTri->p1->y == p2.y)) ||

	 ((prevTri->p2->x == p1.x && prevTri->p2->y == p1.y) &&
	  (prevTri->p3->x == p2.x && prevTri->p3->y == p2.y)) ||

	 ((prevTri->p3->x == p1.x && prevTri->p3->y == p1.y) &&
	  (prevTri->p2->x == p2.x && prevTri->p2->y == p2.y))){
	t->p1p2 = prevTri;
	pointNeighborTriTo(t,t->p1,t->p2,prevTri);
      }
      // p1p3
      else if(((prevTri->p1->x == p1.x && prevTri->p1->y == p1.y) &&
	       (prevTri->p2->x == p3.x && prevTri->p2->y == p3.y)) ||

	      ((prevTri->p2->x == p1.x && prevTri->p2->y == p1.y) &&
	       (prevTri->p1->x == p3.x && prevTri->p1->y == p3.y)) ||

	      ((prevTri->p1->x == p1.x && prevTri->p1->y == p1.y) &&
	       (prevTri->p3->x == p3.x && prevTri->p3->y == p3.y)) ||

	      ((prevTri->p3->x == p1.x && prevTri->p3->y == p1.y) &&

	       (prevTri->p1->x == p3.x && prevTri->p1->y == p3.y)) ||

	      ((prevTri->p2->x == p1.x && prevTri->p2->y == p1.y) &&
	       (prevTri->p3->x == p3.x && prevTri->p3->y == p3.y)) ||

	      ((prevTri->p3->x == p1.x && prevTri->p3->y == p1.y) &&
	       (prevTri->p2->x == p3.x && prevTri->p2->y == p3.y))){
	t->p1p3 = prevTri;
	pointNeighborTriTo(t,t->p1,t->p3,prevTri);
      }
      else{
	assert(((prevTri->p1->x == p2.x && prevTri->p1->y == p2.y) &&
		(prevTri->p2->x == p3.x && prevTri->p2->y == p3.y)) ||

	       ((prevTri->p2->x == p2.x && prevTri->p2->y == p2.y) &&
		(prevTri->p1->x == p3.x && prevTri->p1->y == p3.y)) ||

	       ((prevTri->p1->x == p2.x && prevTri->p1->y == p2.y) &&
		(prevTri->p3->x == p3.x && prevTri->p3->y == p3.y)) ||

	       ((prevTri->p3->x == p2.x && prevTri->p3->y == p2.y) &&
		(prevTri->p1->x == p3.x && prevTri->p1->y == p3.y)) ||

	       ((prevTri->p2->x == p2.x && prevTri->p2->y == p2.y) &&
		(prevTri->p3->x == p3.x && prevTri->p3->y == p3.y)) ||

	       ((prevTri->p3->x == p2.x && prevTri->p3->y == p2.y) &&
		(prevTri->p2->x == p3.x && prevTri->p2->y == p3.y)));
	t->p2p3 = prevTri;
	pointNeighborTriTo(t,t->p2,t->p3,prevTri);
      }
    }
  
    prevTri = t;
  }

  free(tris);
  free(pts);

  if(tileNull){ // We didn't get a tile this time
    free(tt);
    return NULL;
  }
  else
    return tt;
}


//
// Get the index of a given point. First try the three previously
// checked points and then Binary search the sorted points array to
// find the index of the point. This assumes the array is sorted and
// each point is unique!
//
unsigned int getPointsIndex(R_POINT *p,TIN_TILE *tt,unsigned int p1,
			    unsigned int p2, unsigned int p3){
  int Low, Mid, High, comp;
   
  // Before binary searching for the point, lets see if its one of the
  // three previous points
/*   if(p1 > 0 && p1 < tt->pointsCount && p == tt->points[p1]) */
/*     return p1; */
/*   else if (p1 >= tt->pointsCount && p1 < tt->pointsCount+tt->rPointsCount && */
/* 	   p == tt->rPoints[p1-tt->pointsCount]) */
/*     return p1; */
/*   else if (p1 >= tt->pointsCount+tt->rPointsCount && */
/* 	   p == tt->bPoints[p1-tt->pointsCount-tt->rPointsCount]) */
/*     return p1; */

/*   if(p2 > 0 && p2 < tt->pointsCount && p == tt->points[p2]) */
/*     return p2; */
/*   else if (p2 >= tt->pointsCount && p2 < tt->pointsCount+tt->rPointsCount && */
/* 	   p == tt->rPoints[p2-tt->pointsCount]) */
/*     return p2; */
/*   else if (p2 >= tt->pointsCount+tt->rPointsCount && */
/* 	   p == tt->bPoints[p2-tt->pointsCount-tt->rPointsCount]) */
/*     return p2; */

/*   if(p3 > 0 && p3 < tt->pointsCount && p == tt->points[p3]) */
/*     return p3; */
/*   else if (p3 >= tt->pointsCount && p3 < tt->pointsCount+tt->rPointsCount && */
/* 	   p == tt->rPoints[p3-tt->pointsCount]) */
/*     return p3; */
/*   else if (p3 >= tt->pointsCount+tt->rPointsCount && */
/* 	   p == tt->bPoints[p3-tt->pointsCount-tt->rPointsCount]) */
/*     return p3; */
  
  //
  // The point can be in one of five arrays so we need to search each of them
  //

  // Points array
  Low = 0; High = tt->pointsCount-1;
  while( Low <= High ){
    Mid = ( Low + High ) / 2;
    comp = QS_compPoints(&p,&tt->points[Mid]);
    if(comp >= 1)
      Low = Mid + 1;
    else
      if(comp <= -1 )
	High = Mid - 1;
      else
	return Mid;  /* Found */
  }

  // rPoints array
  Low = 0; High = tt->rPointsCount-1;
  while( Low <= High ){
    Mid = ( Low + High ) / 2;
    comp = QS_compPoints(&p,&tt->rPoints[Mid]);
    if(comp >= 1)
      Low = Mid + 1;
    else
      if(comp <= -1 )
	High = Mid - 1;
      else
	return Mid+tt->pointsCount;  /* Found */
  }

  // bPoints array
  Low = 0; High = tt->bPointsCount-2;
  while( Low <= High ){
    Mid = ( Low + High ) / 2;
    comp = QS_compPoints(&p,&tt->bPoints[Mid]);
    if(comp >= 1)
      Low = Mid + 1;
    else
      if(comp <= -1 )
	High = Mid - 1;
      else
	return Mid+tt->pointsCount+tt->rPointsCount;  /* Found */
  }

  /* left tile's right boundary points skip first and last point
     (already included in arrays above) */
  if(tt->left != NULL){
     Low = 1; High = tt->left->rPointsCount-2;
     while( Low <= High ){
       Mid = ( Low + High ) / 2;
       comp = QS_compPoints(&p,&tt->left->rPoints[Mid]);
       if(comp >= 1)
	 Low = Mid + 1;
       else
	 if(comp <= -1 )
	   High = Mid - 1;
      else
	return Mid+tt->pointsCount+tt->rPointsCount+tt->bPointsCount-2; 
     }
  }
  
  /* top tile's right boundary points skip first and last point
     (already included in arrays above) */
  if(tt->top != NULL){
     Low = 1; High = tt->top->bPointsCount-2;
     while( Low <= High ){
       Mid = ( Low + High ) / 2;
       comp = QS_compPoints(&p,&tt->top->bPoints[Mid]);
       if(comp >= 1)
	 Low = Mid + 1;
       else
	 if(comp <= -1 )
	   High = Mid - 1;
      else
	if(tt->left != NULL)
	  return Mid+tt->pointsCount+tt->rPointsCount+tt->bPointsCount+
	    tt->left->rPointsCount-4;
	else
	  return Mid+tt->pointsCount+tt->rPointsCount+tt->bPointsCount-2;
     }
  }


  // The point should be found else die;
  assert(0);
  printf("Point could not be found!\n");fflush(stdout);
  exit(1);
  return 0;
  
}


//
// Write tile to a file
//
void writeTinTile(TIN_TILE *tt, char *path, short freeTriangles){
  FILE *outputf;
  unsigned int index = 0;

  // Validate output file
  if ((outputf = fopen(path, "ab"))== NULL){
     printf("tin: can't write to %s\n",path);
     perror("tin:");
     exit(1);
  }

  TRIANGLE *curT = tt->t;
  TRIANGLE *prevT = curT;

  // Store last three points
  unsigned int lpi1 = 0;
  unsigned int lpi2 = 0;
  unsigned int lpi3 = 0;
  unsigned int pi1 = 0;
  unsigned int pi2 = 0;
  unsigned int pi3 = 0;
  
  // Create an edge which 
  EDGE curE;
  curE.type = IN;
  curE.t1 = NULL;
  curE.t2 = tt->t;
  curE.p1 = tt->e.p1;
  curE.p2 = tt->e.p2;

  // Write tile header information
  // -99999 -99999 -99999 will mark the beginning of a new tile
  int marker = -9999;
  int markerCoord = 0;
  int triMarker = tt->numTris + 10;
  fwrite(&markerCoord,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&markerCoord,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&marker,sizeof(ELEV_TYPE), 1, outputf);
  fwrite(&markerCoord,sizeof(unsigned int), 1, outputf);

  fwrite(&markerCoord,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&markerCoord,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&marker,sizeof(ELEV_TYPE), 1, outputf);
  fwrite(&markerCoord,sizeof(unsigned int), 1, outputf);

  fwrite(&markerCoord,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&markerCoord,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&marker,sizeof(ELEV_TYPE), 1, outputf);
  fwrite(&markerCoord,sizeof(unsigned int), 1, outputf);

  fwrite(&triMarker,sizeof(unsigned int), 1, outputf);

  fwrite(&tt->iOffset,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&tt->jOffset,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&tt->nrows,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&tt->ncols,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&tt->numTris,sizeof(unsigned int), 1, outputf);
  fwrite(&tt->numPoints,sizeof(unsigned int), 1, outputf);

  do{
    // Print tri if we are on its IN edge
    if( (prevT->maxErrorValue >= -30000.0 || prevT->maxErrorValue <= -30003.0 )
      && prevT != NULL){
      //This is the first time we see this triangle so index it and increment
      prevT->pqIndex = index;
      index++;

      pi1 = getPointsIndex(prevT->p1,tt,lpi1,lpi2,lpi3);
      pi2 = getPointsIndex(prevT->p2,tt,lpi1,lpi2,lpi3);
      pi3 = getPointsIndex(prevT->p3,tt,lpi1,lpi2,lpi3);
      assert(pi1 < tt->numPoints && pi2 < tt->numPoints &&
	     pi3 < tt->numPoints);

      fwrite(&prevT->p1->x,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p1->y,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p1->z,sizeof(ELEV_TYPE), 1, outputf);
      fwrite(&pi1,sizeof(unsigned int), 1, outputf);

      fwrite(&prevT->p2->x,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p2->y,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p2->z,sizeof(ELEV_TYPE), 1, outputf);
      fwrite(&pi2,sizeof(unsigned int), 1, outputf);

      fwrite(&prevT->p3->x,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p3->y,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p3->z,sizeof(ELEV_TYPE), 1, outputf);
      fwrite(&pi3,sizeof(unsigned int), 1, outputf);

      fwrite(&prevT->pqIndex,sizeof(unsigned int), 1, outputf);

      lpi1 = pi1;
      lpi2 = pi2;
      lpi3 = pi3;

      prevT->maxErrorValue = -30001.0;
    }
    else if (prevT->maxErrorValue == -30001.0 && prevT != NULL){

      pi1 = getPointsIndex(prevT->p1,tt,lpi1,lpi2,lpi3);
      pi2 = getPointsIndex(prevT->p2,tt,lpi1,lpi2,lpi3);
      pi3 = getPointsIndex(prevT->p3,tt,lpi1,lpi2,lpi3);
      assert(pi1 < tt->numPoints && pi2 < tt->numPoints &&
	     pi3 < tt->numPoints);

      fwrite(&prevT->p1->x,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p1->y,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p1->z,sizeof(ELEV_TYPE), 1, outputf);
      fwrite(&pi1,sizeof(unsigned int), 1, outputf);

      fwrite(&prevT->p2->x,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p2->y,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p2->z,sizeof(ELEV_TYPE), 1, outputf);
      fwrite(&pi2,sizeof(unsigned int), 1, outputf);

      fwrite(&prevT->p3->x,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p3->y,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p3->z,sizeof(ELEV_TYPE), 1, outputf);
      fwrite(&pi3,sizeof(unsigned int), 1, outputf);

      fwrite(&prevT->pqIndex,sizeof(unsigned int), 1, outputf);

      lpi1 = pi1;
      lpi2 = pi2;
      lpi3 = pi3;
      prevT->maxErrorValue--;
    }
    else if (prevT->maxErrorValue == -30002.0 && prevT != NULL){
      
      pi1 = getPointsIndex(prevT->p1,tt,lpi1,lpi2,lpi3);
      pi2 = getPointsIndex(prevT->p2,tt,lpi1,lpi2,lpi3);
      pi3 = getPointsIndex(prevT->p3,tt,lpi1,lpi2,lpi3);
      assert(pi1 < tt->numPoints && pi2 < tt->numPoints &&
	     pi3 < tt->numPoints);

      fwrite(&prevT->p1->x,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p1->y,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p1->z,sizeof(ELEV_TYPE), 1, outputf);
      fwrite(&pi1,sizeof(unsigned int), 1, outputf);

      fwrite(&prevT->p2->x,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p2->y,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p2->z,sizeof(ELEV_TYPE), 1, outputf);
      fwrite(&pi2,sizeof(unsigned int), 1, outputf);

      fwrite(&prevT->p3->x,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p3->y,sizeof(COORD_TYPE), 1, outputf);
      fwrite(&prevT->p3->z,sizeof(ELEV_TYPE), 1, outputf);
      fwrite(&pi3,sizeof(unsigned int), 1, outputf);

      fwrite(&prevT->pqIndex,sizeof(unsigned int), 1, outputf);
      
      if(freeTriangles &&  prevT != tt->t){
	lpi1 = 0;
	lpi2 = 0;
	lpi3 = 0;
	removeTri(prevT);
      }
      prevT->maxErrorValue--;
    }
    else {
      assert(0);
      char str[100];
      sprintf(str,"Possible bad tin output %s\n",ELEV_TYPE_PRINT_CHAR);
      printf(str, prevT->maxErrorValue);
      exit(1);
    }

    // Go to next edge
    prevT=curT;
    curT = nextEdge(curT,tt->v,&curE,tt);
    assert(curT);
    
  }while((curE.p1 != tt->e.p1 || curE.p2 != tt->e.p2) && 
	 (curE.p1 != tt->e.p2 || curE.p2 != tt->e.p1));

  /* Index should not have gotten higher than number of triangles */
  assert(index <= tt->numTris);
  
  // Free all points for this tile
  //
  if(freeTriangles){
    // Remove last triangle
    removeTri(tt->t);
        
    // Free tiles points
    int i = 0;
    for(i = 0;i < tt->pointsCount; i++)
      if( !(tt->points[i]->x == tt->iOffset &&
	    tt->points[i]->y == tt->jOffset))
	free(tt->points[i]);
    free(tt->points);
    tt->points = NULL;
    
    //
    // IMPORTANT: The following code assumes that the rPoints and
    // bPoints arrays are sorted in ascending order x and y
    // repectively
    //
    
    //Free point pointer arrary to the left
    if(tt->left != NULL){
      // Assuming this is sorted we don't want to free the last point
      // as it is shared by the bPoints array
      for(i = 1;i < tt->left->rPointsCount-1; i++){
	free(tt->left->rPoints[i]);
      }
      free(tt->left->rPoints);
      tt->left->rPoints = NULL;
    }
    
    // Free tt's right list if this is the right most tile
    if(tt->right == NULL){
      // Assuming this is sorted we don't want to free the last point
      // as it is shared by the bPoints array
      for(i = 0;i < tt->rPointsCount-1; i++){
	free(tt->rPoints[i]);
      }
      free(tt->rPoints);
      tt->rPoints = NULL;
    }
    
    //Free point pointer array for tile above
    if(tt->top != NULL){
      for(i = 0;i < tt->top->bPointsCount-1; i++){
	free(tt->top->bPoints[i]);
      }
      free(tt->top->bPoints);
      tt->top->bPoints = NULL;
    }
    
    //Free tt's bottom array if it is on the bottom
    if(tt->bottom == NULL){
      for(i = 0;i < tt->bPointsCount-1; i++){
	free(tt->bPoints[i]);
      }
      free(tt->bPoints);
      tt->bPoints = NULL;
    }

  }
  // Close file
  fclose(outputf);
}


//
// write tin file to a given filename, if headerOnly is set to one
// then don't wrtie the tile just write the header.
//
void writeTin(TIN *tin,char *path,short headerOnly){
  FILE *outputf;

  // Validate output file
  if ((outputf = fopen(path, "wb"))== NULL){
    fprintf(stderr, "writeTin: can't write to %s ",path);
    perror("writeTin:");
    exit(1);
  }
  
  // create elevation output format for min and max
  char str[100];
  sprintf(str,"%%s\t%s\n",ELEV_TYPE_PRINT_CHAR);

  // Write tin info
  fwrite(&tin->ncols,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&tin->nrows,sizeof(COORD_TYPE), 1, outputf);
  fwrite(&tin->x,sizeof(double), 1, outputf);
  fwrite(&tin->y,sizeof(double), 1, outputf);
  fwrite(&tin->cellsize,sizeof(double), 1, outputf);
  fwrite(&tin->numTiles,sizeof(unsigned int), 1, outputf);
  fwrite(&tin->numTris,sizeof(unsigned int), 1, outputf);
  fwrite(&tin->numPoints,sizeof(unsigned int), 1, outputf);
  fwrite(&tin->tl,sizeof(unsigned int), 1, outputf);
  fwrite(&tin->min,sizeof(ELEV_TYPE), 1, outputf);
  fwrite(&tin->max,sizeof(ELEV_TYPE), 1, outputf);
  fwrite(&tin->nodata,sizeof(ELEV_TYPE), 1, outputf);

  // Close file
  fclose(outputf);
  
  // If we are not just writing the header then loop through and
  // output every tile
  if(!headerOnly){
    TIN_TILE *tt = tin->tt->next;
    while(tt->next != NULL){
      writeTinTile(tt,path,1);
        tt = tt->next;
        assert(tt);
      }
  }

}




