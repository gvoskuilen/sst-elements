/* SPIM S20 MIPS simulator.
   Code to build assembly instructions and resolve symbolic labels.
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


/* $Header: /var/home/larus/Software/larus/SPIM/RCS/inst.c,v 3.35 1994/07/15 17:53:46 larus Exp $
*/

#include <sst_config.h>
#include "mips_4kc.h"
#include <algorithm>

/*#include <stdio.h>
#include <string.h>

#include "spim.h"
#include "spim-utils.h"
#include "inst.h"
#include "mem.h"
#include "reg.h"
#include "sym-tbl.h"*/
#include "y.tab.h"
#include "op.h"
/*#include "parser.h"
#include "scanner.h"
#include "data.h"*/

using namespace SST;
using namespace SST::MIPS4KCComponent;

//make CXXFLAGS="-g -O0  -std=c++1y -D__STDC_FORMAT_MACROS"
#warning possible problems compiling in clang++ with optimization

/* Local variables: */

/* Map from opcode -> name/type. */

static vector<inst_info> name_tbl = {
#undef OP
#define OP(NAME, OPCODE, TYPE, R_OPCODE) inst_info(NAME, OPCODE, TYPE),
#include "op_tokens.h"
};

/* Sort the opcode table on their key (the opcode value). */
void MIPS4KC::sort_name_table (void)
{
  std::sort(name_tbl.begin(), name_tbl.end(), [](inst_info &a, inst_info &b){return a.value1 < b.value1;});

  sorted_name_table = 1;
}

/* Non-zero means store instructions in kernel, not user, text segment */



/* If FLAG is non-zero, next instruction goes to kernel text segment,
   otherwise it goes to user segment. */

void MIPS4KC::user_kernel_text_segment (int to_kernel)
{
  in_kernel = to_kernel;
}


/* Store an INSTRUCTION in memory at the next location. */

void MIPS4KC::store_instruction (instruction *inst, const mem_addr addr) {
  exception_occurred = 0;
  SET_MEM_INST (addr, inst);
  if (outputLevel > 0) {
    printf("storing %lx to %x\n", inst->encoding, addr);
  }
  if (exception_occurred)
      error ("Invalid address (0x%08x) for instruction\n", addr);

  if (inst != NULL) {
    if (ENCODING (inst) == 0)
      ENCODING (inst) = inst_encode (inst);
  }
}



/* Return a register-type instruction with the given OPCODE, RD, RS, and RT
   fields. */

instruction * MIPS4KC::make_r_type_inst (int opcode, int rd, int rs, int rt)
{
  instruction *inst = (instruction *) zmalloc (sizeof (instruction));

  OPCODE(inst) = opcode;
  RS(inst) = rs;
  RT(inst) = rt;
  RD(inst) = rd;
  SHAMT(inst) = 0;
  return (inst);
}

imm_expr *MIPS4KC::copy_imm_expr (imm_expr *old_expr)
{
  imm_expr *expr = (imm_expr *) xmalloc (sizeof (imm_expr));

  *expr = *old_expr;
  /*memcpy ((void*)expr, (void*)old_expr, sizeof (imm_expr));*/
  return (expr);
}


/* Make and return a deep copy of INST. */

instruction * MIPS4KC::copy_inst (instruction *inst)
{
  instruction *new_inst = (instruction *) xmalloc (sizeof (instruction));

  *new_inst = *inst;
  /*memcpy ((void*)new_inst, (void*)inst , sizeof (instruction));*/
  EXPR (new_inst) = copy_imm_expr (EXPR (inst));
  return (new_inst);
}


void MIPS4KC::free_inst (instruction *inst)
{
  if (inst != break_inst)
    /* Don't free the breakpoint insructions since we only have one. */
    {
      if (EXPR (inst))
	free (EXPR (inst));
      free (inst);
    }
}


/* Maintain a table mapping from opcode to instruction name and
   instruction type.

   Table must be sorted before first use since its entries are
   alphabetical on name, not ordered by opcode. */



/* Print the instruction stored at the memory ADDRESS. */

void MIPS4KC::print_inst (mem_addr addr)
{
  instruction *inst;
  char buf [128];

  exception_occurred = 0;
  READ_MEM_INST (inst, addr);

  if (exception_occurred)
    {
      error ("Can't print instruction not in text segment (0x%08x)\n", addr);
      return;
    }
  print_inst_internal (buf, 128, inst, addr);
  write_output (message_out, buf);
}


