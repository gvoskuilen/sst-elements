/* SPIM S20 MIPS simulator.
   Description of a SPIM S20 instruction.
   (Layout does not correspond to MIPS machine.)
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


/* $Header: /var/home/larus/Software/larus/SPIM/RCS/inst.h,v 3.10 1994/01/18 03:21:45 larus Exp $
*/


/* Describes an expression that produce a value for an instruction's
   immediate field.  Immediates have the form: label +/- offset. */

#ifndef _INST_H
#define _INST_H

typedef struct immexpr
{
  long offset;			/* Offset from symbol */
  struct lab *symbol;		/* Symbolic label */
  short bits;			/* > 0 => 31..16, < 0 => 15..0 */
  short pc_relative;		/* Non-zero => offset from label in code */
} imm_expr;


/* Describes an expression that produce an address for an instruction.
   Address have the form: label +/- offset (register). */

typedef struct addrexpr
{
  unsigned char reg_no;		/* Register number */
  imm_expr *imm;		/* The immediate part */
} addr_expr;



/* Store the instruction fields in an overlapping manner similar to
   the real encoding. */

typedef struct inst_s
{
  short opcode;

  union
    {
      /* R-type or I-type: */
      struct
	{
	  unsigned char rs;
	  unsigned char rt;

	  union
	    {
	      short imm;

	      struct
		{
		  unsigned char rd;
		  unsigned char shamt;
		} r;
	    } r_i;
	} r_i;

      /* J-type: */
      long target;
    } r_t;

  unsigned long encoding;
  imm_expr *expr;
  char *source_line;
  bool faulted;
} instruction;


#define OPCODE(INST)	(INST)->opcode

#define RS(INST)	(INST)->r_t.r_i.rs
#define FS(INST)	(INST)->r_t.r_i.rs
#define BASE(INST)	(INST)->r_t.r_i.rs

#define RT(INST)	(INST)->r_t.r_i.rt
#define FT(INST)	(INST)->r_t.r_i.rt

#define RD(INST)	(INST)->r_t.r_i.r_i.r.rd
#define FD(INST)	(INST)->r_t.r_i.r_i.r.rd

#define SHAMT(INST)	(INST)->r_t.r_i.r_i.r.shamt

#define IMM(INST)	(INST)->r_t.r_i.r_i.imm
#define IOFFSET(INST)	(INST)->r_t.r_i.r_i.imm
#define COND(INST)	(INST)->r_t.r_i.r_i.imm

#define TARGET(INST)	(INST)->r_t.target

#define ENCODING(INST)	(INST)->encoding

#define EXPR(INST)	(INST)->expr

#define SOURCE(INST)	(INST)->source_line


#define COND_UN		0x1
#define COND_EQ		0x2
#define COND_LT		0x4
#define COND_IN		0x8



/* Raise an exception! */

#define RAISE_EXCEPTION(CAUSE, MISC)					\
	{								\
            if (((CAUSE)<= LAST_REAL_EXCEPT) || (Status_Reg & 0x1).getData()) \
	    {								\
	      Cause = (CAUSE) << 2;					\
	      exception_occurred = 1;					\
	      EPC = PC;							\
	      Status_Reg = (Status_Reg & 0xffffffc0) | ((Status_Reg & 0xf) << 2); \
	      MISC;							\
	    }								\
	}								\


#define CL_RAISE_EXCEPTION(enum, cnum, excpt)                 		\
  (excpt) = ((cnum) << 28) | ((enum) << 2) | 0x1;


/* Recognized exceptions (see Ch. 5): */

#define INT_EXCPT 0
#define MOD_EXCPT 1
#define TLBL_EXCPT 2
#define TLBS_EXCPT 3
#define ADDRL_EXCPT 4
#define ADDRS_EXCPT 5
#define IBUS_EXCPT 6
#define DBUS_EXCPT 7
#define SYSCALL_EXCPT 8
#define BKPT_EXCPT 9
#define RI_EXCPT 10
#define CPU_EXCPT 11
#define OVF_EXCPT 12

#define CACHEABLE 13
#define NOT_CACHEABLE 14


/* Floating point exceptions (Ch. 8): */

#define INEXACT_EXCEPT 13
#define INVALID_EXCEPT 14
#define DIV0_EXCEPT 15
#define FOVF_EXCEPT 16
#define FUNF_EXCEPT 17

#define LAST_REAL_EXCEPT FUNF_EXCEPT



/* Exported functions: */


#endif // _INST_H
