/* SPIM S20 MIPS simulator.
   This file describes the MIPS instructions, the assembler pseudo
   instructions, the assembler pseudo-ops, and the spim commands.
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


/* $Header: /var/home/larus/Software/larus/SPIM/RCS/op.h,v 3.12 1994/01/18 03:21:45 larus Exp $
*/

#ifndef OP_H_
#define OP_H_  

/* Type of each entry: */

#define ASM_DIR		0
#define PSEUDO_OP	2

#define B0_TYPE_INST	4
#define B1_TYPE_INST	5
#define I1t_TYPE_INST	6
#define I2_TYPE_INST	7
#define B2_TYPE_INST	25
#define I2a_TYPE_INST	8

#define R1s_TYPE_INST	9
#define R1d_TYPE_INST	10
#define R2td_TYPE_INST	11
#define R2st_TYPE_INST	12
#define R2ds_TYPE_INST	13
#define R2sh_TYPE_INST	14
#define R3_TYPE_INST	15
#define R3sh_TYPE_INST	16

#define FP_I2a_TYPE_INST	17
#define FP_R2ds_TYPE_INST	18
#define FP_R2st_TYPE_INST	19
#define FP_R3_TYPE_INST	20
#define FP_MOV_TYPE_INST 21

#define J_TYPE_INST	22
#define CP_TYPE_INST	23
#define NOARG_TYPE_INST 24

#endif // OP_H_