int MIPS4KC::print_inst_internal (char *buf, int length, instruction *inst, mem_addr addr)
{
  char *bp = buf;
  inst_info *entry;

  if (!sorted_name_table)
    sort_name_table ();

  sprintf (buf, "[0x%08x]\t", addr);
  buf += strlen (buf);
  if (inst == NULL)
    {
      sprintf (buf, "<none>\n");
      buf += strlen (buf);
      return (buf - bp);
    }
  entry = map_int_to_inst_info (name_tbl,
				OPCODE (inst));
  if (entry == NULL)
    {
      sprintf (buf, "<unknown instruction %d>\n", OPCODE (inst));
      buf += strlen (buf);
      return (buf - bp);
    }
  sprintf (buf, "0x%08lx  %s", ENCODING (inst), entry->name);
  buf += strlen (buf);
  switch (entry->value2)
    {
    case B0_TYPE_INST:
      sprintf (buf, " %d", SIGN_EX (IMM (inst) << 2));
      buf += strlen (buf);
      break;

    case B1_TYPE_INST:
      sprintf (buf, " $%d %d", RS (inst), SIGN_EX (IMM (inst) << 2));
      buf += strlen (buf);
      break;

    case I1t_TYPE_INST:
      sprintf (buf, " $%d, %d", RT (inst), IMM (inst));
      buf += strlen (buf);
      break;

    case I2_TYPE_INST:
      sprintf (buf, " $%d, $%d, %d", RT (inst), RS (inst), IMM (inst));
      buf += strlen (buf);
      break;

    case B2_TYPE_INST:
      sprintf (buf, " $%d, $%d, %d", RS (inst), RT (inst),
	       SIGN_EX (IMM (inst) << 2));
      buf += strlen (buf);
      break;

    case I2a_TYPE_INST:
      sprintf (buf, " $%d, %d($%d)", RT (inst), IMM (inst), BASE (inst));
      buf += strlen (buf);
      break;

    case R1s_TYPE_INST:
      sprintf (buf, " $%d", RS (inst));
      buf += strlen (buf);
      break;

    case R1d_TYPE_INST:
      sprintf (buf, " $%d", RD (inst));
      buf += strlen (buf);
      break;

    case R2td_TYPE_INST:
      sprintf (buf, " $%d, $%d", RT (inst), RD (inst));
      buf += strlen (buf);
      break;

    case R2st_TYPE_INST:
      sprintf (buf, " $%d, $%d", RS (inst), RT (inst));
      buf += strlen (buf);
      break;

    case R2ds_TYPE_INST:
      sprintf (buf, " $%d, $%d", RD (inst), RS (inst));
      buf += strlen (buf);
      break;

    case R2sh_TYPE_INST:
      if (ENCODING (inst) == 0)
	{
	  buf -= 3;		/* zap sll */
	  sprintf (buf, "nop");
	}
      else
	sprintf (buf, " $%d, $%d, %d", RD (inst), RT (inst), SHAMT (inst));
      buf += strlen (buf);
      break;

    case R3_TYPE_INST:
      sprintf (buf, " $%d, $%d, $%d", RD (inst), RS (inst), RT (inst));
      buf += strlen (buf);
      break;

    case R3sh_TYPE_INST:
      sprintf (buf, " $%d, $%d, $%d", RD (inst), RT (inst), RS (inst));
      buf += strlen (buf);
      break;

    case FP_I2a_TYPE_INST:
      sprintf (buf, " $f%d, %d($%d)", FT (inst), IMM (inst), BASE (inst));
      buf += strlen (buf);
      break;

    case FP_R2ds_TYPE_INST:
      sprintf (buf, " $f%d, $f%d", FD (inst), FS (inst));
      buf += strlen (buf);
      break;

    case FP_R2st_TYPE_INST:
      sprintf (buf, " $f%d, $f%d", FS (inst), FT (inst));
      buf += strlen (buf);
      break;

    case FP_R3_TYPE_INST:
      sprintf (buf, " $f%d, $f%d, $f%d", FD (inst), FS (inst), FT (inst));
      buf += strlen (buf);
      break;

    case FP_MOV_TYPE_INST:
      sprintf (buf, " $f%d, $f%d", FD (inst), FS (inst));
      buf += strlen (buf);
      break;

    case J_TYPE_INST:
      sprintf (buf, " 0x%08lx", TARGET (inst) << 2);
      buf += strlen (buf);
      break;

    case CP_TYPE_INST:
      sprintf (buf, " $%d, $%d", RT (inst), RD (inst));
      buf += strlen (buf);
      break;

    case NOARG_TYPE_INST:
      break;

    case ASM_DIR:
    case PSEUDO_OP:
    default:
      fatal_error ("Unknown instruction type in print_inst\n");
    }

  if (EXPR (inst) != NULL && EXPR (inst)->symbol != NULL)
    {
      sprintf (buf, " [");
      buf += strlen (buf);
      if (opcode_is_load_store (OPCODE (inst)))
	buf += print_imm_expr (buf, EXPR (inst), BASE (inst));
      else
	buf += print_imm_expr (buf, EXPR (inst), -1);
      sprintf (buf, "]");
      buf += strlen (buf);
    }

  if (SOURCE (inst) != NULL)
    {
      int line_length = buf - bp;
      int comment_length = 2 + strlen (SOURCE (inst));

      /* Make sure comment (source line text) fits on current line. */
      if (length > 50)
	{
	  for ( ; line_length < 50; line_length ++)
	    {
	      sprintf (buf, " ");
	      buf += 1;
	    }
	  if (line_length + comment_length >= length)
	    SOURCE (inst) [length - line_length - 1] = '\0';
	  sprintf (buf, "; %s", SOURCE (inst));
	  buf += strlen (buf);
	}
    }
  sprintf (buf, "\n");
  buf += strlen (buf);
  return (buf - bp);
}



