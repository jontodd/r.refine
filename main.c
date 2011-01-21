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
 * main.c contains the main function for r.refine, controls the high
 * level working of the program, and parses arguments from the user.
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-11
 *
 * COMMENTS:
 *
 * r.refine can either be compiled for a GRASS or for standalone
 * modes. In order to compile for grass the __GRASS__ flag must be
 * defined in the makefile.
 *
 *****************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <strings.h>

#include "rtimer.h"
#include "refine_tin.h"
#include "render_tin.h" 

#ifdef __GRASS__
#include "grass.h"
#endif

// global tin needed for rendering in OpenGL 
extern TIN *tinGlobal;

// parse arguments from the user 
void parse_args(int argc, char *argv[],double *err,double *mem,
		int *useNoData, int *delaunay, int *render,
		char **outputFile,char **inputFile,
		char **outputSites, char** outputVect);


int main(int argc, char** argv) {

  Rtimer refineTime;
  Rtimer totalTime;
  Rtimer outputTime;
  
  char buf1[1000];
  char buf2[1000];
  char buf3[1000];

  tinGlobal = NULL;
  TILED_GRID *gridFile = NULL;

  BOOL doRefine = 0; // Should we refine? Default: No 
  
  // default refinement parameters 
  double err = 1;           // Default error = 1% 
  double errAmt = 0.01;     // Default error = 1% 
  double mem = 250;         // Default memory size 250mb 
  int useNoData = 0;        // Default to not use nodata points 
  int delaunay = 1;         // Default to use delaunay 
  int doRender = 0;         // Default to not render the tin 
  char *outputFile = NULL;  // File to save tin as 
  char *inputFile = NULL;   // Input grid file 
  char *outputSites = NULL; // Output filename for sites 
  char *outputVect = NULL; // Output filename for  vectors 

  //
  //  GRASS only initializations
  // 
#ifdef __GRASS__

   // initialize GIS library 
  G_gisinit(argv[0]);

  struct GModule *module;
  module = G_define_module();
  module->description ="r.refine: scalable raster-to-TIN simplification.";

  // get the current region and dimensions 
  struct Cell_head * region;
  region = (struct Cell_head*)malloc(sizeof(struct Cell_head));
  assert(region);
  if (G_get_set_window(region) == -1) {
    G_fatal_error("r.refine: error getting current region");
  }
  int nr = G_window_rows();
  int nc = G_window_cols();

  fprintf(stderr, "region size is %d x %d\n", nr, nc);

#endif


  // get parameters from user arguments 
  parse_args(argc,argv,&err,&mem,&useNoData,&delaunay,&doRender,&outputFile,
	     &inputFile,&outputSites, &outputVect);

  // import from grid if we have an inputFile 
  if(inputFile != NULL){
#ifdef __GRASS__
    gridFile = raster2tiledGrid(inputFile,nr,nc,getTileLength(mem));
#else
    gridFile = readGrid2Tile(inputFile,getTileLength(mem));
#endif
  }

  // start total timer 
  rt_start(totalTime);

  // initialize a tin if we have a grid 
  if(gridFile != NULL){
    doRefine = 1;
    tinGlobal = initTin(gridFile, err, mem,useNoData,outputFile);
  }

  // if we just initialized the tin then we will refine it 
  if(doRefine){
    // start refine timer 
    rt_start(refineTime);

    // make err a percentage of the max and min elevations 
    errAmt = ((double)(tinGlobal->max - tinGlobal->min)) * (err/100.0);

    // refine the tin 
    refineTin(errAmt,delaunay,tinGlobal,outputFile,outputSites,outputVect,useNoData);
    
    // stop timers and print their values to a buffer for output 
    rt_stop(refineTime);
    rt_stop(totalTime);
    rt_sprint(buf1,totalTime);
    rt_sprint(buf2,refineTime);
    rt_sprint(buf3,outputTime);

    // report the stats 
    printf(".......DONE........\n");
    //printf("%s\n", argv[1]);
    printf("err=%.2f%c absErr=%.2f  mem=%.2fMB numTiles=%d\n",  
	   err,'%', errAmt, mem, tinGlobal->numTiles);
    printf("raster: %ld points\n",(long)tinGlobal->nrows*tinGlobal->ncols);
    printf("TIN: triangles=%ld points=%ld\n", 
	   (long)tinGlobal->numTris, 
	   (long)tinGlobal->numPoints);
    printf("total time: %s\n", buf1);
  }
  
  // if user wants to render then we pass control to OpenGL. Once
  // control is passed there is no way to execute further code so this
  // should always be done last unless using threads
  if(doRender){
    tinGlobal = readTinFile(outputFile);
    OGL_init(&argc,argv);
    OGL_render(argc, argv);
  }

  return 0;
}


//
// parse_args code for the GRASS and stand alone versions
// 

#ifdef __GRASS__

