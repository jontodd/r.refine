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
 * grass.c has functions that handle the output of sites and vectors
 * and the input of rasters to both grids and tiled grids
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-11
 *
 * COMMENTS:
 *
 *****************************************************************************/

#include "grass.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>



#define INTERNAL_NODATA_VALUE -9999

/* write the header for a sites file */
void writeSitesHeader(FILE *sitesFile, char *siteFileName){

  {
  Site_head shead;
  char buf[320];

  shead.name = G_store(siteFileName);
  shead.desc = G_store(G_recreate_command());
   
  shead.time = (struct TimeStamp*)NULL;

  shead.form = shead.labels = shead.stime = (char *)NULL;

  shead.form = G_store("||#");
  sprintf(buf,"Easting|Northing|#%s","No Label");
  shead.labels = G_store(buf);
    

  G_site_put_head (sitesFile, &shead);
  free (shead.name);
  free (shead.desc);
  free (shead.form);
  free (shead.labels);
  }
  
}

/* write a tile to a sites file */
void writeSitesTile(TIN_TILE *tt,FILE *sitesFile, char* sitesFileName){
  assert(sitesFile && tt);
  struct Cell_head w;
  Site *s;
  int i;
  G_get_set_window (&w);

  printf ("write TIN tile to sites file %s\n", sitesFileName);fflush(stdout);


  if(NULL == (s = G_site_new_struct(DCELL_TYPE,2,0,0)))
    G_fatal_error("memory allocation failed for site");
 
  for(i=0;i<tt->pointsCount;i++){
    s->north = G_row_to_northing((double)(tt->points[i]->x +.5), &w);
    s->east = G_col_to_easting((double)(tt->points[i]->y +.5), &w); 
    s->dcat = (double)tt->points[i]->z;
    G_site_put(sitesFile, s);
    G_percent(i, tt->pointsCount - 1, 2);
  }

  // Handle Special Boundary cases
  //

    for(i = 0;i < tt->rPointsCount; i++){
      s->north = G_row_to_northing((double)(tt->rPoints[i]->x +.5), &w);
      s->east = G_col_to_easting((double)(tt->rPoints[i]->y +.5), &w); 
      s->dcat = (double)tt->rPoints[i]->z;
      G_site_put(sitesFile, s);
    }

    for(i = 0;i < tt->bPointsCount; i++){
      s->north = G_row_to_northing((double)(tt->bPoints[i]->x +.5), &w);
      s->east = G_col_to_easting((double)(tt->bPoints[i]->y +.5), &w); 
      s->dcat = (double)tt->bPoints[i]->z;
      G_site_put(sitesFile, s);
    }

  assert(s); 
  G_site_free_struct (s);
  assert(sitesFile);
}

/* GRASS function for writing a default file header */
static void _set_default_head_info (struct dig_head *head)
{
	struct Cell_head wind;
	char *organization;

	if (getenv("GRASS_ORGANIZATION"))    /* added MN 12/2001 */
	{
		organization=(char *)getenv("GRASS_ORGANIZATION");
		sprintf(head->organization, "%s", organization);
	}
	else
		strcpy(head->organization, "GRASS Development Team") ;

	G_get_window (&wind) ;
        head->W = wind.west;
        head->E = wind.east;
        head->N = wind.north;
        head->S = wind.south;
	head->plani_zone = G_zone ();
	sprintf (head->line_3, "Projection: %s", 
			G_database_projection_name());

        /* avoid 1:0 scale */
        head->orig_scale = 1;
	                
}

/* GRASS function for writing a default file header */
void set_default_head_info (struct dig_head *head)
{
	time_t ticks = time (NULL) ;
	struct tm *theTime = localtime (&ticks) ;

	/* Date in ISO 8601 format YYYY-MM-DD */
	strftime (head->date, 20, "%Y-%m-%d", theTime) ;
	/* free (theTime) ; */

	_set_default_head_info (head);
	
	/* Arbitrary scale */
	head->orig_scale = 24000;

}

/* write a tile to a vector file */
void writeVectorTile(struct Map_info *Map, TIN_TILE *tt){

  int type, count;
  int alloc_points, n_points ;
  double *xarray, *yarray;
  struct line_pnts *Points ;
  struct Cell_head w;

  G_get_set_window (&w);

  printf ("write TIN tile to vect file %s\n", Map->name);fflush(stdout);

  /* Initialize the Point structure, ONCE */
  Points = Vect_new_line_struct();
  /* allocat for DOT type */
  type = DOT;
  alloc_points = 3;
  xarray = (double *) dig_falloc(alloc_points, sizeof(double));
  yarray = (double *) dig_falloc(alloc_points, sizeof(double));

  n_points = 2;
  count = 0;

  // Traverse the TIN crossing over every edge
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
    
    
    xarray[0] = G_col_to_easting((double)curE.p1->y +.5, &w);
    xarray[1] = G_col_to_easting((double)curE.p2->y +.5, &w);
    yarray[0] = G_row_to_northing((double)curE.p1->x +.5, &w);
    yarray[1] = G_row_to_northing((double)curE.p2->x +.5, &w);
    count++;
      
    /* make a vector dig record */
    if (0 > Vect_copy_xy_to_pnts (Points, xarray, yarray, n_points))
      G_fatal_error ("Vect_copy error\n");

    Vect_write_line (Map,  (unsigned int) type, Points);
    
    
    // Go to next edge
    prevT = curT;
    curT = nextEdge(curT,tt->v,&curE,tt);
    assert(curT);
    
  } while((curE.p1 != tt->e.p1 || curE.p2 != tt->e.p2) && 
	  (curE.p1 != tt->e.p2 || curE.p2 != tt->e.p1));    
}

