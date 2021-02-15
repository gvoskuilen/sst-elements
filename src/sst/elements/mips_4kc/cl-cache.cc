/* SPIM S20 MIPS Cycle Level simulator.
   Definitions for the SPIM S20 Cycle Level Simulator (SPIM-CL).
   Copyright (C) 1991-1992 by Anne Rogers (amr@cs.princeton.edu) and
   Scott Rosenberg (scottr@cs.princeton.edu)
   ALL RIGHTS RESERVED.

   SPIM-CL is distributed under the following conditions:

     You may make copies of SPIM-CL for your own use and modify those copies.

     All copies of SPIM-CL must retain our names and copyright notice.

     You may not sell SPIM-CL or distributed SPIM-CL in conjunction with a
     commerical product or service without the expressed written consent of
     Anne Rogers.

   THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.
*/

/* Simple write through cache -- stall on all misses                 */
/* Split transaction bus simulator                                   */

#include <sst_config.h>
#include "mips_4kc.h"

using namespace SST;
using namespace SST::MIPS4KCComponent;


#define TRUE 1
#define FALSE 0

unsigned int MIPS4KC::bus_service ()
{
    return 0;
}

int MIPS4KC::cache_service (mem_addr addr, int type, 
                            unsigned int *req_num)
{
    return CACHE_HIT;

  switch (type) {

  case (STORE):
    break;

  case (LOAD):
    break;

  case (ILOAD):
    break;
  }

  //AFR
  return CACHE_MISS;
}


