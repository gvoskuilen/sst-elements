/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
   Copyright (C) 1990-1994 by James Larus (larus@cs.wisc.edu).
   ALL RIGHTS RESERVED.
   Improved by Emin Gun Sirer.

   SPIM is distributed under the following conditions:

     You may make copies of SPIM for your own use and modify those copies.

     All copies of SPIM must retain my name and copyright notice.

     You may not sell SPIM or distributed SPIM in conjunction with a
     commerical product or service without the expressed written consent of
     James Larus.

   THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE. */


/* $Header: /var/home/larus/Software/larus/SPIM/RCS/mips-syscall.c,v 1.27 1994/08/12 16:20:08 larus Exp $ */

#include <sst_config.h>
#include "mips_4kc.h"

using namespace SST;
using namespace SST::MIPS4KCComponent;

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

/*#include "spim.h"
#include "inst.h"
#include "mem.h"
#include "reg.h"
#include "read-aout.h"
#include "sym-tbl.h"
#include "spim-syscall.h"
#include "mips-syscall.h"*/

#include "cl-except.h"




/* Local functions: */


#ifndef mips
#ifndef OPEN_MAX
#define OPEN_MAX 20
#endif
#endif

#ifndef NSIG
#define NSIG 128
#endif

/* Local variables: */




#define REG_ERR 7



/* Table describing arguments to syscalls. */

typedef struct
{
  int syscall_num;
  int syscall_type;
  int arg0;
  int arg1;
  int arg2;
  int arg3;
  int arg4;
  char *syscall_name;
} syscall_desc;

enum {BAD_SYSCALL, UNIX_SYSCALL, SPC_SYSCALL};

enum {NO_ARG, INT_ARG, ADDR_ARG, STR_ARG, FD_ARG};  /* Type of argument */

static syscall_desc syscall_table[] =
{
    0
};


#define MAX_SYSCALL	(sizeof(syscall_table)/ sizeof(syscall_table[0]))


static int syscall_usage[MAX_SYSCALL]; /* Track system calls */


#define SYSCALL_COUNT(SYSCALL)						\
  if (SYSCALL < MAX_SYSCALL && SYSCALL >= 0) syscall_usage[SYSCALL]++;



/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */

int MIPS4KC::do_syscall (void)
{
#warning check for wrong syscall
  SYSCALL_COUNT(R[REG_V0].getData());
  if (1)   {
      /* Syscalls for the source-language version of SPIM.  These are
	 easier to use than the real syscall and are portable to non-MIPS
	 (non-Unix?) operating systems. */
        
      switch (R[REG_V0].getData())
	{
	case PRINT_INT_SYSCALL:
          write_output (console_out, "PRINT_INT_SYSCALL(p%d): %d %u %x\n", 
                        proc_num, 
                        R[REG_A0].getData(), R[REG_A0].getData(), R[REG_A0].getData());
	  break;

	case PRINT_FLOAT_SYSCALL:
	  {
	    float val = FPR_S (REG_FA0);

	    write_output (console_out, "%f", val);
	    break;
	  }

	case PRINT_DOUBLE_SYSCALL:
	  write_output (console_out, "%f", FPR[REG_FA0/2]);
	  break;

	case PRINT_STRING_SYSCALL:
            //write_output (console_out, "%s", MEM_ADDRESS_PTR(R[REG_A0]));
	  break;

	case READ_INT_SYSCALL:
	  {
	    static char str [256];
            fatal_error("READ INT not impl\n");
	    //read_input (str, 256);
	    R[REG_RES] = atol (str);
	    break;
	  }

	case READ_FLOAT_SYSCALL:
	  {
	    static char str [256];
            fatal_error("read float not impl\n");
	    //read_input (str, 256);
	    FGR [REG_FRES] = (float) atof (str);
	    break;
	  }

	case READ_DOUBLE_SYSCALL:
	  {
	    static char str [256];
            fatal_error("read doulbe not impl\n");
	    //read_input (str, 256);
	    FPR [REG_FRES] = atof (str);
	    break;
	  }

	case READ_STRING_SYSCALL:
	  {
            fatal_error("read string not impl\n");
              //read_input ( (char *) MEM_ADDRESS (R[REG_A0]), R[REG_A1]);
	    break;
	  }

	case SBRK_SYSCALL:
	  {
	    mem_addr x = data_top;
	    expand_data (R[REG_A0].getData());
	    R[REG_RES] = x;
	    break;
	  }

	case EXIT_SYSCALL:
	  if (cycle_level) return (-1);
	  else
	    return (0);

        case GET_PROC_NUM_SYSCALL:
            R[REG_RES] = proc_num;
            break;

	default:
	  if (cycle_level) 
	    return (0);
	  run_error ("Unknown system call: %d\n", R[REG_V0].getData());
	  break;
	}
    }
  else
#ifdef mips
    {
#ifdef CL_SPIM
      if (!cycle_level)
#endif
	if (!fds_initialized)
	  {
	    initialize_prog_fds ();
	    fds_initialized = 1;
	  }

      /* Use actual MIPS system calls. First translate arguments from
	 simulated memory to actual memory and correct file descriptors. */
      if (R[REG_V0] < 0 || R[REG_V0] > MAX_SYSCALL)
	{
#ifdef CL_SPIM
	  if (cycle_level)
	    return(0);
#endif
	  run_error ("Illegal system call: %d\n", R[REG_V0]);
	}

      switch (syscall_table[R[REG_V0]].syscall_type)
	{
	case BAD_SYSCALL:
#ifdef CL_SPIM
	  if (cycle_level)
	    return(0);
#endif
	  run_error ("Unknown system call: %d\n", R[REG_V0]);
	  break;

	case UNIX_SYSCALL:
	  unixsyscall ();
	  break;

	case SPC_SYSCALL:
	  /* These syscalls need to be simulated specially: */
	  switch (R[REG_V0])
	    {
	    case SYS_syscall:
	      R[REG_V0] = R[REG_A0];
	      R[REG_A0] = R[REG_A1];
	      R[REG_A1] = R[REG_A2];
	      R[REG_A2] = R[REG_A3];
	      READ_MEM_WORD (R[REG_A3], R[REG_SP] + 16);
#ifdef CL_SPIM
	      if (cycle_level)
		return (do_syscall());
#endif
	      do_syscall ();
	      break;

	    case SYS_sysmips:
	      {
		/* The table smipst maps from the sysmips arguments to syscall
		   numbers */
		int callno;
		static int smipst[] = {7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
					 7, 7, SYS_getrusage, SYS_wait3,
					 SYS_cacheflush, SYS_cachectl};
		callno= R[REG_A0];
		callno= ( (callno >0x100) ? smipst[callno - 0x100 + 10]
			 : smipst[callno]);
		R[REG_V0] = callno;
		R[REG_A0] = R[REG_A1];
		R[REG_A1] = R[REG_A2];
		R[REG_A2] = R[REG_A3];
		READ_MEM_WORD (R[REG_A3], R[REG_SP] + 16);
#ifdef CL_SPIM
		if (cycle_level)
		  return (do_syscall());
#endif	
		do_syscall ();
		break;
	      }

	    case SYS_exit:
	      {
#ifdef CL_SPIM
		if (cycle_level)
		  {
		    write_output (message_out,
				  "\nProgram exited with value (%d).\n",
				  R[REG_A0]);
		    return (-1);
		  }
#endif
		kill_prog_fds ();

		return (0);
	      }

	    case SYS_close:
	      if (unixsyscall () >= 0)
		prog_fds[R[REG_A0]] = -1; /* Mark file descriptor closed */
	      break;

	    case SYS_open:
	    case SYS_creat:
	    case SYS_dup:
	      {
		int ret = unixsyscall ();

		if (ret >= 0)
		  prog_fds[ret] = ret; /* Update fd translation table */

		break;
	      }

	    case SYS_pipe:
	      {
		/* This isn't too useful unless we implement fork () or other
		   fd passing mechanisms */
		int fd1, fd2;

		if (unixsyscall () >= 0)
		  {
		    READ_MEM_WORD (fd1, MEM_ADDRESS (R[REG_A0]));
		    READ_MEM_WORD (fd2, MEM_ADDRESS (R[REG_A0]));
		    prog_fds[fd1] = fd1;
		    prog_fds[fd2] = fd2;
		  }
		break;
	      }

	    case SYS_select:
	      {
		int fd;
		/*
		 * We have to use this kludge to circumvent typechecking
		 * because the memory read macros take the lefthand side
		 * as an argument instead of simply returnign the value
		 * at the address
		 */
		long int kludge;
		fd_set a, b, c;
		fd_set *readfds = &a, *writefds = &b, *exceptfds = &c;
		struct timeval *timeout;

		FD_ZERO (readfds);
		FD_ZERO (writefds);
		FD_ZERO (exceptfds);
		READ_MEM_WORD (kludge, R[REG_SP] + 16);
		if (kludge == NULL)
		  timeout = NULL;
		else
		  timeout = (struct timeval *) MEM_ADDRESS (kludge);

		if (R[REG_A1] == NULL)
		  readfds = NULL;
		else
		  for (fd = 0; fd < R[REG_A0]; fd++)
		    if (FD_ISSET (fd, (fd_set *) MEM_ADDRESS (R[REG_A1])))
		      FD_SET (prog_fds[fd], readfds);

		if (R[REG_A2] == NULL)
		  writefds = NULL;
		else
		  for (fd = 0; fd < R[REG_A0]; fd++)
		    if (FD_ISSET (fd, (fd_set *) MEM_ADDRESS (R[REG_A2])))
		      FD_SET (prog_fds[fd], writefds);

		if (R[REG_A3] == NULL)
		  exceptfds = NULL;
		else
		  for (fd = 0; fd < R[REG_A0]; fd++)
		    if (FD_ISSET (fd, (fd_set *) MEM_ADDRESS (R[REG_A3])))
		      FD_SET (prog_fds[fd], exceptfds);

		R[REG_RES] = select (R[REG_A0], readfds, writefds, exceptfds,
				     timeout);
		if (readfds == NULL)
		  R[REG_A1] = NULL;
		else
		  for (fd = 0; fd < R[REG_A0]; fd++)
		    if (FD_ISSET (fd, readfds))
		      FD_SET (reverse_fds (fd),
			      (fd_set *) MEM_ADDRESS (R[REG_A1]));

		if (writefds == NULL)
		  R[REG_A2] = NULL;
		else
		  for (fd = 0; fd < R[REG_A0]; fd++)
		    if (FD_ISSET (fd, writefds))
		      FD_SET (reverse_fds (fd),
			      (fd_set *) MEM_ADDRESS (R[REG_A2]));

		if (exceptfds == NULL)
		  R[REG_A3] = NULL;
		else
		  for (fd = 0; fd < R[REG_A0]; fd++)
		    if (FD_ISSET (fd, exceptfds))
		      FD_SET (reverse_fds (fd),
			      (fd_set *) MEM_ADDRESS (R[REG_A3]));

		if (R[REG_RES] < 0)
		  {
		    R[REG_ERR] = -1;
		    R[REG_RES] = errno;
		    return (-1);
		  }
		else
		  {
		    R[REG_ERR] = 0;
		    return (R[REG_RES]);
		  }
	      }

	    case SYS_sbrk:
	      {
		expand_data (R[REG_A0]);
		R[REG_RES] = program_break;
		program_break += R[REG_A0];
		R[REG_ERR] = 0;
		break;
	      }

	    case SYS_brk:
	      /* Round up to 4096 byte (page) boundary */
	      if ( ( (int) R[REG_A0] - (int) data_top) > 0)
		expand_data (ROUND (R[REG_A0], 4096)- (int)data_top);
	      R[REG_RES] = program_break;
	      program_break = ROUND (R[REG_A0], 4096);
	      R[REG_ERR] = 0;
	      break;

	    case SYS_sigvec:
	      {
		int x;

#ifdef CL_SPIM
		if (cycle_level) {
		  if (R[REG_A2] != 0) {
		    /* copy old sigvec data if structure is provided */
		    mem_addr tmp = MEM_ADDRESS(R[REG_A2]);
		    memcpy (&HANDLE(R[REG_A0]), tmp, sizeof(struct sigvec));
		  }
		  if (R[REG_A1] != 0) {
		    /* grab new sighandle information */
		    mem_addr tmp = MEM_ADDRESS(R[REG_A1]);
		    memcpy (&HANDLE(R[REG_A0]), tmp, sizeof(struct sigvec));
		    CATCH |= (1 << R[REG_A0]);
		  }
		  TRAMP(R[REG_A0]) = R[REG_A3];
		  R[REG_ERR] = 0;
		  break;
		}
		else
#endif
		  {
		    if (R[REG_A2] != 0)
		      * (struct sigvec *) MEM_ADDRESS (R[REG_A2]) = sighandler[R[REG_A0]];
		    READ_MEM_WORD (x, R[REG_A1]);
		    sighandler[R[REG_A0]].sv_handler = (void (*) ()) x;
		    READ_MEM_WORD (x,R[REG_A1] + sizeof (int *));
		    sighandler[R[REG_A0]].sv_mask = x;
		    READ_MEM_WORD (x,R[REG_A1] + sizeof (int *)
				   + sizeof (sigset_t));
		    sighandler[R[REG_A0]].sv_flags = x;
		    exception_address[R[REG_A0]] = R[REG_A3];
		    R[REG_ERR] = 0;
		    break;
		  }
	      }

	    case SYS_sigreturn:
#ifdef CL_SPIM
	      if (cycle_level)
		dosigreturn (MEM_ADDRESS (R[REG_A0]));
	      else
#endif
		do_sigreturn (MEM_ADDRESS (R[REG_A0]));
	      R[REG_ERR] = 0;
	      break;

	    case SYS_sigsetmask:
#ifdef CL_SPIM
	      if (cycle_level) {
		R[REG_RES] = MASK;
		MASK = R[REG_A0];
	      }
	      else
#endif
		{
		  R[REG_RES] = prog_sigmask;
		  prog_sigmask = R[REG_A0];
		}
	      R[REG_ERR] = 0;
	      break;

	    case SYS_sigblock:
#ifdef CL_SPIM
	      if (cycle_level) {
		R[REG_RES] = MASK;
		MASK |= R[REG_A0];
	      }
	      else
#endif
		{
		  R[REG_RES] = prog_sigmask;
		  prog_sigmask |= R[REG_A0];
		}
	      R[REG_ERR] = 0;
	      break;

	    case SYS_cacheflush:
#if 0
	      R[REG_RES] = cache_flush ((void*)MEM_ADDRESS (R[REG_A0]),
					R[REG_A1],
					R[REG_A2]);
#endif
	      R[REG_ERR] = 0;
	      break;

	    case SYS_cachectl:
#if 0
	      R[REG_RES] = cache_ctl ((void*)MEM_ADDRESS (R[REG_A0]),
				      R[REG_A1],R
				      [REG_A2]);
#endif
	      R[REG_ERR] = 0;
	      break;

	    default:
#ifdef CL_SPIM
	      if (cycle_level)
		return(0);
#endif
	      run_error ("Unknown special system call: %d\n",R[REG_V0]);
	      break;
	    }
	  break;

	default:
#ifdef CL_SPIM
	  if (cycle_level)
	    return(0);
#endif
	  run_error ("Unknown type for syscall: %d\n", R[REG_V0]);
	  break;
	}
    }
#else
  run_error ("Can't use MIPS syscall on non-MIPS system\n");
#endif

  return (1);
}