/* read a raster into a grid */
GRID* raster2grid(char* gridname, int nrows, int ncols) {

  printf("raster2grid: reading raster %s...", gridname);

  COORD_TYPE i,j;
  GRID *g = (GRID *) malloc(sizeof(GRID));
  g->name = gridname;
  g->nodata = INTERNAL_NODATA_VALUE;

  // Check Sizes
  if(nrows > COORD_TYPE_MAX || ncols > COORD_TYPE_MAX){
    printf("raster: Too many rows or columns. Change type of COORD.");
    exit(1);
  }
  g->nrows = nrows;
  g->ncols = ncols;
  
  // Dynamically allocate array for data
  if((g->data = (ELEV_TYPE**) malloc((g->nrows)*sizeof(ELEV_TYPE*)))==NULL){
     printf("raster: insufficient memory");
     exit(1);
  }
  assert(g->data);
  for(i=0;i<g->nrows;i++){
    if((g->data[i] = (ELEV_TYPE*) malloc((g->ncols)*sizeof(ELEV_TYPE)))==NULL){
      printf("raster: insufficient memory");
      exit(1);
    }
    assert(g->data[i]);
  }

  
  char *mapset;
  mapset = G_find_cell (gridname, "");
  if (mapset == NULL) {
    G_fatal_error ("cell file [%s] not found", gridname);
    exit(1);
  }

  /* open map */
  int infd;
  if ( (infd = G_open_cell_old (gridname, mapset)) < 0)
    G_fatal_error ("Cannot open cell file [%s]", gridname);
  
  /* determine map type (CELL/FCELL/DCELL) */
  RASTER_MAP_TYPE data_type;
  data_type = G_raster_map_type(gridname, mapset);
  
  /* Allocate input buffer */
  void *inrast;
  inrast = G_allocate_raster_buf(data_type);
 
  CELL c;
  FCELL f;
  DCELL d;
  ELEV_TYPE x;
  int isnull = 0;
  g->min = 9999;
  g->max = 0;
  for (i = 0; i< nrows; i++) {
    
    /* read input map */
    if (G_get_raster_row (infd, inrast, i, data_type) < 0)
      G_fatal_error ("Could not read from <%s>, row=%d",gridname,i);
    
    for (j=0; j<ncols; j++) {
      
      switch (data_type) {
      case CELL_TYPE:
	c = ((CELL *) inrast)[j];
	isnull = G_is_c_null_value(&c);
	if (!isnull) {
	  x = (ELEV_TYPE)c;  d = (DCELL)c;
	}
	break;
      case FCELL_TYPE:
	f = ((FCELL *) inrast)[j];
	isnull = G_is_f_null_value(&f);
	if (!isnull) {
	  x = (ELEV_TYPE)f; d = (DCELL)f;
	}
	break;
      case DCELL_TYPE:
	d = ((DCELL *) inrast)[j];
	isnull = G_is_d_null_value(&d);
	if (!isnull) {
	  x = (ELEV_TYPE)d;
	}
	break;
      default:
	G_fatal_error("raster type not implemented");
      }

      /* handle null values */
      if (isnull) {
	x = INTERNAL_NODATA_VALUE;
      } else {
	/* check range */
	if ((d > (DCELL)ELEV_TYPE_MAX) || (d < (DCELL)(ELEV_TYPE_MIN))) {
	  fprintf(stderr, "reading cell file %s at (i=%d,j=%d) value=%.1f\n",
		  gridname, i, j, d);
		  G_fatal_error("value out of range.");
		}
      }
      /* set check and set value */
      if(x < g->min && x != g->nodata)
	g->min = x;
      if(x > g->max)
	g->max = x;
      g->data[i][j] = x;
      //printf("%d ", x);
      
    } /* for j */
    //printf("\n");

    G_percent(i, nrows, 2);
  }/* for i */
  
  /* delete buffers */
  G_free(inrast);
  /* close map files */
  G_close_cell (infd);

  printf(".done\n");
  return g;
}