/* Return non-zero if an INSTRUCTION is a conditional branch. */

#ifdef __STDC__
int
opcode_is_branch (int opcode)
#else
int
opcode_is_branch (opcode)
     int opcode;
#endif
{
  switch (opcode)
    {
    case Y_BEQ_OP:
    case Y_BEQZ_POP:
    case Y_BGE_POP:
    case Y_BGEU_POP:
    case Y_BGEZ_OP:
    case Y_BGEZAL_OP:
    case Y_BGT_POP:
    case Y_BGTU_POP:
    case Y_BGTZ_OP:
    case Y_BLE_POP:
    case Y_BLEU_POP:
    case Y_BLEZ_OP:
    case Y_BLT_POP:
    case Y_BLTU_POP:
    case Y_BLTZ_OP:
    case Y_BLTZAL_OP:
    case Y_BNE_OP:
    case Y_BNEZ_POP:
    case Y_BC1F_OP:
    case Y_BC1T_OP:
      return (1);

    default:
      return (0);
    }
}


/* Return non-zero if an INSTRUCTION is an conditional branch (jump). */

#ifdef __STDC__
int
opcode_is_jump (int opcode)
#else
int
opcode_is_jump (opcode)
     int opcode;
#endif
{
  switch (opcode)
    {
    case Y_J_OP:
    case Y_JAL_OP:
      return (1);

    default:
      return (0);
    }
}

/* Return non-zero if an INSTRUCTION is a load or store. */

#ifdef __STDC__
int
opcode_is_load_store (int opcode)
#else
int
opcode_is_load_store (opcode)
     int opcode;
#endif
{
  switch (opcode)
    {
    case Y_LB_OP: return (1);
    case Y_LBU_OP: return (1);
    case Y_LH_OP: return (1);
    case Y_LHU_OP: return (1);
    case Y_LW_OP: return (1);
    case Y_LWC0_OP: return (1);
    case Y_LWC1_OP: return (1);
    case Y_LWC2_OP: return (1);
    case Y_LWC3_OP: return (1);
    case Y_LWL_OP: return (1);
    case Y_LWR_OP: return (1);
    case Y_SB_OP: return (1);
    case Y_SH_OP: return (1);
    case Y_SW_OP: return (1);
    case Y_SWC0_OP: return (1);
    case Y_SWC1_OP: return (1);
    case Y_SWC2_OP: return (1);
    case Y_SWC3_OP: return (1);
    case Y_SWL_OP: return (1);
    case Y_SWR_OP: return (1);
    case Y_L_D_POP: return (1);
    case Y_L_S_POP: return (1);
    case Y_S_D_POP: return (1);
    case Y_S_S_POP: return (1);
    default: return (0);
    }
}


/* Return non-zero if a breakpoint is set at ADDR. */

int MIPS4KC::inst_is_breakpoint (mem_addr addr)
{
  instruction *old_inst;

  if (break_inst == NULL)
    break_inst = make_r_type_inst (Y_BREAK_OP, 1, 0, 0);

  READ_MEM_INST (old_inst, addr);
  return (old_inst == break_inst);
}


/* Set a breakpoint at ADDR and return the old instruction.  If the
   breakpoint cannot be set, return NULL. */

