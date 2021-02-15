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

#ifndef _CL_CYCLE_H
#define _CL_CYCLE_H

#define FALSE 0
#define TRUE 1

#define set_ex_bypass(r, val)   \
{ EX_bp_reg = r;                \
  EX_bp_val = val;              \
}

#define set_mem_bypass(r, val)  \
{ MEM_bp_reg = r;               \
  MEM_bp_val = val;             \
}

#define read_R_reg(r)                                       \
  (((r) == 0)                                               \
      ? 0                                                   \
      : ((EX_bp_reg == (r))                                       \
         ? EX_bp_val                                             \
         : ((MEM_bp_reg == (r)) ? MEM_bp_val :  R[(r)])))

/* cc = 1 => CPR register, cc = 0 => CCR register  */
#define set_CP_bypass(z, r, value, cc)          \
{  CP_bp_reg = r;                               \
   CP_bp_val = value;                           \
   CP_bp_cc = cc;                               \
   CP_bp_z = z;                                 \
   }


/* cc = 1 => CPR register, cc = 0 => CCR register  */
#define read_CP_reg(z, r, cc)                                    \
  (((z == CP_bp_z) && (CP_bp_reg == (r))  && (cc == CP_bp_cc))   \
   ? CP_bp_val                                                   \
   : ((cc == 1) ? CPR[z][r] : CCR[z][r]))

#define bp_clear()                        \
{ EX_bp_reg = 0;                          \
  MEM_bp_reg = 0;                         \
  CP_bp_reg = 0;                          \
}


#define SIGN_BIT(X) ((X) & 0x80000000)

#define ARITH_OVFL(RESULT, OP1, OP2) (SIGN_BIT (OP1) == SIGN_BIT (OP2) \
				      && SIGN_BIT (OP1) != SIGN_BIT (RESULT))


#define VALUE(ps) (ps->value)
#define VALUE1(ps) (ps->value1)
#define ADDR(ps) (ps->addr_value)
#define PADDR(ps) (ps->paddr)
#define RNUM(ps) (ps->req_num)
#define STAGE(ps) (ps->stage)
#define Operand1(ps) (ps->op1)
#define Operand2(ps) (ps->op2)
#define Operand3(ps) (ps->op3)
#define FPoperand1(ps) (ps->fop1)
#define FPoperand2(ps) (ps->fop2)
#define FPvalue(ps) (ps->fval)
#define STAGE_PC(ps) (ps->pc)
#define DSLOT(ps) (ps->dslot)
#define EXCPT(ps) (ps->exception)
#define CYL_COUNT(ps) (ps->cyl_count)
#define Count(ps) (ps->count)



/* CU0 at bit 28, CU2 at bit 30, CU3 at bit 31 */
#define COP_Available(cp) 	((Status_Reg.getData() >> (28 + cp)) & 0x1)

#define IS_BRANCH(oc) \
   ((oc ==  Y_BC0F_OP)  || (oc == Y_BC2F_OP) || \
    (oc == Y_BC3F_OP)   || (oc == Y_BC0T_OP) || \
    (oc == Y_BC2T_OP)   || (oc == Y_BC3T_OP) || \
    (oc == Y_BEQ_OP)    || (oc == Y_BGEZ_OP) || \
    (oc == Y_BGEZAL_OP) || (oc == Y_BGTZ_OP) || \
    (oc == Y_BLEZ_OP)   || (oc == Y_BLTZ_OP) || \
    (oc == Y_BLTZAL_OP) || (oc == Y_BNE_OP)  || \
    (oc == Y_J_OP)      || (oc == Y_JAL_OP)  || \
    (oc == Y_JALR_OP)   || (oc == Y_JR_OP))

