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

#include <sst_config.h>
#include "mips_4kc.h"

#include "y.tab.h"

using namespace SST;
using namespace SST::MIPS4KCComponent;



#define MAX_EXCPTS 13
#define sig_mesg(a,mesg)						\
    write_output (message_out, "\n%s (signal %d @ %lld) %s", DESC(a), a, reg_word::getNow(), mesg)

/* Exported Variables: */

namespace SST {
namespace MIPS4KCComponent {
    mem_addr breakpoint_reinsert; /* !0 -> reinsert break at this address */


/* Local Functions:< */

/* local variables */

static int spim_related_sig = 0x980897f8;	/* signals caught off of */
						/* unix that coincide with */
						/* these bits are spim */
						/* related */

excpt_desc MIPS4KC::excpt_handler[] =
{
/* name		mapped to signal # ?	frequency */
  {"Int", 	SIGINT,			0},
  {"MOD", 	0,			0},
  {"TLBL", 	0,			0},
  {"TLBS", 	0,			0},
  {"AdEL", 	SIGBUS,			0},
  {"AdES", 	SIGBUS,			0},
  {"IBE", 	SIGBUS,			0},
  {"DBE", 	SIGBUS,			0},
  {"Sys", 	SIGSYS,			0},
  {"Bp",	0,			0},
  {"RI", 	SIGILL,			0},
  {"CpU", 	SIGILL,			0},
  {"OvF",	SIGFPE,			0},
};


signal_desc MIPS4KC::siginfo[] =
{
/* name		status	description */
  {"0     ",	0x7,	"Signal 0",			},
  {"SIGHUP",	0x7,	"Hangup", 			},
  {"SIGINT",	0x6,	"Interrupt", 			},
  {"SIGQUIT",	0x7,	"Quit", 			},
  {"SIGILL",	0x7,	"Illegal instruction",	 	},
  {"SIGTRAP",	0x7,	"Trace/BPT trap", 		},
  {"SIGABRT",	0x7,	"IOT trap", 			},
  {"SIGEMT",	0x7,	"EMT trap",			},
  {"SIGFPE",	0x7,	"Floating point exception",	},
  {"SIGKILL",	0x7,	"Killed", 			},
  {"SIGBUS",	0x7,	"Bus error", 			},
  {"SIGSEGV",	0x7,	"Segmentation fault", 		},
  {"SIGSYS",	0x7,	"Bad system call", 		},
  {"SIGPIPE",	0x7,	"Broken pipe",			},
  {"SIGALRM",	0x7,	"Alarm clock", 			},
  {"SIGTERM",	0x7,	"Terminated", 			},
  {"SIGURG",	0x7,	"Urgent I/O condition",		},
  {"SIGSTOP",	0x7,	"Stopped (signal)", 		},
  {"SIGTSTP",	0x7,	"Stopped", 			},
  {"SIGCONT",	0x7,	"Continued", 			},
  {"SIGCHLD",	0x7,	"Child exited", 		},
  {"SIGTTIN",	0x7,	"Stopped (tty input)", 		},
  {"SIGTTOU",	0x7,	"Stopped (tty output)", 	},
  {"SIGIO",	0x7,	"I/O possible", 		},
  {"SIGXCPU",	0x7,	"CPU time limit exceeded", 	},
  {"SIGXFSZ",	0x7,	"Filesize limit exceeded", 	},
  {"SIGVTALRM",	0x7,	"Virtual timer expired", 	},
  {"SIGPROF",	0x7,	"Profiling timer expired", 	},
  {"SIGWINCH",	0x7,	"Window size changes", 		},
  {"SIGLOST",	0x7,	"Server lost", 			},
  {"SIGUSR1",	0x7,	"User signal 1", 		},
  {"SIGUSR2",	0x7,	"User signal 2", 		},
};

}
}


/* set spim up to catch all signals if in cycle level simulation */

