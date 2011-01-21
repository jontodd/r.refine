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
 * rtimer.h format ouput for system timer
 *
 * AUTHOR(S): Jonathan Todd - <jonrtodd@gmail.com>
 *            Laura Toma - <ltoma@bowdoin.edu>
 *
 * UPDATED:   jt 2005-08-15
 *
 * COMMENTS: Based on the rtimer by Laura Toma.
 *
 *****************************************************************************/

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "rtimer.h"

char *
rt_sprint_safe(char *buf, Rtimer rt) {
  if(rt_w_useconds(rt) == 0) {
    sprintf(buf, "%4.2f\t%.1f%%\t",
	    0.0, 0.0);
  } else {
    sprintf(buf, "%4.2f\t%.1f%%\t",
	    rt_w_useconds(rt)/1000000,
	    100.0*(rt_u_useconds(rt)+rt_s_useconds(rt)) / rt_w_useconds(rt));
  }
  return buf;
}
