/* SPIM S20 MIPS simulator.
   Misc. routines for SPIM.
   Copyright (C) 1990-1994 by James Larus (larus@cs.wisc.edu).
   ALL RIGHTS RESERVED.

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


/* $Header: /var/home/larus/Software/larus/SPIM/RCS/spim-utils.c,v 3.41 1994/08/11 21:51:02 larus Exp $
*/

#include <sst_config.h>
#include "mips_4kc.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/*#include "spim.h"
#include "spim-utils.h"
#include "inst.h"*/
//#include "mem.h"
//#include "reg.h"
//#include "scanner.h"
//#include "parser.h"
//#include "y.tab.h"
//#include "run.h"

using namespace SST;
using namespace SST::MIPS4KCComponent;


/* Initialize or reinitialize the state of the machine. */

void MIPS4KC::initialize_world (int load_trap_handler)
{
  /* Allocate the floating point registers */
  if (FGR == NULL)
    FPR = (double *) xmalloc (16 * sizeof (double));
  /* Allocate the memory */
  make_memory (initial_text_size,
	       initial_data_size, initial_data_limit,
	       initial_stack_size, initial_stack_limit,
	       initial_k_text_size,
	       initial_k_data_size, initial_k_data_limit);
  initialize_registers ();
  initialize_sighandlers();

  /* cycle level stuff */
  //mem_system = mem_sys_init ();
  cycle_running = 0;

}


void MIPS4KC::write_startup_message (void)
{
  write_output (message_out, "CL-SPIM %s\n", "SST");
  write_output (message_out,
		"Copyright 1990-1994 by James R. Larus (larus@cs.wisc.edu).\n");
  write_output (message_out,
		"Copyright (C) 1991-1994 by Anne Rogers (amr@cs.princeton.edu)\n");
  write_output (message_out,
		"and Scott Rosenberg (scottr@cs.princeton.edu).\n");
  write_output (message_out, "All Rights Reserved.\n");
  write_output (message_out, "See the file README a full copyright notice.\n");
}



void MIPS4KC::initialize_registers (void)
{
  memclr (FPR, 16 * sizeof (double));
  FGR = (float *) FPR;
  FWR = (int *) FPR;
  //memclr (R, 32 * sizeof (reg_word));
  R[29] = STACK_TOP - BYTES_PER_WORD - 4096; /* Initialize $sp */
  PC = 0;
  Cause = 0;
  EPC = 0;
  Status_Reg = 0;
  BadVAddr = 0;
  Context = 0;
  PRId = 0;

  PC = 0;
  Status_Reg = (0x3 << 28) | (0x3);
}


#if 0
mem_addr MIPS4KC::starting_address (void)
{
  if (PC == 0)
    {
      if (program_starting_address)
	return (program_starting_address);
      else
	return (program_starting_address
		= find_symbol_address (DEFAULT_RUN_LOCATION));
    }
  else
    return (PC);
}
#endif


/* Initialize the SPIM stack with ARGC, ARGV, and ENVP data. */

void MIPS4KC::initialize_run_stack (int argc, char **argv)
{
  char **p;
  extern char **environ;
  int i, j = 0, env_j;
  mem_addr addrs[10000];

  R[REG_A2] = R[29];

#if 0
  /* Put strings on stack: */
  for (p = environ; *p != '\0'; p++)
    addrs[j++] = copy_str_to_stack (*p);
#endif
  write_output (message_out, "NOT COPYING ENVIORN TO STACK\n");

  R[REG_A1] = R[29];
  env_j = j;
  for (i = 0; i < argc; i++)
    addrs[j++] = copy_str_to_stack (argv[i]);

  R[29] = (R[29] - 7) & ~7;	/* Double-word align */
  /* Build vectors on stack: */
  for (i = env_j - 1; i >= 0; i--)
    copy_int_to_stack (addrs[i]);
  for (i = j - 1; i >= env_j; i--)
    copy_int_to_stack (addrs[i]);

  R[REG_A0] = argc;
  R[29] = R[29] & ~7;		/* Round down to nearest double-word */
  R[29] = copy_int_to_stack (argc); /* Leave pointing to argc */
  if (outputLevel > 0) {
      printf("stack starting at %x\n", R[29].getData());
  }
}