/* read a raster into a tiled grid */
TILED_GRID* raster2tiledGrid(char* gridname, int nrows, int ncols,
			     unsigned int TL) {

  printf("raster2grid: reading raster %s..", gridname);

  COORD_TYPE i,j;
  int iNumTiles,jNumTiles;
  int fd = -1;
  long value;
  TILED_GRID *g = (TILED_GRID *) malloc(sizeof(TILED_GRID));
  g->name = gridname;
  g->nodata = INTERNAL_NODATA_VALUE;

  // Check Sizes
  if(nrows > COORD_TYPE_MAX || ncols > COORD_TYPE_MAX){
    printf("raster: Too many rows or columns. Change type of COORD.");
    exit(1);
  }
  g->nrows = nrows;
  g->ncols = ncols;
  g->TL = TL;

  
  char *mapset;
  mapset = G_find_cell (gridname, "");
  if (mapset == NULL) {
    G_fatal_error ("cell file [%s] not found", gridname);
    exit(1);
  }

  /* open map */
  int infd;
  if ( (infd = G_open_cell_old (gridname, mapset)) < 0)
    G_fatal_error ("Cannot open cell file [%s]", gridname);
  
  /* determine map type (CELL/FCELL/DCELL) */
  RASTER_MAP_TYPE data_type;
  data_type = G_raster_map_type(gridname, mapset);
  
  /* Allocate input buffer */
  void *inrast;
  inrast = G_allocate_raster_buf(data_type);

  // Decide how many tiles there will be
  jNumTiles = ceil( ((double) g->ncols)/ ((double) TL-1));
  iNumTiles = ceil( ((double) g->nrows)/ ((double) TL-1));

  // Allocate the file pointers
  g->files = (FILE***)malloc(iNumTiles * sizeof(FILE**));
  assert(g->files);
  for(i=0;i<iNumTiles;i++){
    g->files[i] = (FILE**)malloc(jNumTiles * sizeof(FILE*));
    assert(g->files[i]);
  }

  // Open random tmp files for each tile
  for (i = 0; i < iNumTiles; i++){
    for (j = 0; j < jNumTiles; j++){
      char template[] = "/tmp/tile.XXXXXX";
      if((fd = mkstemp(template)) == -1){
	perror("mkstemp failed!");
	exit(1);
      }
      
     if((g->files[i][j] = fdopen(fd,"w+b")) == NULL){
      perror("fdopen failed!");
      exit(1);
     }
    }
  }
 
  CELL c;
  FCELL f;
  DCELL d;
  ELEV_TYPE x;
  int isnull = 0;
  g->min = 9999;
  g->max = 0;
  int ti,ti1 = -1,tj,tj1 = -1;

  for (i = 0; i< nrows; i++) {
    
    /* read input map */
    if (G_get_raster_row (infd, inrast, i, data_type) < 0)
      G_fatal_error ("Could not read from <%s>, row=%d",gridname,i);
    
    for (j=0; j<ncols; j++) {
      
      switch (data_type) {
      case CELL_TYPE:
	c = ((CELL *) inrast)[j];
	isnull = G_is_c_null_value(&c);
	if (!isnull) {
	  x = (ELEV_TYPE)c;  d = (DCELL)c;
	}
	break;
      case FCELL_TYPE:
	f = ((FCELL *) inrast)[j];
	isnull = G_is_f_null_value(&f);
	if (!isnull) {
	  x = (ELEV_TYPE)f; d = (DCELL)f;
	}
	break;
      case DCELL_TYPE:
	d = ((DCELL *) inrast)[j];
	isnull = G_is_d_null_value(&d);
	if (!isnull) {
	  x = (ELEV_TYPE)d;
	}
	break;
      default:
	G_fatal_error("raster type not implemented");
      }

      /* handle null values */
      if (isnull) {
	x = INTERNAL_NODATA_VALUE;
      } else {
	/* check range */
	if ((d > (DCELL)ELEV_TYPE_MAX) || (d < (DCELL)(ELEV_TYPE_MIN))) {
	  fprintf(stderr, "reading cell file %s at (i=%d,j=%d) value=%.1f\n",
		  gridname, i, j, d);
		  G_fatal_error("value out of range.");
		}
      }
      /* set check and set value */
      if(x < g->min && x != g->nodata)
	g->min = x;
      if(x > g->max)
	g->max = x;
      
      // Map i,j to tile(s)
      if(i != 0 && i % (TL-1) == 0) // If on a boundary
	ti1 = (i-1)/(TL-1);
      ti = i/(TL-1);
      if(j != 0 && j % (TL-1) == 0) // If on a boundary
	tj1 = (j-1)/(TL-1);
      tj = j/(TL-1);
      value = x;
      writeElevToTile(g->files[ti][tj],(ELEV_TYPE)value);
      if(ti1 != -1){
	writeElevToTile(g->files[ti1][tj],(ELEV_TYPE)value);
	if(tj1 != -1)
	  writeElevToTile(g->files[ti1][tj1],(ELEV_TYPE)value);
      }
      if(tj1 != -1)
	writeElevToTile(g->files[ti][tj1],(ELEV_TYPE)value);
      
      ti1 = tj1 = -1;
      
    } /* for j */

    G_percent(i, nrows, 2);
  }/* for i */
  
  /* delete buffers */
  G_free(inrast);
  /* close map files */
  G_close_cell (infd);

  printf("..done\n");
  return g;
}