/* Execute a Unix system call.  Returns negative on error. */

int MIPS4KC::unixsyscall (void)
{
  fatal_error("syscall not implemented\n");
  return -1;
#if 0
  int *arg0, *arg1, *arg2, *arg3;

  arg0 = SYSCALL_ARG (REG_V0,arg0, REG_A0);
  arg1 = SYSCALL_ARG (REG_V0,arg1, REG_A1);
  arg2 = SYSCALL_ARG (REG_V0,arg2, REG_A2);
  arg3 = SYSCALL_ARG (REG_V0,arg3, REG_A3);
  //R[REG_RES] = syscall (R[REG_V0], arg0, arg1, arg2, arg3);

  /* See if an error has occurred during the system call. If so, the
     libc wrapper must be notifified by setting register 7 to be less than
     zero and the return value should be errno. If not, register 7 should
     be zero. r7 acts like the carry flag in the old days.  */

  if (R[REG_RES] < 0)
    {
      R[REG_ERR] = -1;
      R[REG_RES] = errno;
      return (-1);
    }
  else
    {
      R[REG_ERR] = 0;
      return (R[REG_RES]);
    }
#endif
}


int MIPS4KC::reverse_fds (int fd)
{
  int i;

  for (i = 0; i < OPEN_MAX; i++)
    if (prog_fds[i] == fd)
      return (i);

  run_error ("Couldn't reverse translate fds\n");
  return (-1);
}