void parse_args(int argc, char *argv[],double *err,double *mem,
		int *useNoData, int *delaunay, int *render,
		char **outputFile,char **inputFile,
		char **outputSites, char** outputVect) {

// input grid  
  struct Option *input_grid;
  input_grid = G_define_option() ;
  input_grid->key        = "grid";
  input_grid->type       = TYPE_STRING;
  input_grid->required   = YES;
  input_grid->gisprompt  = "old,cell,raster" ;
  input_grid->description= "Input raster" ;

// error threshold value, in percentage
  struct Option *epsilon;
  epsilon = G_define_option();
  epsilon->key  = "epsilon";
  epsilon->type = TYPE_DOUBLE;
  epsilon->required = NO;
  epsilon->answer = "1.0"; // default value 
  epsilon->description = "Error threshold, in percentage of max elevation";

// output tin  
  struct Option *output_file;
  output_file = G_define_option() ;
  output_file->key        = "tin";
  output_file->type       = TYPE_STRING;
  output_file->required   = NO;
  output_file->answer     = "output.tin";
  output_file->description= "Output TIN file" ;

// output sites  
  struct Option *output_sites;
  output_sites = G_define_option() ;
  output_sites->key        = "output_sites";
  output_sites->type       = TYPE_STRING;
  output_sites->required   = NO;
  output_sites->answer     = "NULL";
  output_sites->gisprompt  = "new,site_lists,sites";
  output_sites->description= "Name of output sites file." ;

// output sites  
  struct Option *output_vect;
  output_vect = G_define_option() ;
  output_vect->key        = "output_vect";
  output_vect->type       = TYPE_STRING;
  output_vect->required   = NO;
  output_vect->answer     = "NULL";
  output_vect->gisprompt  = "new,vector,vector";
  output_vect->description= "Name of output vector file." ;


  
 // main memory 
  struct Option *memory;
  memory = G_define_option() ;
  memory->key         = "memory";
  memory->type        = TYPE_INTEGER;
  memory->required    = NO;
  memory->answer      = "500"; // 300MB default value 
  memory->description = "Main memory size (in MB)";

  // Use Delaunay ? 
  struct Flag *del;
  del = G_define_flag() ;
  del->key         = 'd';
  del->description = "Do NOT use Delaunay triangulation";
 
  // Use nodata points? 
  struct Flag *no_data;
  no_data = G_define_flag() ;
  no_data->key         = 'n';
  no_data->description = "Include nodata points (in simplification)";

  // render tin ?
  struct Flag *render_tin;
  render_tin = G_define_flag() ;
  render_tin->key        = 'r';
  render_tin->description= "Render TIN in OpenGL" ;



  if (G_parser(argc, argv)) {
    exit (-1);
  }

  // print out 
  *err =  strtod(epsilon->answer, NULL);
  *mem = strtol(memory->answer,NULL,10);
  *inputFile = input_grid->answer;
  *outputFile = output_file->answer;
  if (strcmp("NULL", output_sites->answer) == 0) 
    *outputSites = NULL;
  else *outputSites =   output_sites->answer;

  if (strcmp("NULL", output_vect->answer) == 0) 
    *outputVect = NULL;
  else *outputVect =   output_vect->answer;
  
  //default is 0
  if (no_data->answer) *useNoData = 1;
  
  //default is 1
  if (del->answer) *delaunay = 0;

  //default is 0
  if (render_tin->answer) *render = 1;

  printf("%s grid=%s output=%s output-sites=%s outputVect=%s "
	 "error=%.2f mem=%.2f delaunay=%d no_data=%d render=%d\n",
	 argv[0], *inputFile, *outputFile, *outputSites, *outputVect,
	 *err, *mem, *delaunay, *useNoData, *render);
}

#else

void parse_args(int argc, char *argv[],double *err,double *mem,
		int *useNoData, int *delaunay, int *render,
		char **outputFile, char **inputFile,
		char **outputSites, char **outputVect){

  // check for an import... if so we just display tin 
  if (argc >= 3 && strcmp(argv[2],"import")==0){
    
    // since we render the outputFile, we will set the outputfile to
    // be the name of the tin we are importing
    *outputFile = argv[1];
    
    if(argc == 4){
      if(strcmp(argv[3],"render")==0)
	*render = 1;
      else
	*render = 0;
    }
  }
  // validate the number of arguments 
  else if (argc < 4){
    printf("usage: r.refine <intput-grid> <output-tin> <error> [memory in MB]" 
	   "[delaunay] [nodata] [render]\n");
    printf("       tin <input-tin> import [render]\n"); 
    exit(1);
  }

  // since we are not importing the TIN lets read in the gird 
  else{

    *inputFile = argv[1];
    *outputFile = argv[2];

    // convert error value from char to float 
    sscanf(argv[3],"%lf",err);
    
    // set memory 
    if(argc >= 5)
      sscanf(argv[4],"%lf",mem);
    else {
      printf("Assuming 250 MB of memory.\n");
    }
    
    // set delaunay 
    if(argc >= 6)
      sscanf(argv[5],"%d",delaunay);
    else {
      printf("Assuming Delaunay is on.\n");
    }
    
    // set nodata handeling 
    if(argc >= 7)
      sscanf(argv[6],"%d",useNoData);
    else {
      printf("Assuming no to handle nodata points\n");
    }

    // decide whether or not to render 
    if(argc == 8){
      if(strcmp(argv[7],"render")==0){
	*render = 1;
      }
      else
	*render = 0;
    }
  }
}

#endif