instruction * MIPS4KC::set_breakpoint (mem_addr addr)
{
  instruction *old_inst;

  if (break_inst == NULL)
    break_inst = make_r_type_inst (Y_BREAK_OP, 1, 0, 0);

  exception_occurred = 0;
  READ_MEM_INST (old_inst, addr);
  if (old_inst == break_inst)
    return (NULL);
  SET_MEM_INST (addr, break_inst);
  if (exception_occurred)
    return (NULL);
  else
    return (old_inst);
}



/* An immediate expression has the form: SYMBOL +/- IOFFSET, where either
   part may be omitted. */


/* Return a shallow copy of an EXPRESSION that only uses the upper
   sixteen bits of the expression's value. */

/* Return a shallow copy of the EXPRESSION with the offset field
   incremented by the given amount. */

/*imm_expr * MIPS4KC::incr_expr_offset (imm_expr *expr, long int value)
{
  imm_expr *new_expr = copy_imm_expr (expr);

  new_expr->offset += value;
  return (new_expr);
  }*/


imm_expr * MIPS4KC::addr_expr_imm (addr_expr *expr)
{
  return (expr->imm);
}


int MIPS4KC::addr_expr_reg (addr_expr *expr)
{
  return (expr->reg_no);
}



/* Map between a SPIM instruction and the binary representation of the
   instruction. */


/* Maintain a table mapping from internal opcode (i_opcode) to actual
   opcode (a_opcode).  Table must be sorted before first use since its
   entries are alphabetical on name, not ordered by opcode. */




/* Map from internal opcode -> real opcode */

static vector<inst_info> i_opcode_tbl = {
#undef OP
#define OP(NAME, I_OPCODE, TYPE, A_OPCODE) inst_info(NAME, I_OPCODE, A_OPCODE),
#include "op_tokens.h"
};


/* Sort the opcode table on their key (the interal opcode value). */

void MIPS4KC::sort_i_opcode_table (void)
{
    sort(i_opcode_tbl.begin(), i_opcode_tbl.end(), [](inst_info &a, inst_info &b){return a.value1 < b.value1;});
    sorted_i_opcode_table = 1;
}


#define REGS(R,O) (((R) & 0x1f) << O)


long MIPS4KC::inst_encode (instruction *inst)
{
  long a_opcode = 0;
  inst_info *entry;

  if (inst == NULL)
    return (0);
  if (!sorted_i_opcode_table)
    sort_i_opcode_table ();
  if (!sorted_name_table)
    sort_name_table ();

  entry = map_int_to_inst_info (i_opcode_tbl,
				OPCODE (inst));
  if (entry == NULL)
    return 0;

  a_opcode = entry->value2;
  entry = map_int_to_inst_info (name_tbl,
				OPCODE (inst));

  switch (entry->value2)
    {
    case B0_TYPE_INST:
      return (a_opcode
	      | (IOFFSET (inst) & 0xffff));

    case B1_TYPE_INST:
      return (a_opcode
	      | REGS (RS (inst), 21)
	      | (IOFFSET (inst) & 0xffff));

    case I1t_TYPE_INST:
      return (a_opcode
	      | REGS (RS (inst), 21)
	      | REGS (RT (inst), 16)
	      | (IMM (inst) & 0xffff));

    case I2_TYPE_INST:
    case B2_TYPE_INST:
      return (a_opcode
	      | REGS (RS (inst), 21)
	      | REGS (RT (inst), 16)
	      | (IMM (inst) & 0xffff));

    case I2a_TYPE_INST:
      return (a_opcode
	      | REGS (BASE (inst), 21)
	      | REGS (RT (inst), 16)
	      | (IOFFSET (inst) & 0xffff));

    case R1s_TYPE_INST:
      return (a_opcode
	      | REGS (RS (inst), 21));

    case R1d_TYPE_INST:
      return (a_opcode
	      | REGS (RD (inst), 11));

    case R2td_TYPE_INST:
      return (a_opcode
	      | REGS (RT (inst), 16)
	      | REGS (RD (inst), 11));

    case R2st_TYPE_INST:
      return (a_opcode
	      | REGS (RS (inst), 21)
	      | REGS (RT (inst), 16));

    case R2ds_TYPE_INST:
      return (a_opcode
	      | REGS (RS (inst), 21)
	      | REGS (RD (inst), 11));

    case R2sh_TYPE_INST:
      return (a_opcode
	      | REGS (RT (inst), 16)
	      | REGS (RD (inst), 11)
	      | REGS (SHAMT (inst), 6));

    case R3_TYPE_INST:
      return (a_opcode
	      | REGS (RS (inst), 21)
	      | REGS (RT (inst), 16)
	      | REGS (RD (inst), 11));

    case R3sh_TYPE_INST:
      return (a_opcode
	      | REGS (RS (inst), 21)
	      | REGS (RT (inst), 16)
	      | REGS (RD (inst), 11));

    case FP_I2a_TYPE_INST:
      return (a_opcode
	      | REGS (BASE (inst), 21)
	      | REGS (RT (inst), 16)
	      | (IOFFSET (inst) & 0xffff));

    case FP_R2ds_TYPE_INST:
      return (a_opcode
	      | REGS (FS (inst), 11)
	      | REGS (FD (inst), 6));

    case FP_R2st_TYPE_INST:
      return (a_opcode
	      | REGS (FT (inst), 16)
	      | REGS (FS (inst), 11));

    case FP_R3_TYPE_INST:
      return (a_opcode
	      | REGS (FT (inst), 16)
	      | REGS (FS (inst), 11)
	      | REGS (FD (inst), 6));

    case FP_MOV_TYPE_INST:
      return (a_opcode
	      | REGS (FS (inst), 11)
	      | REGS (FD (inst), 6));

    case J_TYPE_INST:
      return (a_opcode
	      | TARGET (inst));

    case CP_TYPE_INST:
      return (a_opcode
	      | REGS (RT (inst), 16)
	      | REGS (RD (inst), 11));

    case NOARG_TYPE_INST:
      return (a_opcode);

    case ASM_DIR:
    case PSEUDO_OP:
    default:
      fatal_error ("Unknown instruction type in inst_encoding\n");
      return (0);		/* Not reached */
    }
}