#define IS_MEM_OP(oc)                                              \
   ((oc == Y_LB_OP)   || (oc == Y_LBU_OP)  || (oc == Y_LH_OP)   || \
    (oc == Y_LHU_OP)  || (oc == Y_LW_OP)   || (oc == Y_LWL_OP)  || \
    (oc == Y_LWR_OP)  || \
    (oc == Y_LWC0_OP) || (oc == Y_LWC3_OP) || (oc == Y_LWC1_OP) || \
    (oc == Y_SB_OP)   || (oc == Y_SH_OP)   || (oc == Y_SWC1_OP) || \
    (oc == Y_SW_OP)   || (oc == Y_SWC0_OP) ||                      \
    (oc == Y_SWC3_OP) || (oc == Y_SWL_OP)  || (oc == Y_SWR_OP)  || \
    (oc == Y_LWC2_OP) || (oc == Y_SWC2_OP))

#define IS_LOAD_OP(oc)                                              \
   ((oc == Y_LB_OP)   || (oc == Y_LBU_OP)  || (oc == Y_LH_OP)   || \
    (oc == Y_LHU_OP)  || (oc == Y_LW_OP)   || (oc == Y_LWL_OP)  || \
    (oc == Y_LWR_OP)  || (oc == Y_LWC0_OP) || (oc == Y_LWC3_OP) || \
    (oc == Y_LWC1_OP) || (oc == Y_LWC2_OP))

#define IS_STORE_OP(oc)                                             \
    ((oc == Y_SB_OP)   || (oc == Y_SH_OP)   || (oc == Y_SWC1_OP) || \
     (oc == Y_SW_OP)   || (oc == Y_SWC0_OP) ||                      \
     (oc == Y_SWC3_OP) || (oc == Y_SWL_OP)  || (oc == Y_SWR_OP)  || \
     (oc == Y_SWC2_OP))



#define FPA_FWB 5
#define FPA_EX3 4
#define FPA_EX2 3
#define FPA_EX1 2
#define WB 4
#define MEM 3 
#define EX 2
#define ID 1
#define IF 0
#define DONE 0
#define IF_STALL -1
#define MEM_STALL -2  // issued, but not complete
#define STALL -3

#define ALU 0
#define FPA 1

#define PRINT_INT_SYSCALL	1
#define PRINT_FLOAT_SYSCALL	2
#define PRINT_DOUBLE_SYSCALL	3
#define PRINT_STRING_SYSCALL	4
#define READ_INT_SYSCALL	5
#define READ_FLOAT_SYSCALL	6
#define READ_DOUBLE_SYSCALL	7
#define READ_STRING_SYSCALL	8
#define SBRK_SYSCALL		9
#define EXIT_SYSCALL		10
#define GET_PROC_NUM_SYSCALL    11



struct mult_div_unit {
    int count;
    reg_word hi_val;
    reg_word lo_val;
    mult_div_unit() : count(0), hi_val(0), lo_val(0) {;}
};


#define MULT_COST 12
#define DIV_COST 33

/* floating point stuff */

/* Both fr and fr+1 must be set to 1 */
#define is_present(fr) (((FP_reg_present >> (fr)) & 0x1) &&            \
			((FP_reg_present >> (fr+1)) & 0x1))

#define is_single_present(fr) ((FP_reg_present >> (fr)) & 0x1)

#define set_present(fr)                                  \
{ FP_reg_present = FP_reg_present | (1 << fr) | (1 << (fr+1));          \
  }

#define set_single_present(fr)                                        \
{ FP_reg_present = FP_reg_present | (1 << (fr));                        \
  }

#define clr_present(fr)                                 \
{ FP_reg_present = FP_reg_present & (0xffffffff ^ (0x3 << fr));  \
  }

#define clr_single_present(fr)                                 \
{ FP_reg_present = FP_reg_present & (0xffffffff ^ (0x1 << (fr)));  \
  }