void MIPS4KC::initialize_catch_signals (void)
{
  int x;
  struct sigvec handler;

  handler.sv_mask = 0;
  handler.sv_onstack = 0;

#ifdef mips
  if (cycle_level) handler.sv_handler = intercept_signals;
  else
#endif
    handler.sv_handler = SIG_DFL;
  for (x=0; x<NSIG; x++)
    sigvec (x, &handler, NULL);
}


/* initialize handler information */

void MIPS4KC::initialize_sighandlers (void)
{
  memclr (&proc, sizeof (struct spim_proc));
}


/* initialize exception counts */

void MIPS4KC::initialize_excpt_counts (void)
{
  int x;
  for (x=0; x<MAX_EXCPTS; x++)
    excpt_handler[x].freq = 0;
}


int MIPS4KC::process_excpt (void)
{
  int retval = 0;
  int excpt_code = (Cause.getData() >> 2) & 0xf;

  /* check signal pending status when entering system, process if needed */
  if (issig())
    return (psig());

  EXCPT_COUNT(excpt_code)++;

  switch (excpt_code)
    {

    /* the following exceptions cannot be mapped to signals, spim handles */
    case MOD_EXCPT:
    case TLBS_EXCPT:
      tlb_service(0, 0);
      break;

    case TLBL_EXCPT:
      tlb_service(0, 1);
      break;

    case BKPT_EXCPT: {
        fatal_error("No BRKPT_EXCPT\n");
#if 0
      mem_addr addr = EPC + (((Cause>>31) & 1) ? BYTES_PER_WORD : 0);
      write_output (message_out, "Breakpoint caught at: 0x%08x\n", addr);
      delete_breakpoint (addr);
      breakpoint_reinsert = addr;
      retval = 1;
#endif
    }
      break;

    case SYSCALL_EXCPT: {
        if ( ! ((Cause.getData()>>31) & 1) )
            EPC += BYTES_PER_WORD;
        else {
            instruction *tmp_inst = (instruction *) malloc(sizeof(instruction));
#warning check for bad syscall here
            READ_MEM_INST (tmp_inst, EPC.getData());
            EPC = compute_branch_target (tmp_inst);
        }
      retval = do_syscall();

      if (retval == -1)
	/* system call was exit */
	return (-1);

      else if (retval == 0)
	/* bad system call --> turn into a signal */
	psignal (excpt_handler[SYSCALL_EXCPT].sig);

      retval = !retval;	 	/* retval == 1 -> syscall ok;
				 * retval == 0 -> syscall bad */
    }
      break;



      /* these exceptions can be mapped to signals for passing to the */
      /* simulated program */

    case RI_EXCPT:
    case OVF_EXCPT:
    case INT_EXCPT:
    case ADDRL_EXCPT:
    case ADDRS_EXCPT:
    case IBUS_EXCPT:
    case DBUS_EXCPT:
    case CPU_EXCPT:
      psignal (excpt_handler[excpt_code].sig);
      break;



    default:
      write_output (message_out,
		    "Unknown exception: %x @ %lld\n", (excpt_code),
                    reg_word::getNow());
      return (-1);
    }


  /* check signal pending status before leaving system, process if needed */
  if (issig()) {
    return (psig());
  } else {
#warning check for faulted PC here
      PC = EPC.getData();
      nPC = EPC.getData() + BYTES_PER_WORD;
      return (retval);
  }
}



/* used by process_excpt to calculate startup address when instruction that */
 /* caused exception has been handled (and must therefore not be redone) */
 /* but lies in a delay slot. */

