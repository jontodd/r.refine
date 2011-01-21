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
 * grid.c contains functions for reading in and outputing arc-ascii
 * grids (rasters)
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS:
 *
 *****************************************************************************/

#include "grid.h"


//
// min - return the min of two ints
//
#define MIN(x,y) ( x < y ? x : y )


//
// Write an elevation value to a file
//
void writeElevToTile(FILE *fp,ELEV_TYPE z){
  
  if(fwrite(&z, sizeof(ELEV_TYPE), 1, fp) < 1) {
    fprintf(stderr, "error: cannot write to file\n");
    exit(1);
  }
}


//
// Read a arc-ascii grid file into a set of tile files. This way we
// don't read the data into memory but instead seperate it into tile
// which we can work on one by one
//
TILED_GRID *readGrid2Tile(char *path, unsigned int TL){
  FILE *inputf;
  COORD_TYPE i,j;
  long value, resolution; 
  int fd = -1;
  int iNumTiles,jNumTiles;


  // Validate input file
  if ((inputf = fopen(path, "rb"))== NULL){
     printf("grid: can't open %s\n",path);
     exit(1);
  }

  // Input file header info into the grid structure
  TILED_GRID *g = (TILED_GRID *) malloc(sizeof(TILED_GRID));
  g->name = path;
  char scanString[200] = "%*s%lu%*s%lu%*s%lf%*s%lf%*s%lf%*s";
  int scan = fscanf(inputf,
		    strcat(scanString,ELEV_TYPE_PRINT_CHAR),
		    &g->ncols,&g->nrows,&g->x,
		    &g->y,&g->cellsize,&g->nodata);

  
  g->TL = TL;
  if(scan==0){
    printf("grid: trouble reading input file!\n");
    exit(1);
  }
  
  if(g->nrows > COORD_TYPE_MAX || g->ncols > COORD_TYPE_MAX){
     printf("grid: Too many rows or columns. Change type of COORD.");
     exit(1);
  }

  // Decide how many tiles there will be
  jNumTiles = ceil( ((double) g->ncols)/ ((double) TL-1));
  iNumTiles = ceil( ((double) g->nrows)/ ((double) TL-1));
  printf("numtile=[%d, %d]\n", iNumTiles, jNumTiles);
  fflush(stdout); 

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

  int ti,ti1 = -1,tj,tj1 = -1;

  // Put data into the files and calculate max & min
  g->min = 9999;
  g->max = 0;
  for(i=0;i<g->nrows;i++){
    for(j=0;j<g->ncols;j++){
      if(fscanf(inputf,"%ld",&value)!=EOF){
	if(value < g->min && value != g->nodata)
	  g->min = value;
	if(value > g->max)
	  g->max = value;
	if(value > ELEV_TYPE_MAX || value < ELEV_TYPE_MIN){
	  printf("grid: grid value is out of range. Value %ld Size of COORD: "
		 "%lu \n"
		 ,value,sizeof(COORD_TYPE));
	  exit(1);
	}
	// Map i,j to tile(s)
	if(i != 0 && i % (TL-1) == 0) // If on a boundary
	  ti1 = (i-1)/(TL-1);
	ti = i/(TL-1);
	if(j != 0 && j % (TL-1) == 0) // If on a boundary
	  tj1 = (j-1)/(TL-1);
	tj = j/(TL-1);

	writeElevToTile(g->files[ti][tj],(ELEV_TYPE)value);
	if(ti1 != -1){
	  writeElevToTile(g->files[ti1][tj],(ELEV_TYPE)value);
	  if(tj1 != -1)
	    writeElevToTile(g->files[ti1][tj1],(ELEV_TYPE)value);
	}
	if(tj1 != -1)
	  writeElevToTile(g->files[ti][tj1],(ELEV_TYPE)value);

	ti1 = tj1 = -1;

      }
      else{
	printf("grid: data file is corrupt");
	exit(1);
      }
    }
  }

  // Set resolution as a function of gridsize
  resolution = MIN(g->ncols,g->nrows)/8;

  // Close file
  fclose(inputf);

  // Return 2d File pointer array
  return g;

}