/* Map from internal opcode -> real opcode */
static vector<inst_info> a_opcode_tbl = {
#undef OP
#define OP(NAME, I_OPCODE, TYPE, A_OPCODE) inst_info(NAME, A_OPCODE, I_OPCODE),
#include "op_tokens.h"
};



/* Sort and populate the opcode table on their key (the interal opcode value). */

void MIPS4KC::sort_a_opcode_table (void)
{
    sort(a_opcode_tbl.begin(), a_opcode_tbl.end(), [](inst_info &a, inst_info &b){return a.value1 < b.value1;});
    sorted_a_opcode_table = 1;
}


#define REG(V,O) ((V) >> O) & 0x1f


instruction * MIPS4KC::inst_decode (unsigned long int value)
{
  long a_opcode = value & 0xfc000000;
  inst_info *entry;
  long i_opcode;

  if (a_opcode == 0)		/* SPECIAL */
    a_opcode |= (value & 0x3f);
  else if (a_opcode == 0x04000000) /* BCOND */
    a_opcode |= (value & 0x001f0000);
  else if (a_opcode == 0x40000000) /* COP0 */
    a_opcode |= (value & 0x03e00000) | (value & 0x1f);
  else if (a_opcode == 0x44000000) /* COP1 */
    {
      a_opcode |= (value & 0x03e00000);
      if ((value & 0xff000000) == 0x45000000)
	a_opcode |= (value & 0x00010000); /* BC1f/t */
      else
	a_opcode |= (value & 0x3f);
    }
  else if (a_opcode == 0x48000000 /* COPz */
	   || a_opcode == 0x4c000000)
    a_opcode |= (value & 0x03e00000);


  if (!sorted_a_opcode_table)
    sort_a_opcode_table ();
  if (!sorted_name_table)
    sort_name_table ();

  entry = map_int_to_inst_info (a_opcode_tbl,
				a_opcode);
  if (entry == NULL)
    return (mk_r_inst (value, 0, 0, 0, 0, 0)); /* Invalid inst */

  i_opcode = entry->value2;

  switch (map_int_to_inst_info (name_tbl,
				i_opcode)->value2)
    {
    case B0_TYPE_INST:
      return (mk_i_inst (value, i_opcode, 0, 0, value & 0xffff));

    case B1_TYPE_INST:
      return (mk_i_inst (value, i_opcode, REG (value, 21), 0, value & 0xffff));

    case I1t_TYPE_INST:
      return (mk_i_inst (value, i_opcode, REG (value, 21), REG (value, 16),
			 value & 0xffff));

    case I2_TYPE_INST:
    case B2_TYPE_INST:
      return (mk_i_inst (value, i_opcode, REG (value, 21), REG (value, 16),
			 value & 0xffff));

    case I2a_TYPE_INST:
      return (mk_i_inst (value, i_opcode, REG (value, 21), REG (value, 16),
			 value & 0xffff));

    case R1s_TYPE_INST:
      return (mk_r_inst (value, i_opcode, REG (value, 21), 0, 0, 0));

    case R1d_TYPE_INST:
      return (mk_r_inst (value, i_opcode, 0, 0, REG (value, 11), 0));

    case R2td_TYPE_INST:
      return (mk_r_inst (value, i_opcode, 0, REG (value, 16), REG (value, 11),
			 0));

    case R2st_TYPE_INST:
      return (mk_r_inst (value, i_opcode, REG (value, 21), REG (value, 16),
			 0, 0));

    case R2ds_TYPE_INST:
      return (mk_r_inst (value, i_opcode, REG (value, 21), 0, REG (value, 11),
			 0));

    case R2sh_TYPE_INST:
      return (mk_r_inst (value, i_opcode, 0, REG (value, 16), REG (value, 11),
			 REG (value, 6)));

    case R3_TYPE_INST:
      return (mk_r_inst (value, i_opcode, REG (value, 21), REG (value, 16),
			 REG (value, 11), 0));

    case R3sh_TYPE_INST:
      return (mk_r_inst (value, i_opcode, REG (value, 21), REG (value, 16),
			 REG (value, 11), 0));

    case FP_I2a_TYPE_INST:
      return (mk_i_inst (value, i_opcode, REG (value, 21), REG (value, 16),
			 value & 0xffff));

    case FP_R2ds_TYPE_INST:
      return (mk_r_inst (value, i_opcode, REG (value, 11), 0, REG (value, 6),
			 0));

    case FP_R2st_TYPE_INST:
      return (mk_r_inst (value, i_opcode, REG (value, 11), REG (value, 16), 0,
			 0));

    case FP_R3_TYPE_INST:
      return (mk_r_inst (value, i_opcode, REG (value, 11), REG (value, 16),
			 REG (value, 6), 0));

    case FP_MOV_TYPE_INST:
      return (mk_r_inst (value, i_opcode, REG (value, 11), 0, REG (value, 6),
			 0));

    case J_TYPE_INST:
      return (mk_j_inst (value, i_opcode, value & 0x2ffffff));

    case CP_TYPE_INST:
      return (mk_r_inst (value, i_opcode, 0, REG (value, 16), REG (value, 11),
			 0));

    case NOARG_TYPE_INST:
      return (mk_r_inst (value, i_opcode, 0, 0, 0, 0));

    case ASM_DIR:
    case PSEUDO_OP:
    default:
      return (mk_r_inst (value, 0, 0, 0, 0, 0)); /* Invalid inst */
    }
}