mem_addr MIPS4KC::copy_str_to_stack (char *s)
{
  mem_addr str_start;
  int i = strlen (s);

  while (i >= 0)
    {
        SET_MEM_BYTE (R[29].getData(), s[i]);
        R[29] -= 1;
        i -= 1;
    }
  str_start = (mem_addr) R[29].getData() + 1;
  R[29] = R[29] & 0xfffffffc;	/* Round down to word boundary */
  return (str_start);
}


mem_addr MIPS4KC::copy_int_to_stack (int n)
{
    SET_MEM_WORD (R[29].getData(), n);
    R[29] -= BYTES_PER_WORD;
    return ((mem_addr) R[29].getData() + BYTES_PER_WORD);
}


/* Run a program starting at PC for N steps and display each
   instruction before executing if FLAG is non-zero.  If CONTINUE is
   non-zero, then step through a breakpoint.  Return non-zero if
   breakpoint is encountered. */

int MIPS4KC::run_program (mem_addr pc, int steps, int display, int cont_bkpt)
{
  fatal_error("no run program\n");
  return 0;
#if 0
  if (cont_bkpt && inst_is_breakpoint (pc))
    {
      mem_addr addr = PC == 0 ? pc : PC;

      delete_breakpoint (addr);
      exception_occurred = 0;
      run_spim (addr, 1, display);
      add_breakpoint (addr);
      steps -= 1;
      pc = PC;
    }

  exception_occurred = 0;
  if (!run_spim (pc, steps, display))
    /* Can't restart program */
    PC = 0;
  if (exception_occurred && Cause == (BKPT_EXCPT << 2))
    return (1);
  else
    return (0);
#endif
}


/* Record of where a breakpoint was placed and the instruction previously
   in memory. */

typedef struct bkptrec
{
  mem_addr addr;
  instruction *inst;
  struct bkptrec *next;
} bkpt;


static bkpt *bkpts = NULL;


#if 0  //AFR: no breakpoint support
/* Set a breakpoint at memory location ADDR. */

void MIPS4KC::add_breakpoint (mem_addr addr)
{
  bkpt *rec = (bkpt *) xmalloc (sizeof (bkpt));

  rec->next = bkpts;
  rec->addr = addr;

  if ((rec->inst = set_breakpoint (addr)) != NULL)
    bkpts = rec;
  else
    {
      if (exception_occurred)
	error ("Cannot put a breakpoint at address 0x%08x\n", addr);
      else
	error ("No instruction to breakpoint at address 0x%08x\n", addr);
      free (rec);
    }
}


/* Delete all breakpoints at memory location ADDR. */
#ifdef __STDC__
void
delete_breakpoint (mem_addr addr)
#else
void
delete_breakpoint (addr)
     mem_addr addr;
#endif
{
  bkpt *p, *b;
  int deleted_one = 0;

  for (p = NULL, b = bkpts; b != NULL; )
    if (b->addr == addr)
      {
	bkpt *n;

	SET_MEM_INST (addr, b->inst);
	if (p == NULL)
	  bkpts = b->next;
	else
	  p->next = b->next;
	n = b->next;
	free (b);
	b = n;
	deleted_one = 1;
      }
    else
      p = b, b = b->next;
  if (!deleted_one)
    error ("No breakpoint to delete at 0x%08x\n", addr);
}

#ifdef __STDC__
static void
delete_all_breakpoints (void)
#else
static void
delete_all_breakpoints ()
#endif
{
  bkpt *b, *n;

  for (b = bkpts, n = NULL; b != NULL; b = n)
    {
      n = b->next;
      free (b);
    }
  bkpts = NULL;
}


/* List all breakpoints. */

#ifdef __STDC__
void
list_breakpoints (void)
#else
void
list_breakpoints ()
#endif
{
  bkpt *b;

  if (bkpts)
    for (b = bkpts;  b != NULL; b = b->next)
      write_output (message_out, "Breakpoint at 0x%08x\n", b->addr);
  else
    write_output (message_out, "No breakpoints set\n");
}
#endif


/* Utility routines */

/* Print the error message then exit. */

/*VARARGS0*/
void MIPS4KC::fatal_error (const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  fmt = va_arg (args, char *);

#ifdef NO_VFPRINTF
  _doprnt (fmt, args, stderr);
#else
  vfprintf (stderr, fmt, args);
#endif
  exit (-1);
  /*NOTREACHED*/
}