/* doesn't include BC1F, BC1T, MTC1, MFC1, or LWC1 which are mostly ALU operations */
#define is_fp_op(oc)                      \
  ((oc == Y_ABS_S_OP) ||                   \
   (oc == Y_ABS_D_OP) ||                   \
   (oc == Y_ADD_S_OP) ||                   \
   (oc == Y_ADD_D_OP) ||                   \
   (oc == Y_C_F_S_OP) ||                   \
   (oc == Y_C_UN_S_OP) ||                   \
   (oc == Y_C_EQ_S_OP) ||                   \
   (oc == Y_C_UEQ_S_OP) ||                   \
   (oc == Y_C_OLE_S_OP) ||                   \
   (oc == Y_C_ULE_S_OP) ||                   \
   (oc == Y_C_SF_S_OP) ||                   \
   (oc == Y_C_NGLE_S_OP) ||                   \
   (oc == Y_C_SEQ_S_OP) ||                   \
   (oc == Y_C_NGL_S_OP) ||                   \
   (oc == Y_C_LT_S_OP) ||                   \
   (oc == Y_C_NGE_S_OP) ||                   \
   (oc == Y_C_LE_S_OP) ||                   \
   (oc == Y_C_NGT_S_OP) ||                   \
   (oc == Y_C_F_D_OP) ||                   \
   (oc == Y_C_UN_D_OP) ||                   \
   (oc == Y_C_EQ_D_OP) ||                   \
   (oc == Y_C_UEQ_D_OP) ||                   \
   (oc == Y_C_OLE_D_OP) ||                   \
   (oc == Y_C_ULE_D_OP) ||                   \
   (oc == Y_C_SF_D_OP) ||                   \
   (oc == Y_C_NGLE_D_OP) ||                   \
   (oc == Y_C_SEQ_D_OP) ||                   \
   (oc == Y_C_NGL_D_OP) ||                   \
   (oc == Y_C_LT_D_OP) ||                   \
   (oc == Y_C_NGE_D_OP) ||                   \
   (oc == Y_C_LE_D_OP) ||                   \
   (oc == Y_C_NGT_D_OP) ||                   \
   (oc == Y_CFC1_OP) ||                   \
   (oc == Y_CTC1_OP) ||                   \
   (oc == Y_CVT_D_S_OP) ||                   \
   (oc == Y_CVT_D_W_OP) ||                   \
   (oc == Y_CVT_S_D_OP) ||                   \
   (oc == Y_CVT_S_W_OP) ||                   \
   (oc == Y_CVT_W_D_OP) ||                   \
   (oc == Y_CVT_W_S_OP) ||                   \
   (oc == Y_DIV_S_OP) ||                   \
   (oc == Y_DIV_D_OP) ||                   \
   (oc == Y_MOV_S_OP) ||                   \
   (oc == Y_MOV_D_OP) ||                   \
   (oc == Y_MUL_S_OP) ||                   \
   (oc == Y_MUL_D_OP) ||                   \
   (oc == Y_NEG_S_OP) ||                   \
   (oc == Y_NEG_D_OP) ||                   \
   (oc == Y_SUB_S_OP) ||                   \
   (oc == Y_SUB_D_OP))


#define is_store(oc)                      \
  ((oc == Y_SW_OP) ||                     \
   (oc == Y_SWL_OP) ||                    \
   (oc == Y_SWR_OP) ||                    \
   (oc == Y_SWC0_OP) ||                   \
   (oc == Y_SWC1_OP) ||                   \
   (oc == Y_SWC2_OP) ||                   \
   (oc == Y_SWC3_OP) ||                   \
   (oc == Y_SB_OP) ||                     \
   (oc == Y_SH_OP))


/* not currently in use; cycle spim never leaps to kernel code */
#define SHIFT_Status()							\
  Status_Reg = (Status_Reg & 0xffffffc0) | ((Status_Reg & 0xf) << 2)
#define RESTORE_Status() 						\
  Status_Reg = (Status_Reg & 0xffffffc0) | ((Status_Reg & 0x3c) >> 2)

#endif // _CL_CYCLE_H