void MIPS4KC::print_syscall_usage (void)
{
  int x;

  printf ("System call counts...\n\n");
  printf ("Call#\t\tFrequency\n");
  for (x = 0; x < MAX_SYSCALL; x ++)
    if (syscall_usage[x] > 0)
      printf("%d(%s)\t\t%d\n",
	     x, syscall_table[x].syscall_name, syscall_usage[x]);
  printf ("\n");
}


void  MIPS4KC::initialize_prog_fds (void)
{
  int x;

  for (x = 0; x < OPEN_MAX; prog_fds[x++] = -1);
  if (((prog_fds[0] = dup(0)) < 0) || 
      ((prog_fds[1] = dup(1)) < 0) ||
      ((prog_fds[2] = dup(2)) < 0))
    error("init_prog_fds");
}


/* clear out programs file descriptors, close necessary files */

void MIPS4KC::kill_prog_fds (void)
{
  int x;

  for (x = 0; x < OPEN_MAX; x++)
    if (prog_fds[x] != -1) close(prog_fds[x]);
}



void MIPS4KC::handle_exception (void)
{
#warning check for faulted exception here
  if (!quiet && ((Cause >> 2) & 0xf) != INT_EXCPT)
      error ("Exception occurred at PC=0x%08x\n", EPC.getData());

  exception_occurred = 0;
  PC = EXCEPTION_ADDR;

  switch (((Cause >> 2) & 0xf).getData())
    {
    case INT_EXCPT:
	R[REG_A0] = SIGINT;
      break;

    case ADDRL_EXCPT:
	R[REG_A0] = SIGSEGV;
      if (!quiet)
          error ("  Unaligned address in inst/data fetch: 0x%08x\n",BadVAddr.getData());
      break;

    case ADDRS_EXCPT:
	R[REG_A0] = SIGSEGV;
      if (!quiet)
          error ("  Unaligned address in store: 0x%08x\n", BadVAddr.getData());
      break;

    case IBUS_EXCPT:
	R[REG_A0] = SIGBUS;
      if (!quiet)
          error ("  Bad address in text read: 0x%08x\n", BadVAddr.getData());
      break;

    case DBUS_EXCPT:
	R[REG_A0] = SIGBUS;
      if (!quiet)
          error ("  Bad address in data/stack read: 0x%08x\n", BadVAddr.getData());
      break;

    case BKPT_EXCPT:
      exception_occurred = 0;
      return;

    case SYSCALL_EXCPT:
      if (!quiet)
	error ("  Error in syscall\n");
      break;

    case RI_EXCPT:
      if (!quiet)
	error ("  Reserved instruction execution\n");
      break;

    case OVF_EXCPT:
	R[REG_A0] = SIGFPE;
      if (!quiet)
	error ("  Arithmetic overflow\n");
      break;

    default:
      if (!quiet)
          error ("Unknown exception: %d\n", Cause.getData() >> 2);
      break;
    }

#ifdef mips
      if ((prog_sigmask & (1 << R[REG_A0])) == 1)
	return;

      if((int) sighandler[R[REG_A0]].sv_handler == 0)
	run_error ("Exception occurred at PC=0x%08x\nNo handler for it.\n",
		   EPC);

      setup_signal_stack();
      R[REG_A1] = 48;

      R[REG_A2] = R[29];
      R[REG_A3] = (int) sighandler[R[REG_A0]].sv_handler;
      if ((PC = exception_address[R[REG_A0]]) == 0)
	PC = (int) find_symbol_address ("sigvec") + 44;
#endif
}