/* Return the entry in the hash TABLE of length LENGTH with key STRING.
   Return NULL if no such entry exists. */

inst_info * MIPS4KC::map_string_to_inst_info (inst_info tbl[], int tbl_len, char *id)
{
  int low = 0;
  int hi = tbl_len - 1;

  while (low <= hi)
    {
      int mid = (low + hi) / 2;
      char *idp = id, *np = tbl[mid].name;

      while (*idp == *np && *idp != '\0') {idp ++; np ++;}

      if (*np == '\0' && *idp == '\0') /* End of both strings */
	return (& tbl[mid]);
      else if (*idp > *np)
	low = mid + 1;
      else
	hi = mid - 1;
    }

  return NULL;
}


/* Return the entry in the hash TABLE of length LENGTH with VALUE1 field NUM.
   Return NULL if no such entry exists. */

inst_info *MIPS4KC::map_int_to_inst_info (vector<inst_info> &tbl, int num)
{
    int tbl_len = tbl.size();
    int low = 0;
    int hi = tbl_len - 1;

  while (low <= hi)
    {
      int mid = (low + hi) / 2;
      if (tbl[mid].value1 == num)
	return (&tbl[mid]);
      else if (num > tbl[mid].value1)
	low = mid + 1;
      else
	hi = mid - 1;
    }
  printf(" ret null\n");
  return NULL;
}





#ifdef NEED_STRTOL
#ifdef __STDC__
unsigned long
strtol (const char* str, const char** eptr, int base)
#else
long
strtol (str, eptr, base)
     char *str, **eptr;
     int base;
#endif
{
  long result;

  if (base != 16)
    fatal_error ("SPIM's strtol only works for base 16 (not base %d)\n", base);
  if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X'))
    str += 2;
  sscanf (str, "%lx", &result);
  return (result);
}
#endif

#ifdef NEED_STRTOUL
#ifdef __STDC__
unsigned long
strtoul (const char* str, char** eptr, int base)
#else
unsigned long
strtoul (str, eptr, base)
     char *str, **eptr;
     int base;
#endif
{
  unsigned long result;

  if (base != 16)
    fatal_error ("SPIM's strtoul only works for base 16 (not base %d)\n",
		 base);
  if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X'))
    str += 2;
  sscanf (str, "%lx", &result);
  return (result);
}
#endif


char * MIPS4KC::str_copy (char *str)
{
    return (strcpy((char*)xmalloc(strlen(str) + 1), str));
}


void * MIPS4KC::xmalloc (int size)
{
  void *x = (void *) malloc (size);

  if (x == 0)
    fatal_error ("Out of memory at request for %d bytes.\n");
  return (x);
}


/* Allocate a zero'ed block of storage. */

void * MIPS4KC::zmalloc (int size)
{
#ifdef __STDC__
  void *z = (void *) malloc (size);
#else
  char *z = (char *) malloc (size);
#endif

  if (z == 0)
    fatal_error ("Out of memory at request for %d bytes.\n");

  memclr (z, size);
  return (z);
}

void MIPS4KC::write_output (FILE *fp, const char *fmt, ...)
{
  va_list args;
  FILE *f;
#ifndef __STDC__
  char *fmt;
  long fp;
#endif
  int restore_console_to_program = 0;

#ifdef __STDC__
  va_start (args, fmt);
  f = (FILE *) fp;		/* Not too portable... */
#else
  va_start (args);
  fp = va_arg (args, long);
  f = (FILE *) fp;		/* Not too portable... */
  fmt = va_arg (args, char *);
#endif

  if (f != 0)
#ifdef NO_VFPRINTF
    _doprnt (fmt, args, f);
#else
    vfprintf (f, fmt, args);
#endif
  else
#ifdef NO_VFPRINTF
    _doprnt (fmt, args, stdout);
#else
    vfprintf (stdout, fmt, args);
#endif
  fflush (f);
  va_end (args);

}

int MIPS4KC::run_error (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  //console_to_spim ();

  vfprintf (stderr, fmt, args);

  va_end (args);
  //longjmp (spim_top_level_env, 1);
  return (0);                   /* So it can be used in expressions */
}

void MIPS4KC::error (const char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);

  vfprintf (stderr, fmt, args);
  va_end (args);
}