reg_word MIPS4KC::compute_branch_target (instruction *inst)
{
  reg_word tmp_PC;

  /* printf("Computing branch..."); */
  switch (OPCODE (inst)) {
  case Y_BC0F_OP:
  case Y_BC2F_OP:
  case Y_BC3F_OP:
    if (CpCond[OPCODE (inst) - Y_BC0F_OP] == 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BC1F_OP:
    if (FpCond == 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else tmp_PC = nPC + BYTES_PER_WORD;
    break;


  case Y_BC0T_OP:
  case Y_BC2T_OP:
  case Y_BC3T_OP:
    if (CpCond[OPCODE (inst) - Y_BC0T_OP] != 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else
      tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BC1T_OP:
    if (FpCond != 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else
      tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BEQ_OP:
    if (read_R_reg(RS (inst)) == read_R_reg(RT (inst)))
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else
      tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BGEZ_OP:
    if (SIGN_BIT (read_R_reg(RS (inst))) == 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BGEZAL_OP:
    if (SIGN_BIT (read_R_reg(RS (inst))) == 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BGTZ_OP:
    if (read_R_reg(RS (inst)) != 0 && SIGN_BIT (read_R_reg(RS (inst))) == 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BLEZ_OP:
    if (read_R_reg(RS (inst)) == 0 || SIGN_BIT (read_R_reg(RS (inst))) != 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BLTZ_OP:
    if (SIGN_BIT (read_R_reg(RS (inst))) != 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BLTZAL_OP:
    if (SIGN_BIT (read_R_reg(RS (inst))) != 0)
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_BNE_OP:
    if (read_R_reg(RS (inst)) != read_R_reg(RT (inst)))
      tmp_PC = nPC + (SIGN_EX (IOFFSET (inst)) << 2);
    else tmp_PC = nPC + BYTES_PER_WORD;
    break;

  case Y_J_OP:
    tmp_PC = ((PC & 0xf0000000) | (TARGET (inst) & 0x03ffffff) << 2);
    break;

  case Y_JAL_OP:
    tmp_PC = ((PC & 0xf0000000) | ((TARGET (inst) & 0x03ffffff) << 2));
    break;

  case Y_JALR_OP:
  case Y_JR_OP:
    tmp_PC = read_R_reg(RS (inst));
    break;

  default:
    printf("Error in compute-branch-target\n");
  }

  /* printf("%x\n", tmp_PC); */
  return(tmp_PC);
}




/* spim's signal handler, catch signals off of unix */

void MIPS4KC::intercept_signals (int sig, int code, struct sigcontext *scp)
{
  if (!cycle_running) {
    if (spim_related_sig & (1 << sig))
      sig_mesg (sig, "occurred.  Possible internal error?\n");
    else if (sig == SIGINT)
      write_output (message_out, "Quit\n");
    else
      sig_mesg (sig, "occurred. ?\n");
    cycle_steps = 0;
    /*longjmp (spim_top_level_env, 1);*/
  }
  else if (spim_related_sig & (1 << sig))
    cycle_steps = 0;
  else psignal (sig);
}




/* post a signal for simulated program if it is not being ignored */

void MIPS4KC::psignal (int sig)
{
  if (! PASS(sig)) {
    sig_mesg (sig, "occurred but not passed.\n");
    cycle_steps = 0;
  }
  else proc.p_sig |= (1 << sig);
}



/* check signals pending, return (1) if signal must be processed */

int MIPS4KC::issig (void)
{
  int x;

  for (x=0; x<NSIG; x++)
    if (proc.p_sig & (1 << x)) break;

  if (x >= NSIG)
    return (0);

  if ((1 << x) & proc.p_sigmask) {
    if (PRINT (x))
      sig_mesg (x, "passed to program.  Masked.\n");
    return (0);
  }
  proc.p_cursig = x;
  return (1);
}



/* handle signal by performing default action or by preparing for simulated
 * program's specified handler */

int MIPS4KC::psig (void)
{
  /* remove signal from pending list */
  proc.p_sig &= (~ (1 << proc.p_cursig));

  if (PRINT(proc.p_cursig))
    sig_mesg (proc.p_cursig, "passed to program.\n");

  if (! (CATCH & (1 << proc.p_cursig))) {
    /* if program hasn't specified handler; try to emulate SIG_DFL */
    PC = EPC;
    nPC = EPC + BYTES_PER_WORD;
    switch (proc.p_cursig) {

    case SIGCONT:
    case SIGIO:
    case SIGSTOP:
    case SIGTSTP:
    case SIGTTIN:
    case SIGTTOU:
    case SIGWINCH:
      write_output (message_out, "Program stopped.\n");
      return (1);

    default:
        {
            char str[100];
            sprintf(str, "Program terminated @ %llu\n", reg_word::getNow());
            write_output (message_out, str);
        }
      return (-1);
    }
  }

  sendsig ();
  return (STOP (proc.p_cursig));
}



/* write status of machine to stack, fill register with appropriate
 * arguments for signal trampoline code, and jump to that code */

void MIPS4KC::sendsig (void)
{
#if 0
  int i;
  struct sigcontext *sc;

  R[29] -= sizeof(struct sigcontext) + 4;
  sc = (struct sigcontext *) MEM_ADDRESS(R[29]);
  /**/sc->sc_onstack = 0;
  sc->sc_mask = proc.p_sigmask;
  sc->sc_pc = EPC;
  for(i=0; i < 32; ++i)
    sc->sc_regs[i] = R[i];
  sc->sc_mdlo = LO;
  sc->sc_mdhi = HI;
  /**/sc->sc_ownedfp = 0;
  for(i=0; i < 32; ++i)
    sc->sc_fpregs[i] = FPR[i];
  /**/sc->sc_fpc_csr = 0;
  /**/sc->sc_fpc_eir = 0;
  sc->sc_cause = Cause;
  sc->sc_badvaddr = BadVAddr;
  /**/sc->sc_badpaddr = 0;

  /* set up new mask for duration of signal handling */
  MASK = (1 << proc.p_cursig) | (HANDLE_MASK(proc.p_cursig));

  R[REG_A0] = proc.p_cursig;
  R[REG_A1] = (Cause >> 2) & 0xf;
  R[REG_A2] = R[29];
  R[REG_A3] = (int) HANDLE(R[REG_A0]);

  PC = TRAMP(proc.p_cursig);
  nPC = PC + BYTES_PER_WORD;
#endif
}



/* return state of machine to the way it was before signal handling */

void MIPS4KC::dosigreturn (mem_addr sigptr)
{
#if 0
  int i;
  struct sigcontext *sc;

  sc = (struct sigcontext *) sigptr;
  proc.p_sigmask = sc->sc_mask;
  EPC = sc->sc_pc;
  for (i=0; i < 32; ++i)
    R[i] = sc->sc_regs[i];
  LO = sc->sc_mdlo;
  HI = sc->sc_mdhi;
  for(i=0; i < 32; ++i) /* FPU registers */
    FPR[i] = sc->sc_fpregs[i];
  Cause = sc->sc_cause;
  BadVAddr = sc->sc_badvaddr;
  R[29] += sizeof(struct sigcontext) + 4;
#endif
}



void MIPS4KC::print_except_stats (void)
{
  int x;

  printf ("Exception occurrences...\n\n");
  printf ("Name\t\tFrequency\n");
  for (x=0; x<MAX_EXCPTS; x++)
    printf ("%s\t\t%d\n", EXCPT_STR(x), EXCPT_COUNT(x));
}



void MIPS4KC::print_signal_status (int sig)
{
  int x;

  printf ("Signal status...\n\n");
  printf ("Signal\t\tStop\tPrint\tPass\tDescription\n");
  if (sig == ALL_SIGNALS)
    for (x=0; x<NSIG; x++)
      printf ("%s (%d)\t%1d\t%1d\t%1d\t%s\n", siginfo[x].signame, x,
	      STOP(x), PRINT(x), PASS(x), DESC(x));
  else
    printf ("%s (%d)\t%1d\t%1d\t%1d\t%s\n", siginfo[sig].signame, sig,
	      STOP(sig), PRINT(sig), PASS(sig), DESC(sig));
}