#ifdef __STDC__
static void
setup_signal_stack (void)
#else
static void
setup_signal_stack ()
#endif
{
#ifdef mips
  int i;
  struct sigcontext *sc;

  R[29] -= sizeof(struct sigcontext) + 4;
  sc = (struct sigcontext *) MEM_ADDRESS (R[29]);
  sc->sc_onstack = 0		/**/;
  sc->sc_mask = prog_sigmask;
  sc->sc_pc = EPC;
  for(i=0; i < 32; ++i)		/* general purpose registers */
    sc->sc_regs[i] = R[i];
  sc->sc_mdlo = LO;		/* mul/div low */
  sc->sc_mdhi = HI;
  sc->sc_ownedfp = 0;		/* fp has been used */
  for(i=0; i < 32; ++i)		/* FPU registers */
    sc->sc_fpregs[i] = FPR[i];
  sc->sc_fpc_csr = 0;		/* floating point control and status reg */
  sc->sc_fpc_eir = 0;
  sc->sc_cause = Cause;		/* cp0 cause register */
  sc->sc_badvaddr = BadVAddr;	/* cp0 bad virtual address */
  sc->sc_badpaddr = 0;		/* cpu bd bad physical address */
#endif
}


#ifdef __STDC__
static void
do_sigreturn (mem_addr sigptr)
#else
static void
do_sigreturn (sigptr)
  mem_addr sigptr;
#endif
{
#ifdef mips
  int i;
  struct sigcontext *sc;

  sc = (struct sigcontext *) sigptr;
  prog_sigmask = sc->sc_mask;
  PC = sc->sc_pc - BYTES_PER_WORD;
  for(i=0; i < 32; ++i)
    R[i] = sc->sc_regs[i];
  LO = sc->sc_mdlo;
  HI = sc->sc_mdhi;
  for(i=0; i < 32; ++i)		/* FPU registers */
    FPR[i] = sc->sc_fpregs[i];
  Cause = sc->sc_cause;
  BadVAddr = sc->sc_badvaddr;
  R[29] += sizeof(struct sigcontext) + 4;
#endif
}