//
// bring grid file into an array
//
GRID *importGrid(char *path)
{
  FILE *inputf;
  COORD_TYPE i,j;
  long value, resolution;

  // Validate input file
  if ((inputf = fopen(path, "r"))== NULL){
     printf("grid: can't open %s\n",path);
     exit(1);
  }

  // Input file info into the grid structure
  GRID *g = (GRID *) malloc(sizeof(GRID));
  g->name = path;
  char scanString[200] = "%*s%lu%*s%lu%*s%lf%*s%lf%*s%lf%*s";
  int scan = fscanf(inputf,
		    strcat(scanString,ELEV_TYPE_PRINT_CHAR),
		    &g->ncols,&g->nrows,&g->x,
		    &g->y,&g->cellsize,&g->nodata);
  if(scan==0){
    printf("grid: trouble reading input file!\n");
    exit(1);
  }

  if(g->nrows > COORD_TYPE_MAX || g->ncols > COORD_TYPE_MAX){
     printf("grid: Too many rows or columns. Change type of COORD.");
     exit(1);
  }
    

  // Dynamically allocate array for data
  if((g->data = (ELEV_TYPE**) malloc((g->nrows)*sizeof(ELEV_TYPE*)))==NULL){
     printf("grid: insufficient memory");
     exit(1);
  }
  assert(g->data);
  for(i=0;i<g->nrows;i++){
    if((g->data[i] = (ELEV_TYPE*) malloc((g->ncols)*sizeof(ELEV_TYPE)))==NULL){
      printf("grid: insufficient memory");
      exit(1);
    }
    assert(g->data[i]);
  }
  
  // Put data into the array and calculate max & min
  g->min = 9999;
  g->max = 0;
  for(i=0;i<g->nrows;i++){
    for(j=0;j<g->ncols;j++){
      if(fscanf(inputf,"%ld",&value)!=EOF){
	if(value < g->min && value != g->nodata)
	  g->min = value;
	if(value > g->max)
	  g->max = value;
	if(value > ELEV_TYPE_MAX || value < ELEV_TYPE_MIN){
	  printf("grid: grid value is out of range. Value %ld Size of COORD: "
		 "%lu \n"
		 ,value,sizeof(COORD_TYPE));
	  exit(1);
	}
	  
	g->data[i][j]=(ELEV_TYPE)value;
      }
      else{
	printf("grid: data file is corrupt");
	exit(1);
      }
    }
  }

  // Set resolution as a function of gridsize
  resolution = MIN(g->ncols,g->nrows)/8;

  // Close file
  fclose(inputf);

  // Return grid
  return g;
}


//
// write grid structure to file path
//
void writeGrid(GRID *g,char *path){
  FILE *outputf;
  COORD_TYPE i,j;

  // Validate output file
  if ((outputf = fopen(path, "w"))== NULL){
     printf("grid: can't write to %s\n",path);
     exit(1);
  }
  
  // Write grid info
  fprintf(outputf,"ncols\t\t%lu\n",g->ncols);
  fprintf(outputf,"nrows\t\t%lu\n",g->nrows);
  fprintf(outputf,"xllcorner\t%f\n",g->x);
  fprintf(outputf,"yllcorner\t%f\n",g->y);
  fprintf(outputf,"cellsize\t%f\n",g->cellsize);
  char str[100];
  sprintf(str,"NODATA_value\t%s\n",ELEV_TYPE_PRINT_CHAR);
  fprintf(outputf,str,g->nodata);

  // Write data
  for(i=0;i<g->nrows;i++){
    for(j=0;j<g->ncols;j++){
      fprintf(outputf, strcat(ELEV_TYPE_PRINT_CHAR," "),g->data[i][j]);
    }
    fprintf(outputf, "\n");
  }

  // Close file
  fclose(outputf);
}


//
// Print the grid info 
//
void printGrid(GRID *g){
  char str[100];
  sprintf(str,"%%s: %s\n",ELEV_TYPE_PRINT_CHAR);

  printf("\nGrid Info:\n\n");
  printf("Name: %s\n",g->name);
  printf("Number of Columns: %lu\n",g->ncols);
  printf("Number of Rows: %lu\n",g->nrows);
  printf("Lat Lon X Corner: %f\n",g->x);
  printf("Lat Lon Y Corner: %f\n",g->y);
  printf("Cell size: %f\n",g->cellsize);
  printf(str,"No data value",g->nodata);
  printf(str,"Min value",g->min);
  printf(str,"Max value",g->max);
}


//
// free memory from grid
//
void freeGrid(GRID *g){
  free(g->name);
  COORD_TYPE i;
  for(i=0;i < g->nrows; i++)
    free(g->data[i]);
  free(g->data);

  free(g);
}

