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

#ifndef _CL_EXCEPT_H
#define _CL_EXCEPT_H

//#include <syscall.h>
#include <signal.h>

/* gdb-style for tracking each signal */
typedef struct signal_desc
{
  const char *signame;
  short stats;		/* bit 0 --> pass signal to simulated program
			 * bit 1 --> print if signal is seen
			 * bit 2 --> stop if signal is seen */
  const char *desc;		/* short description of signal */
} signal_desc;

#define DESC(sig)	(siginfo[sig].desc)
#define STOP(sig)	(siginfo[sig].stats & 0x4 ? 1 : 0)
#define PRINT(sig)	(siginfo[sig].stats & 0x2 ? 1 : 0)
#define PASS(sig)	(siginfo[sig].stats & 0x1 ? 1 : 0)
#define SET_STOP(sig,flag)						\
  siginfo[sig].stats = (flag ? siginfo[sig].stats | 0x4 :		\
			   siginfo[sig].stats & ~0x4)
#define SET_PRINT(sig,flag)						\
    siginfo[sig].stats = (flag ? siginfo[sig].stats | 0x2 :	\
			     siginfo[sig].stats & ~0x2)
#define SET_PASS(sig,flag)						\
    siginfo[sig].stats = (flag ? siginfo[sig].stats | 0x1 :	\
			     siginfo[sig].stats & ~0x1)


/* implemented as discussed in 4.3BSD UNIX Operating System, Leffler, */
/* McKusick, Karels, Quarterman, Addison-Wesley, New York, 1989. */
typedef struct spim_proc
{
  int p_sig;			/* list of pending signals for current proc */
  int p_sigmask;		/* which signals to mask */
  int p_sigcatch;		/* which signals to catch */
  int p_cursig;			/* signal currently being processed */
  struct sigvec sv[NSIG];	/* handler information for each signal */
  mem_addr tramp_addr[NSIG];	/* trampoline addr for each handler */
} spim_proc;

#define HANDLE(sig)		(proc.sv[sig].sv_handler)
#define HANDLE_MASK(sig)	(proc.sv[sig].sv_mask)
#define TRAMP(sig)		(proc.tramp_addr[sig])
#define MASK			proc.p_sigmask
#define CATCH			proc.p_sigcatch



typedef struct excpt_desc
{
  string excptname;
  int sig;		/* if mappable to signal, what is signal ? */
  int freq;		/* how many times has it occurred */
    excpt_desc(const char*e, int s, int f) : excptname(e), sig(s), freq(f) {
        //printf("init %s %s %p\n", e, excptname.c_str(), this);
    }
} excpt_desc;


#define EXCPT_STR(x)	(excpt_handler[x].excptname.c_str())
#define EXCPT_COUNT(x)	(excpt_handler[x].freq)




#define ALL_SIGNALS 100001		/* when passed as first parameter */
					/* to print_signal_status, print */
					/* all signals' information. */

#endif // _CL_EXCEPT_H
