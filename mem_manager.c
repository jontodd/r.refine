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
 * mem_manager.c functions to track memory leaks and overall usage
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: This code is mostly for debug purposes and is not
 * currently being used
 *
 *****************************************************************************/

#include "mem_manager.h"

unsigned int memUsage = 0;

void *myMalloc(size_t size,char *str){
  if(str != NULL)
    printf("myMalloc: %s \n",str);
  memUsage += size;
  return malloc(size);
}

void myFree(void *ptr, char *str){
  if(str != NULL)
    printf("myFree: %s \n",str);
  memUsage -= sizeof(ptr);
  free(ptr);
}