instruction * MIPS4KC::mk_r_inst (unsigned long value, int opcode, int rs, int rt, int rd, int shamt)
{
  instruction *inst = (instruction *) zmalloc (sizeof (instruction));

  OPCODE (inst) = opcode;
  RS (inst) = rs;
  RT (inst) = rt;
  RD (inst) = rd;
  SHAMT (inst) = shamt;
  ENCODING (inst) = value;
  EXPR (inst) = NULL;
  return (inst);
}


instruction * MIPS4KC::mk_i_inst (unsigned long value, int opcode, int rs, int rt, int offset)
{
  instruction *inst = (instruction *) zmalloc (sizeof (instruction));

  OPCODE (inst) = opcode;
  RS (inst) = rs;
  RT (inst) = rt;
  IOFFSET (inst) = offset;
  ENCODING (inst) = value;
  EXPR (inst) = NULL;
  return (inst);
}

instruction * MIPS4KC::mk_j_inst (unsigned long value, int opcode, int target)
{
  instruction *inst = (instruction *) zmalloc (sizeof (instruction));

  OPCODE (inst) = opcode;
  TARGET (inst) = target;
  ENCODING (inst) = value;
  EXPR (inst) = NULL;
  return (inst);
}




void MIPS4KC::inst_cmp (instruction *inst1, instruction *inst2)
{
  char buf[1024];

  if (memcmp (inst1, inst2, sizeof (instruction) - 4) != 0)
    {
      printf ("=================== Not Equal ===================\n");
      print_inst_internal (buf, 1024, inst1, 0);
      printf ("%s\n", buf);
      print_inst_internal (buf, 1024, inst2, 0);
      printf ("%s\n", buf);
      printf ("=================== Not Equal ===================\n");
    }
}


