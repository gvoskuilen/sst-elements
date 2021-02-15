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

#ifdef mips
#define _IEEE 1
#include <nan.h>
#else
#define NaN(X) 0
#endif

#include <math.h>
#include <stdio.h>

#include "y.tab.h"

#include "cl-cache.h"
#include "cl-cycle.h"
#include "cl-tlb.h"
#include "cl-except.h"

using namespace SST;
using namespace SST::MIPS4KCComponent;

// ugly - essentially a goto
#define END_OF_CYCLE 	{						\
			if (display) print_pipeline ();			\
			continue;					\
			}

void MIPS4KC::sendRequestToCache(PIPE_STAGE ps, bool isLoad, size_t memSz, 
                                 memReq::dataVec &data){
    using namespace Interfaces;

    memReq::Command cmd;
    if (isLoad) {
        cmd = memReq::Read;
    } else {
        cmd = memReq::Write;
        // record outoging faulted addresses
        reg_word::checkRecordWriteFaults(ADDR(ps), VALUE(ps), memSz);
    }

    SimpleMem::Request *req = 
        new memReq(cmd, ADDR(ps).getData(), memSz, data);
    // make all requests not cacheable for now - add TLB or cache
    // flush later
    req->flags |= memReq::F_NONCACHEABLE;
    memory->sendRequest(req);
    requestsOut.insert(std::make_pair(req->id, ps));
    //printf(" sent id:%llx to cache\n", req->id);
}

/* 
 * issue memory requests to cache
 */
void MIPS4KC::process_rising_MEM (PIPE_STAGE ps) {    
    instruction *inst = ps->inst;

    if (outputLevel > 0) {
        printf("rising_MEM issuing addr: 0x%x ", ADDR(ps).getData());
        print_inst(ps->pc.getData()); 
    }
    
    size_t memSize = 0;
    Interfaces::SimpleMem::Request::dataVec data;

    switch (OPCODE (inst)) {
        /* 'normal' memory operations */
    case Y_LB_OP:
    case Y_LBU_OP:
        if (memSize == 0) memSize = 1;
    case Y_LH_OP:
    case Y_LHU_OP:
        if (memSize == 0) memSize = 2;
    case Y_LW_OP:
        if (memSize == 0) memSize = 4;
        faultChecker.checkAndInject_MEM_PRE(ADDR(ps), VALUE(ps), 1);
        sendRequestToCache(ps, true, memSize, data);    
        break;

    case Y_SB_OP:
        if (memSize == 0) {
            faultChecker.checkAndInject_MEM_PRE(ADDR(ps), VALUE(ps), 0);
            memSize = 1;
            data.resize(memSize);
            data[0] = (uint8_t)(VALUE(ps).getData() & 0xff);
        }
    case Y_SH_OP:
        if (memSize == 0) {
            faultChecker.checkAndInject_MEM_PRE(ADDR(ps), VALUE(ps), 0);
            memSize = 2;
            data.resize(memSize);
            data[1] = (uint8_t)((VALUE(ps).getData()>>8) & 0xff);
            data[0] = (uint8_t)((VALUE(ps).getData()>>0) & 0xff);
        }
    case Y_SW_OP:
        if (memSize == 0) {
            faultChecker.checkAndInject_MEM_PRE(ADDR(ps), VALUE(ps), 0);
            memSize = 4;
            data.resize(memSize);
            data[3] = (uint8_t)((VALUE(ps).getData()>>24) & 0xff);
            data[2] = (uint8_t)((VALUE(ps).getData()>>16) & 0xff);
            data[1] = (uint8_t)((VALUE(ps).getData()>>8) & 0xff);
            data[0] = (uint8_t)((VALUE(ps).getData()>>0) & 0xff);
        }
        //printf("store to cache: %x\n", ADDR(ps).getData());
        sendRequestToCache(ps, false, memSize, data);  
        break;

        /* coprocessor */
    case Y_LWC0_OP: case Y_LWC2_OP: case Y_LWC3_OP: case Y_SWC0_OP:
    case Y_SWC2_OP: case Y_SWC3_OP: case Y_CFC0_OP: case Y_CFC2_OP:
    case Y_CFC3_OP: case Y_MFC0_OP: case Y_MFC2_OP: case Y_MFC3_OP:
    case Y_CTC0_OP: case Y_CTC2_OP: case Y_CTC3_OP: case Y_MTC0_OP:
    case Y_MTC2_OP: case Y_MTC3_OP:
        fatal_error("Coprocessors not supported");
        break;

        /* FPU */
    case Y_LWC1_OP: case Y_SWC1_OP: case Y_MFC1_OP: case Y_LWL_OP:
    case Y_LWR_OP: case Y_SWL_OP: case Y_SWR_OP:
        fatal_error("FPU not supported");
        break;

    default:
        /*do nothing: case Y_SYSCALL_OP:     case Y_TLBP_OP:    case Y_TLBR_OP:
          case Y_TLBWI_OP:    case Y_TLBWR_OP:*/
        break;
    }
}


/*
 * function cl_run_rising: issue memory requests
 */
void MIPS4KC::cl_run_rising () {
    PIPE_STAGE ps_ptr = alu[MEM];
    if (ps_ptr != NULL) {            
        if (IS_MEM_OP(OPCODE(ps_ptr->inst))) {  // if it is a load            
            if (ps_ptr->issued_to_cache == 0) { // not already issued
                process_rising_MEM(ps_ptr);
                ps_ptr->issued_to_cache = 1;
                STAGE(ps_ptr) = MEM_STALL;
            }
        }
    }
}

/* function cl_run_falling:
 * responsible for calling cycle_spim, simulating falling edge of clock
 */

int MIPS4KC::cl_run_falling (reg_word addr, int display)
{
    // check for register file & PC faults
    faultChecker.checkAndInject_RF(R);
    faultChecker.checkAndInject_PC_FAULT(addr);
    
    PC = addr;
    cycle_running = 1;

    int ret = cycle_spim (display);

    if (ret == 1) {        
	switch (process_excpt ()) {
            
            /* exception processed ok, continue execution */
	case 0:
            cycle_init ();
            break;
            
            /* exception processed ok, but stop execution */
	case 1:
            cycle_init ();
            return true;
            
            /* bad exception, signal, or exit; reinit for next time */
	case -1:
            PC = 0;
            nPC = 0;
            cycle_init ();
            kill_prog_fds ();
            cycle_running = 0;            
            return true;
	}
    } else if (ret == 2) {
        // we're done
        return true;
    }

    return false;
}


/* for initialization of counting stuff and between runs of program */

void MIPS4KC::cl_initialize_world (int run)
{
  initialize_registers ();
  initialize_run_stack (0, 0);
  if (cycle_level) {
      if (run) {
          initialize_prog_fds ();
      }
      cycle_init ();
      mdu_and_fp_init ();
  }
}


/* cleanup after exceptions */

void MIPS4KC::cycle_init (void)
{
  R[0] = 0;
  EPC = 0;
  Cause = 0;
  bd_slot = FALSE;
  bp_clear ();
  pipe_dealloc (WB+1, alu);

  program_break = ((program_break / 32) + 1) * 32;
}


/* should be called when floating point and mdu unit are starting fresh */

void MIPS4KC::mdu_and_fp_init (void)
{
  MDU.count = 0;
  FP_add_cnt = 0;
  FP_mul_cnt = 0;
  FP_div_cnt = 0;
  FP_reg_present = 0xffffffff;
  HI_present = 1;
  LO_present = 1;
}


/* function cycle_spim:
 * responsible for stepping through code until an exception occurs; returns
 * 1 upon finding an exception, 0 upon completing number of steps.
 */

int MIPS4KC::cycle_spim (int display)
{
  PIPE_STAGE ps_ptr;
  mem_addr bus_req;
  int id_stall = FALSE, mem_stall = FALSE, cmiss = 0;

  do { // ugly hack to all us to 'continue' out of this loop

    bus_req = bus_service();

#if 0
    /* increment Random register each cycle */
    Random = (((Random >> 8) < 63) ? ((Random >> 8) + 1) : 8) << 8;
#endif
    cmiss = 0;

    /* Service mult/div unit */
    if (MDU.count == 1) {
      MDU.count = 0;
      HI_present = 1;
      HI = MDU.hi_val;
      LO_present = 1;
      LO = MDU.lo_val;
    }
    else if (MDU.count > 1)
      MDU.count--;

    /* Write Back Stage */
    ps_ptr = alu[WB];
    if (ps_ptr != NULL) {
      if (EXCPT (ps_ptr) != 0) {
	/* exception is about to be handled, set up registers */
	Cause = EXCPT (ps_ptr);
	if (DSLOT(ps_ptr)) Cause |= 0x80000000;
	else Cause &= 0x7fffffff;
	EPC = STAGE_PC(ps_ptr) - (DSLOT (ps_ptr) ? BYTES_PER_WORD : 0);
	return (1);
      }

      process_WB(ps_ptr);
      reg_word::countINSTFaults(STAGE_PC(ps_ptr), ps_ptr->inst->faulted);
      
      /* maintain invariant */
      R[0] = 0;
      stage_dealloc(ps_ptr);
      alu[WB] = NULL;
    }

    /* MEM stage - check for completion
       stage should be MEM_STALL (waiting for cache) or MEM (good to go)
    */
    if (alu[WB] == NULL) {
        ps_ptr = alu[MEM];
        if (ps_ptr != NULL) {
            //printf("MEM: %s\n", (STAGE(ps_ptr)==MEM_STALL) ? "MEM_STALL" : "MEM");
            if (STAGE (ps_ptr) == MEM_STALL) { // waiting on cache
                assert(ps_ptr->issued_to_cache);
                // check for completion from cache
                memReq *req = check_cacheComplete(ps_ptr);
                if (req) {
                    mem_stall = FALSE;
                    if (EXCPT (ps_ptr) == 0) {
                        // complete load/store, set bypasses
                        process_MEM(ps_ptr, req);
                    } 
                    
                    if (EXCPT (ps_ptr) != 0) { //exception
                        STAGE (ps_ptr) = WB;
                        alu[WB] = ps_ptr;
                        alu[MEM] = NULL;
                        pipe_dealloc(MEM, alu);
                        END_OF_CYCLE;
                    } else { // OK, advance pipe
                        STAGE (ps_ptr) = WB;
                        alu[WB] = ps_ptr;
                        alu[MEM] = NULL;
                    }
                    delete req;
                } else {
                    // still waiting
                    mem_stall = TRUE;
                }
            } else if (STAGE(ps_ptr) == MEM) {
                // nothing to do, forward on bypass registers, pass
                // stage on                
                assert(ps_ptr->issued_to_cache == 0);
                assert(!IS_MEM_OP(OPCODE(ps_ptr->inst)));
                // non mem insts set MEM bypass to EX bypass
                set_mem_bypass(EX_bp_reg, EX_bp_val);
                STAGE (ps_ptr) = WB;
                alu[WB] = ps_ptr;
                alu[MEM] = NULL;
            } else {
                fatal_error("mem stage not set correctly\n");
            }
            // check for faults on the mem_bypass which may have been
            // set above.
            faultChecker.checkAndInject_MEM_BP_FAULT(MEM_bp_val);
        }
    }

    /* EX Stage */
    if (alu[MEM] == NULL) {
        ps_ptr = alu[EX];
        if ((ps_ptr != NULL) && (! mem_stall)) {

            if (EXCPT (ps_ptr) == 0)
                process_EX(ps_ptr, &(MDU));

            STAGE (ps_ptr) = MEM;
            alu[MEM] = ps_ptr;
            alu[EX] = NULL;

            if (EXCPT (ps_ptr) != 0) {
                pipe_dealloc(EX, alu);
                END_OF_CYCLE;
            }
        }
    }

    /* ID Stage */
    if (alu[EX] == NULL) {
        ps_ptr = alu[ID];
        if ((ps_ptr != NULL) && (STAGE(ps_ptr) == ID) && !mem_stall) {
            id_stall = FALSE;

            DSLOT(ps_ptr) = bd_slot;
            if (EXCPT (ps_ptr) == 0) {
                process_ID (ps_ptr, &id_stall, MDU.count);
                if (!id_stall)
                    bd_slot = ((DSLOT(ps_ptr) == TRUE) ? FALSE :
                               (IS_BRANCH (OPCODE(ps_ptr->inst)) ? TRUE : FALSE));
            }
            
            if (!id_stall) {
                STAGE (ps_ptr) = EX;
                /* If it's a floating point op move it to the floating point
                 * pipeline */
                if ((EXCPT(ps_ptr) == 0) && (is_fp_op(OPCODE (ps_ptr->inst))))
                    fatal_error("Floating point not supported");
                else
                    alu[EX] = ps_ptr;
                alu[ID] = NULL;
            }

            if (EXCPT (ps_ptr) != 0) {
                pipe_dealloc(ID, alu);
                END_OF_CYCLE;
            }
        }
    }

    if (alu[ID] == NULL) {
        ps_ptr = alu[IF];
        if ((ps_ptr != NULL) && (STAGE (ps_ptr) == IF_STALL)) {
            /* IF STALL stage */
            if (RNUM (ps_ptr) == bus_req) {
                if (mem_stall || id_stall) {
                    STAGE(ps_ptr) = IF;
                    END_OF_CYCLE;
                } else {
                    STAGE (ps_ptr) = ID;
                    alu[ID] = ps_ptr;
                    alu[IF] = NULL;
                    PC = (nPC.getData() ? nPC : PC + BYTES_PER_WORD);
                }
            } else {
                END_OF_CYCLE;
            }
        } else if (ps_ptr != NULL && (STAGE(ps_ptr) == IF))  {
            /* IF Stage */
            if (STAGE_PC(ps_ptr).getData() == last_text_addr) {
                // we are done with the program
                return 2;
            }

            faultChecker.checkAndInject_INST_ADDR_FAULT(STAGE_PC(ps_ptr));
            CL_READ_MEM_INST(ps_ptr->inst, STAGE_PC(ps_ptr),
                             PADDR(ps_ptr), EXCPT(ps_ptr));
            faultChecker.checkAndInject_INST_TYPE_FAULT(ps_ptr);
            //printf("IF fetch %x\n", STAGE_PC(ps_ptr).getData());

            /* if exception, must be tlb, read instruction for viewing purposes */
            if (EXCPT(ps_ptr) != 0) {
                if (! (mem_stall || id_stall)) {
                    alu[ID] = ps_ptr;
                    alu[IF] = NULL;
                    STAGE (ps_ptr) = ID;
                    PC = (nPC.getData() ? nPC : PC + BYTES_PER_WORD);
                }
                END_OF_CYCLE;
            } else if (cmiss == CACHE_MISS) {
                STAGE (ps_ptr) = IF_STALL;
                END_OF_CYCLE;
            } else if (mem_stall || id_stall) {
                END_OF_CYCLE;
            } else {
                alu[ID] = ps_ptr;
                alu[IF] = NULL;
                STAGE (ps_ptr) = ID;
                PC = (nPC.getData() ? nPC : PC + BYTES_PER_WORD);
            }
        }
    }

  
    /* if we got this far, get next instruction into IF stage */
    alu[IF] = stage_alloc();
    STAGE (alu[IF]) = IF;
    STAGE_PC (alu[IF]) = PC;
    /* do a dummy read just so we can see the instruction in the pipeline */
#warning check for faulted read?
    READ_MEM_INST (alu[IF]->inst, PC.getData());

    END_OF_CYCLE;

  } while (0);
  return 0;
}

int MIPS4KC::can_issue (short int oc)
{
  /* Can't issue anything if the adder is busy or about to be busy */
  if ((FP_add_cnt > 0) || (FP_mul_cnt == 1) || ((FP_div_cnt <= 3) && (FP_div_cnt > 0)))
    return(0);

  switch (oc)
    {
    case Y_ADD_S_OP:
    case Y_ADD_D_OP:
    case Y_CVT_S_D_OP:
    case Y_CVT_W_D_OP:
    case Y_CVT_W_S_OP:
    case Y_SUB_S_OP:
    case Y_SUB_D_OP:
      /* Need adder for 2 cycles.  First one is already known
	 to be free.  As long as mul_cnt > 2 or = 0, and div_cnt > 4 or = 0
	 ok */
      return (((FP_mul_cnt == 0) || (FP_mul_cnt > 2)) &&
	      ((FP_div_cnt == 0) || (FP_div_cnt > 4)));
      break;

    case Y_CVT_D_W_OP:
    case Y_CVT_S_W_OP:
      /* Need adder for 3 cycles.  First one is already known
	 to be free.  As long as mul_cnt > 3, and div_cnt > 5
	 ok */
      return (((FP_mul_cnt == 0) || (FP_mul_cnt > 2)) &&
	      ((FP_div_cnt == 0) || (FP_div_cnt > 4)));

    case Y_DIV_S_OP:
    case Y_DIV_D_OP:
      /* Need adder for 3 cycles in > 8 cycles.  If there is
 	 an outstanding multiply it must be finished before then. */
      return (FP_div_cnt == 0); break;

    case Y_MUL_S_OP:
      /* Need adder for 1 cycle in 2 cycles.  */
      return ((FP_mul_cnt == 0) &&
	      ((FP_div_cnt > 2) || (FP_div_cnt == 0)));

      case Y_MUL_D_OP:
      /* Need adder for 1 cycle in 3 cycles */
      return ((FP_mul_cnt == 0) && ((FP_div_cnt > 3) || (FP_div_cnt == 0)));
      break;

    default:
      return (1);
      break;
    }
}


#define stall2(op, r1, r2) 						\
  (!(can_issue((op)) && is_present((r1)) && is_present ((r2))))

#define stall3(op, r1, r2, r3) 						\
  (!(can_issue((op)) && is_present((r1)) && is_present ((r2)) 		\
     && is_present((r3))))




/* simulate ID stage */
/* must process WB stage before this one to make sure the right values
   are available */

/* Branches in the delay slot are treated as Nops.  */
int MIPS4KC::process_ID (PIPE_STAGE ps, int *stall, int mult_div_busy)
{
  instruction *inst;
  reg_word tmp_PC;
  int excpt = -1;
#warning should check for PC faulty assignments here and report
  inst = ps->inst;
  tmp_PC = PC + BYTES_PER_WORD;

  if (inst == 0) {
      out.output(CALL_INFO, "invalid instruction (pc %x) @ %lld\n",
                 PC.getData(),
                 reg_word::getNow());
      fflush(NULL);
  }

  switch (OPCODE (inst))
    {
    case Y_ADD_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_ADDI_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = IMM (inst);
      break;

    case Y_ADDIU_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = IMM (inst);
      break;

    case Y_ADDU_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_AND_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_ANDI_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = IMM (inst);
      break;

    case Y_BC0F_OP:
    case Y_BC2F_OP:
    case Y_BC3F_OP:
        if (!DSLOT(ps)) {
          /* arg to COP_Available divided by two to find which coprocessor, */
          /* see opcode table in y.tab.c */
          if (!COP_Available( (OPCODE (inst) - Y_BC0F_OP)/2 )) {
              CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE(inst)-Y_BC0F_OP)/2, EXCPT (ps))
                  } else if (CpCond[(OPCODE (inst) - Y_BC0F_OP)/2] == 0) {
              tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
          }
        }
      break;

    case Y_BC1F_OP:
        if (!DSLOT(ps)) {
          if (!COP_Available( (OPCODE (inst) - Y_BC0F_OP)/2 )) {
              CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE(inst)-Y_BC0F_OP)/2, EXCPT(ps))
          } else if (FpCond == 0) {
              tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
          }
        }
      break;


    case Y_BC0T_OP:
    case Y_BC2T_OP:
    case Y_BC3T_OP:
      if (!DSLOT(ps)) {
	if (!COP_Available( (OPCODE (inst) - Y_BC0T_OP)/2 ))
	  CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE(inst)-Y_BC0T_OP)/2, EXCPT(ps))
	else if (CpCond[(OPCODE (inst) - Y_BC0T_OP)/2] != 0)
	  tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
      }
      break;

    case Y_BC1T_OP:
      if (!DSLOT(ps)) {
	if (!COP_Available( (OPCODE (inst) - Y_BC0T_OP)/2 ))
	  CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE(inst)-Y_BC0T_OP)/2, EXCPT(ps))
	else if (FpCond != 0)
	  tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
      }
      break;

    case Y_BEQ_OP:
      if (!DSLOT(ps) &&  (read_R_reg(RS (inst)) == read_R_reg(RT (inst))))
	  tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
      break;

    case Y_BGEZ_OP:
      if (!DSLOT(ps) && (SIGN_BIT (read_R_reg(RS (inst))) == 0))
	tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
      break;

    case Y_BGEZAL_OP:
      if (!DSLOT(ps) && (SIGN_BIT (read_R_reg(RS (inst))) == 0))
	tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
      break;

    case Y_BGTZ_OP:
      if ((!DSLOT(ps)) &&
	  (read_R_reg(RS (inst)) != 0 && SIGN_BIT (read_R_reg(RS (inst))) == 0))
	tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
      break;

    case Y_BLEZ_OP:
      if ((!DSLOT(ps)) &&
	  (read_R_reg(RS (inst)) == 0 || SIGN_BIT (read_R_reg(RS (inst))) != 0))
	tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
      break;

    case Y_BLTZ_OP:
      if ((!DSLOT(ps)) &&
	  (SIGN_BIT (read_R_reg(RS (inst))) != 0))
	tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
      break;

    case Y_BLTZAL_OP:
      if ((!DSLOT(ps)) &&
	  (SIGN_BIT (read_R_reg(RS (inst))) != 0))
	tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
      break;

    case Y_BNE_OP:
        {
            if ((!DSLOT(ps)) &&
                (read_R_reg(RS (inst)) != read_R_reg(RT (inst))))
                tmp_PC = PC + (SIGN_EX (IOFFSET (inst)) << 2);
        }
      break;

    case Y_BREAK_OP:
      CL_RAISE_EXCEPTION(BKPT_EXCPT, 0, EXCPT(ps));
      break;

    case Y_CFC0_OP:
    case Y_CFC2_OP:
    case Y_CFC3_OP:
      if (!COP_Available(OPCODE (inst) - Y_CFC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_CFC0_OP) , EXCPT(ps));
      Operand1 (ps) = read_CP_reg((OPCODE (inst) - Y_CFC0_OP), RD (inst), 0);
      break;

    case Y_COP0_OP:
    case Y_COP1_OP:
    case Y_COP2_OP:
    case Y_COP3_OP:
      if (!COP_Available(OPCODE (inst) - Y_COP0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_COP0_OP) , EXCPT(ps));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_CTC0_OP:
    case Y_CTC2_OP:
    case Y_CTC3_OP:
      if (!COP_Available(OPCODE (inst) - Y_CTC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_CTC0_OP) , EXCPT(ps));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_DIV_OP:
    case Y_DIVU_OP:
      if (mult_div_busy)
	*stall = 1;
      else {
	Operand1 (ps) = read_R_reg(RS (inst));
	Operand2 (ps) = read_R_reg(RT (inst));
      }
      break;

    case Y_J_OP:
      if (!DSLOT (ps))
	tmp_PC = ((PC & 0xf0000000) | (TARGET (inst) & 0x03ffffff) << 2);
      break;

    case Y_JAL_OP:
      if (!DSLOT (ps))
	tmp_PC = ((PC & 0xf0000000) | ((TARGET (inst) & 0x03ffffff) << 2));
      break;

    case Y_JALR_OP:
    case Y_JR_OP:
        if (!DSLOT (ps)) {
          tmp_PC = read_R_reg(RS (inst)).getData();
        }
      break;

    case Y_LB_OP:
    case Y_LBU_OP:
    case Y_LH_OP:
    case Y_LHU_OP:
    case Y_LW_OP:
    case Y_LWL_OP:
    case Y_LWR_OP:
      Operand1 (ps) = read_R_reg(BASE (inst));
      Operand2 (ps) = IOFFSET (inst);
      break;

    case Y_LWC0_OP:
    case Y_LWC2_OP:
    case Y_LWC3_OP:
      if (!COP_Available(OPCODE (inst) - Y_LWC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_LWC0_OP) , EXCPT(ps));
      Operand1 (ps) = read_R_reg(BASE (inst));
      Operand2 (ps) = IOFFSET (inst);
      break;

    case Y_LUI_OP:
      Operand2 (ps) = IMM (inst);
      break;

    case Y_MFC0_OP:
    case Y_MFC2_OP:
    case Y_MFC3_OP:
      if (!COP_Available(OPCODE (inst) - Y_MFC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_MFC0_OP), EXCPT(ps));
      Operand1 (ps) = read_CP_reg((OPCODE (inst) - Y_MFC0_OP), RD (inst), 1);
      break;

    case Y_MFHI_OP:
      if (HI_present)
	Operand1 (ps) = HI;
      else
	*stall = 1;
      break;

    case Y_MFLO_OP:
      if (LO_present)
	Operand1 (ps) = LO;
      else
	*stall = 1;
      break;

    case Y_MTC0_OP:
    case Y_MTC2_OP:
    case Y_MTC3_OP:
      if (!COP_Available(OPCODE (inst) - Y_MTC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_MTC0_OP), EXCPT(ps));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_MTC1_OP:
      if (!COP_Available(OPCODE (inst) - Y_MTC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_MTC0_OP), EXCPT(ps));
      Operand2 (ps) = read_R_reg(RT (inst));
      *stall = ! (is_present(RD (inst)));
      clr_single_present(RD (inst));
      break;


    case Y_MTHI_OP:
    case Y_MTLO_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      break;

    case Y_MULT_OP:
    case Y_MULTU_OP:
      if (mult_div_busy)
	*stall = 1;
      else {
	Operand1 (ps) = read_R_reg(RS (inst));
	Operand2 (ps) = read_R_reg(RT (inst));
      }
      break;

    case Y_NOR_OP:
    case Y_OR_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_ORI_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = IMM (inst);
      break;


    case Y_RFE_OP:
      if (!COP_Available(0))
	CL_RAISE_EXCEPTION(CPU_EXCPT, 0x0 , EXCPT(ps));
      Operand1 (ps) = (Status_Reg & 0xfffffff0);
      Operand2 (ps) = ((Status_Reg & 0x3c) >> 2);
      break;

    case Y_SB_OP:
    case Y_SH_OP:
      Operand1 (ps) = read_R_reg(RT (inst));
      Operand2 (ps) = read_R_reg(BASE (inst));
      Operand3 (ps) = IOFFSET (inst);
      break;

    case Y_SLL_OP:
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_SLLV_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) =  0x1f;

    case Y_SLT_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_SLTI_OP:
    case Y_SLTIU_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = IMM (inst);
      break;


    case Y_SLTU_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_SRA_OP:
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_SRAV_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_SRL_OP:
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_SRLV_OP:
    case Y_SUB_OP:
    case Y_SUBU_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_SW_OP:
    case Y_SWL_OP:
    case Y_SWR_OP:
      Operand1 (ps) = read_R_reg(RT (inst));
      Operand2 (ps) = read_R_reg(BASE (inst));
      Operand3 (ps) = IOFFSET (inst);
      break;

    case Y_SWC1_OP:
      if (!COP_Available(OPCODE (inst) - Y_SWC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_SWC0_OP), EXCPT(ps));
      Operand2 (ps) = read_R_reg(BASE (inst));
      Operand3 (ps) = IOFFSET (inst);

      *stall = !is_single_present(RT (inst));
      break;

    case Y_SWC0_OP:
    case Y_SWC3_OP:
      if (!COP_Available(OPCODE (inst) - Y_SWC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_SWC0_OP), EXCPT(ps));
      Operand1 (ps) = read_CP_reg((OPCODE (inst) - Y_SWC0_OP), RT (inst), 1);
      Operand2 (ps) = read_R_reg(BASE (inst));
      Operand3 (ps) = IOFFSET (inst);
      break;

    case Y_SWC2_OP:
      /* do nothing, SWC2 is being used to print memory stats */
      break;

    case Y_SYSCALL_OP:
      CL_RAISE_EXCEPTION(SYSCALL_EXCPT, 0, EXCPT(ps));
      break;

    case Y_TLBP_OP:
    case Y_TLBR_OP:
    case Y_TLBWI_OP:
    case Y_TLBWR_OP:
      if (!COP_Available(0))
	CL_RAISE_EXCEPTION(CPU_EXCPT, 0, EXCPT(ps));
      break;

    case Y_XOR_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = read_R_reg(RT (inst));
      break;

    case Y_XORI_OP:
      Operand1 (ps) = read_R_reg(RS (inst));
      Operand2 (ps) = IMM (inst);
      break;

    /* Floating point operations */
    case Y_ABS_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;


    case Y_ABS_D_OP:
      FPoperand1 (ps) = FPR_D (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;

    case Y_ADD_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      FPoperand2 (ps) = FPR_S (FT (inst));
      *stall = stall3(OPCODE (inst), FS (inst), FT (inst), FD (inst));
      break;

    case Y_ADD_D_OP:
      FPoperand1 (ps) = FPR_D (FS (inst));
      FPoperand2 (ps) = FPR_D (FT (inst));
      *stall = stall3(OPCODE (inst), FS (inst), FT(inst), FD (inst));
      break;


    case Y_C_F_S_OP:
    case Y_C_UN_S_OP:
    case Y_C_EQ_S_OP:
    case Y_C_UEQ_S_OP:
    case Y_C_OLE_S_OP:
    case Y_C_ULE_S_OP:
    case Y_C_SF_S_OP:
    case Y_C_NGLE_S_OP:
    case Y_C_SEQ_S_OP:
    case Y_C_NGL_S_OP:
    case Y_C_LT_S_OP:
    case Y_C_NGE_S_OP:
    case Y_C_LE_S_OP:
    case Y_C_NGT_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      FPoperand2 (ps) = FPR_S (FT (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FT (inst));
      break;


    case Y_C_F_D_OP:
    case Y_C_UN_D_OP:
    case Y_C_EQ_D_OP:
    case Y_C_UEQ_D_OP:
    case Y_C_OLE_D_OP:
    case Y_C_ULE_D_OP:
    case Y_C_SF_D_OP:
    case Y_C_NGLE_D_OP:
    case Y_C_SEQ_D_OP:
    case Y_C_NGL_D_OP:
    case Y_C_LT_D_OP:
    case Y_C_NGE_D_OP:
    case Y_C_LE_D_OP:
    case Y_C_NGT_D_OP:
      FPoperand1 (ps) = FPR_D (FS (inst));
      FPoperand2 (ps) = FPR_D (FT (inst));
      *stall = stall3(OPCODE (inst), FS (inst), FT (inst), FD (inst));
      break;

    case Y_CFC1_OP:
      /* What to do */
      break;

    case Y_CTC1_OP:
      /* What to do */
      break;

    case Y_CVT_D_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;

    case Y_CVT_D_W_OP:
      FPoperand1 (ps) = FPR_W (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;

    case Y_CVT_S_D_OP:
      FPoperand1 (ps) = FPR_D (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;


    case Y_CVT_S_W_OP:
      FPoperand1 (ps) = FPR_W (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;

    case Y_CVT_W_D_OP:
      FPoperand1 (ps) = FPR_D (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;


    case Y_CVT_W_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;

    case Y_DIV_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      FPoperand2 (ps) = FPR_S (FT (inst));
      *stall = stall3(OPCODE (inst), FS (inst), FT (inst), FD (inst));
      break;

    case Y_DIV_D_OP:
      FPoperand1 (ps) = FPR_D (FS (inst));
      FPoperand2 (ps) = FPR_D (FT (inst));
      *stall = stall3(OPCODE (inst), FS (inst), FT (inst), FD (inst));
      break;


    case Y_LWC1_OP:
      if (!COP_Available(OPCODE (inst) - Y_LWC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_LWC0_OP) , EXCPT(ps));
      Operand1 (ps) = read_R_reg(BASE (inst));
      Operand2 (ps) = IOFFSET (inst);
      /* can't issue the load if some instruction in the floating
	 point-pipeline is going to write to the dest */
      *stall = ! is_single_present(FT (inst));
      break;

    case Y_MFC1_OP:
      if (!COP_Available(OPCODE (inst) - Y_MFC0_OP))
	CL_RAISE_EXCEPTION(CPU_EXCPT, (OPCODE (inst) - Y_MFC0_OP), EXCPT(ps));
      *stall = ! (is_single_present (RD (inst)));
      break;

    case Y_MOV_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;


    case Y_MOV_D_OP:
      FPoperand1 (ps) = FPR_D (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
       break;

    case Y_MUL_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      FPoperand2 (ps) = FPR_S (FT (inst));
      *stall = stall3(OPCODE (inst), FS (inst), FT (inst), FD (inst));
      break;


    case Y_MUL_D_OP:
      FPoperand1 (ps) = FPR_D (FS (inst));
      FPoperand2 (ps) = FPR_D (FT (inst));
      *stall = stall3(OPCODE (inst), FS (inst), FT (inst), FD (inst));
      break;

    case Y_NEG_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;


    case Y_NEG_D_OP:
      FPoperand1 (ps) = FPR_D (FS (inst));
      *stall = stall2(OPCODE (inst), FS (inst), FD (inst));
      break;


    case Y_SUB_S_OP:
      FPoperand1 (ps) = FPR_S (FS (inst));
      FPoperand2 (ps) = FPR_S (FT (inst));
      *stall = stall3(OPCODE (inst), FS (inst), FT (inst), FD (inst));
      break;


    case Y_SUB_D_OP:
      FPoperand1 (ps)  = FPR_D (FS (inst));
      FPoperand2 (ps) = FPR_D (FT (inst));
      *stall = stall3(OPCODE (inst), FS (inst), FT (inst), FD (inst));
      break;


    default:
      excpt = RI_EXCPT << 2;
      *stall = 0;
      break;
    }

  if (*stall == 0) {
    nPC = tmp_PC;
  }

  return (excpt);

}




/* Simulate EX stage */
/* integer unit only */
/* where do bypass values get set? */
/* Goal of EX to to compute value or address and to set the
   necessary bypass values. */

void MIPS4KC::process_EX (PIPE_STAGE ps, struct mult_div_unit *pMDU)
{
  instruction *inst;

  inst = ps->inst;

  switch (OPCODE (inst))
    {
    case Y_ADD_OP:
      {	reg_word vs, vt;
	reg_word sum;

	vs = Operand1 (ps);
	vt = Operand2 (ps);
	sum = vs + vt;
        faultChecker.checkAndInject_ALU(sum);

	if (ARITH_OVFL (sum, vs, vt))
	  CL_RAISE_EXCEPTION (OVF_EXCPT, 0, EXCPT(ps));

	VALUE (ps) = sum;
	set_ex_bypass(RD (inst), sum);
	break;
      }

    case Y_ADDI_OP:
        { reg_word vs, imm;
          reg_word sum;

	vs = Operand1 (ps);
	imm = Operand2(ps).truncExtend(); 
	sum = vs + imm;
        faultChecker.checkAndInject_ALU(sum);

	if (ARITH_OVFL (sum, vs, imm))
	  CL_RAISE_EXCEPTION (OVF_EXCPT, 0, EXCPT(ps));
	VALUE (ps) = sum;
	set_ex_bypass(RT (inst), sum);
	break;
      }

    case Y_ADDIU_OP:
        VALUE (ps) = Operand1 (ps) + (Operand2(ps).truncExtend());
        faultChecker.checkAndInject_ALU(VALUE(ps));
        set_ex_bypass(RT (inst), VALUE (ps));
        break;

    case Y_ADDU_OP:
      VALUE (ps) = Operand1 (ps) + Operand2 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RD (inst), VALUE (ps));
      break;

    case Y_AND_OP:
      VALUE (ps) = Operand1 (ps) & Operand2 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RD (inst), VALUE (ps));
      break;

    case Y_ANDI_OP:
      VALUE (ps) = Operand1 (ps) & (Operand2 (ps) & 0xffff);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RT (inst), VALUE (ps));
      break;

    case Y_BC0F_OP:
    case Y_BC1F_OP:
    case Y_BC2F_OP:
    case Y_BC3F_OP:
    case Y_BC0T_OP:
    case Y_BC1T_OP:
    case Y_BC2T_OP:
    case Y_BC3T_OP:
    case Y_BEQ_OP:
    case Y_BGEZ_OP:
      set_ex_bypass(0, 0);
      break;

    case Y_BGEZAL_OP:
      VALUE (ps) = STAGE_PC(ps) + 2 * BYTES_PER_WORD;
      /* where is the link value computed and stored?  should it be
	 available through bypass?  */
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(31, VALUE (ps));
      break;

    case Y_BGTZ_OP:
    case Y_BLEZ_OP:
    case Y_BLTZ_OP:
      set_ex_bypass(0, 0);
      break;

    case Y_BLTZAL_OP:
      VALUE (ps) = STAGE_PC(ps) + 2 * BYTES_PER_WORD;
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(31, VALUE (ps));
      break;

    case Y_BNE_OP:
      set_ex_bypass(0, 0);
      break;

    case Y_BREAK_OP:
      /* what to do? */
      break;

    case Y_CFC0_OP:
    case Y_CFC2_OP:
    case Y_CFC3_OP:
      VALUE (ps) = Operand1 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(0, 0);
      break;

    case Y_COP0_OP:
    case Y_COP1_OP:
    case Y_COP2_OP:
    case Y_COP3_OP:
      VALUE (ps) = Operand2 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      /* bypass? */
      set_ex_bypass(0, 0);
      break;

    case Y_CTC0_OP:
    case Y_CTC2_OP:
    case Y_CTC3_OP:
      VALUE (ps) = Operand2 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      /* bypass? */
      set_ex_bypass(0, 0);
      break;

    case Y_DIV_OP:
      { reg_word hi, lo;

	if (Operand2 (ps) != 0)
	  {
	    hi = Operand1 (ps) % Operand2 (ps);
	    lo = Operand1 (ps) / Operand2 (ps);
	  }
	else
	  {
	    hi = HI;
	    lo = LO;
	  }

	HI_present = 0;
	LO_present = 0;
	pMDU->hi_val = hi;
	pMDU->lo_val = lo;
	/* -1 is for this cycle */
	pMDU->count = DIV_COST - 1;

	set_ex_bypass(0, 0);
	break;
      }

    case Y_DIVU_OP:
      {reg_word hi, lo;

       if (Operand2 (ps) != 0)
	 {
             //hi = (unsigned long) Operand1 (ps) % (unsigned long) Operand2 (ps);
             //lo = (unsigned long) Operand1 (ps) / (unsigned long) Operand2 (ps);
             hi = Operand1(ps).unsigned_mod(Operand2(ps));
             lo = Operand1(ps).unsigned_div(Operand2(ps));
	 }
       else
	 {
	   hi = HI;
	   lo = LO;
	 }

       faultChecker.checkAndInject_MDU(hi,lo);

       HI_present = 0;
       LO_present = 0;
       pMDU->hi_val = hi;
       pMDU->lo_val = lo;
       /* -1 is for this cycle */
       pMDU->count = DIV_COST - 1;
       set_ex_bypass(0, 0);
       break;
     }

    case Y_J_OP:
      set_ex_bypass(0, 0);
      break;

    case Y_JAL_OP:
      VALUE (ps) = STAGE_PC(ps) + 2 * BYTES_PER_WORD;
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(31, VALUE (ps));
      break;

    case Y_JALR_OP:
      VALUE (ps) =  STAGE_PC(ps) + 2 * BYTES_PER_WORD;
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RD (inst), VALUE (ps));
      break;

    case Y_JR_OP:
      set_ex_bypass(0, 0);
      break;

    case Y_LB_OP:
    case Y_LBU_OP:
    case Y_LH_OP:
    case Y_LHU_OP:
    case Y_LW_OP:
    case Y_LWC0_OP:
    case Y_LWC2_OP:
    case Y_LWC3_OP:
    case Y_LWL_OP:
    case Y_LWR_OP:
      ADDR (ps) = read_R_reg(BASE (inst)) + IOFFSET (inst);
      faultChecker.checkAndInject_ALU(ADDR(ps));
      set_ex_bypass(0, 0);
      break;

    case Y_LWC1_OP:
      clr_single_present(FT (inst));
      ADDR (ps) = read_R_reg(BASE (inst)) + IOFFSET (inst);
      faultChecker.checkAndInject_ALU(ADDR(ps));
      set_ex_bypass(0, 0);
      break;

    case Y_LUI_OP:
      VALUE (ps) = (Operand2 (ps) << 16) & 0xffff0000;
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RT (inst), VALUE (ps));
      break;

    case Y_MFC0_OP:
    case Y_MFC2_OP:
    case Y_MFC3_OP:
      VALUE (ps) = Operand1 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(0, 0);
      break;

    case Y_MFC1_OP:
      /* DO nothing...value is read in MEM */
      set_ex_bypass(0, 0);
      break;

    case Y_MFHI_OP:
      VALUE (ps) = Operand1 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RD (inst), VALUE (ps));
      break;

    case Y_MFLO_OP:
      VALUE (ps) = Operand1 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RD (inst), VALUE (ps));
      break;

    case Y_MTC0_OP:
    case Y_MTC2_OP:
    case Y_MTC3_OP:
      VALUE (ps) = Operand2 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(0, 0);
      break;

    case Y_MTC1_OP:
      VALUE (ps) = Operand2 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(0, 0);
      break;

    case Y_MTHI_OP:
      HI = Operand1 (ps);
      faultChecker.checkAndInject_ALU(HI);
      set_ex_bypass(0, 0);
      break;

    case Y_MTLO_OP:
      LO = Operand1 (ps);
      faultChecker.checkAndInject_ALU(LO);
      set_ex_bypass(0, 0);
      break;

    case Y_MULT_OP:
      {
	reg_word v1 = Operand1 (ps), v2 = Operand2 (ps);
	reg_word lo, hi;
	int neg_sign = 0;

	if (v1 < 0)
	  v1 = - v1, neg_sign = 1;
	if (v2 < 0)
	  v2 = - v2, neg_sign = ! neg_sign;


	long_multiply (v1, v2, &hi, &lo);
	if (neg_sign)
	  {
            reg_word carry = 0;

	    lo = ~ lo;
	    hi = ~ hi;
	    carry = lo & 0x80000000;
	    lo += 1;
	    if ((lo ^ carry) == 1)
	      hi += 1;
	  }

        faultChecker.checkAndInject_MDU(hi,lo);

	HI_present = 0;
	LO_present = 0;
	pMDU->hi_val = hi;
	pMDU->lo_val = lo;
	/* -1 is for this cycle */
	pMDU->count = MULT_COST - 1;

	set_ex_bypass(0, 0);
	break;
      }

    case Y_MULTU_OP:
      {reg_word hi, lo;

       long_multiply (Operand1 (ps), Operand2 (ps), &hi, &lo);

       faultChecker.checkAndInject_MDU(hi,lo);

       HI_present = 0;
       LO_present = 0;
       pMDU->hi_val = hi;
       pMDU->lo_val = lo;
       /* -1 is for this cycle */
       pMDU->count = MULT_COST - 1;

       set_ex_bypass(0, 0);
       break;
     }

    case Y_NOR_OP:
      VALUE (ps) = ~ (Operand1 (ps) | Operand2 (ps));
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RD (inst), VALUE (ps));
      break;


    case Y_OR_OP:
      VALUE (ps) = Operand1 (ps) | Operand2 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RD (inst), VALUE (ps));
      break;

    case Y_ORI_OP:
      VALUE (ps) = Operand1 (ps) | (Operand2(ps) & 0xffff);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RT (inst), VALUE (ps));
      break;

    case Y_RFE_OP:
      /* but without kernel code, this should never occur */
      Status_Reg = (Status_Reg & 0xfffffff0) | ((Status_Reg & 0x3c) >> 2);
      faultChecker.checkAndInject_ALU(Status_Reg);
      set_ex_bypass(0, 0);
      break;

    case Y_SB_OP:
    case Y_SH_OP:
      ADDR (ps) = Operand2 (ps) + Operand3 (ps);
      faultChecker.checkAndInject_ALU(ADDR(ps));
      VALUE (ps) = Operand1 (ps);
      set_ex_bypass(0, 0);
      break;

    case Y_SLL_OP:
      {
	int shamt = SHAMT (inst);

	if (shamt >= 0 && shamt < 32)
	  VALUE (ps) = Operand2 (ps) << shamt;
	else
	  VALUE (ps) = Operand2 (ps);

        faultChecker.checkAndInject_ALU(VALUE(ps));
	set_ex_bypass(RD (inst), VALUE (ps));
	break;
      }

    case Y_SLLV_OP:
      {
        const reg_word shamt = (Operand1(ps) & 0x1f);

	if (shamt >= 0 && shamt < 32)
	  VALUE (ps) = Operand2(ps) << shamt;
	else
	  VALUE (ps) = Operand2 (ps);

        faultChecker.checkAndInject_ALU(VALUE(ps));
	set_ex_bypass(RD (inst), VALUE (ps));
	break;
      }

    case Y_SLT_OP:
        // use special function to preserve fault tracking
        VALUE(ps) = setLessThan(Operand1(ps),Operand2(ps));
        faultChecker.checkAndInject_ALU(VALUE(ps));
        set_ex_bypass(RD (inst), VALUE (ps));
        break;

    case Y_SLTI_OP:
        faultChecker.checkAndInject_ALU(VALUE(ps));
        VALUE(ps) = setLessThan(Operand1(ps),(Operand2(ps).truncExtend()));
        
        set_ex_bypass(RT (inst), VALUE (ps));
        break;

    case Y_SLTIU_OP:
      {
        VALUE(ps) = setLessThan(Operand1(ps),(Operand2(ps).truncExtend()), 1);
        faultChecker.checkAndInject_ALU(VALUE(ps));
	set_ex_bypass(RT (inst), VALUE (ps));
	break;
      }

    case Y_SLTU_OP:
      VALUE(ps) = setLessThan(Operand1(ps),Operand2(ps), 1);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RD (inst), VALUE (ps));
      break;

    case Y_SRA_OP:
      {
	int shamt = SHAMT (inst);
	reg_word val = Operand2 (ps);

	if (shamt >= 0 && shamt < 32)
	  VALUE (ps) = val >> shamt;
	else
	  VALUE (ps) = val;

        faultChecker.checkAndInject_ALU(VALUE(ps));
	set_ex_bypass(RD (inst), VALUE (ps));
	break;
      }

    case Y_SRAV_OP:
      {
        const reg_word shamt = Operand1 (ps) & 0x1f;
	reg_word val = Operand2 (ps);

	if (shamt >= 0 && shamt < 32)
	  VALUE (ps) = val >> shamt;
	else
	  VALUE (ps) = val;

        faultChecker.checkAndInject_ALU(VALUE(ps));
	set_ex_bypass(RD (inst), VALUE (ps));
	break;
      }

    case Y_SRL_OP:
        {
            int shamt = SHAMT (inst);
                
            if (shamt >= 0 && shamt < 32) {
                VALUE(ps) = Operand2(ps).shift_right_logical(shamt);
            } else
                VALUE(ps) = Operand2(ps);
            
            faultChecker.checkAndInject_ALU(VALUE(ps));
            set_ex_bypass(RD (inst), VALUE (ps));
            break;
      }

    case Y_SRLV_OP:
      {
          const reg_word shamt = (Operand1(ps) & 0x1f);

	if (shamt >= 0 && shamt < 32)
            VALUE (ps) = Operand2(ps).shift_right_logical(shamt);
	else
            VALUE (ps) = Operand2(ps);

        faultChecker.checkAndInject_ALU(VALUE(ps));
	set_ex_bypass(RD (inst), VALUE (ps));
	break;
      }

    case Y_SUB_OP:
      {
	reg_word vs = Operand1 (ps), vt = Operand2 (ps);
	reg_word diff = vs - vt;

	if (SIGN_BIT (vs) != SIGN_BIT (vt)
	    && SIGN_BIT (vs) != SIGN_BIT (diff))
	  CL_RAISE_EXCEPTION (OVF_EXCPT, 0, EXCPT(ps));

	VALUE (ps) = diff;

        faultChecker.checkAndInject_ALU(VALUE(ps));
	set_ex_bypass(RD (inst), diff);
	break;
      }

    case Y_SUBU_OP:
        //VALUE (ps) = (unsigned long) Operand1 (ps) - (unsigned long) Operand2 (ps);
        VALUE(ps) = Operand1(ps) - Operand2(ps);
        faultChecker.checkAndInject_ALU(VALUE(ps));
        set_ex_bypass(RD (inst), VALUE (ps));
        break;

    case Y_SW_OP:
    case Y_SWC0_OP:
    case Y_SWC1_OP:
    case Y_SWC3_OP:
    case Y_SWL_OP:
    case Y_SWR_OP:
      ADDR (ps) = Operand2 (ps) + Operand3 (ps);
      faultChecker.checkAndInject_ALU(ADDR(ps));
      VALUE (ps) = Operand1 (ps);
      set_ex_bypass(0, 0);
      break;

    case Y_SWC2_OP:
      /* do nothing, SWC2 is being used to print memory stats */
      break;

    case Y_SYSCALL_OP:
      break;

    case Y_TLBP_OP:
    case Y_TLBR_OP:
    case Y_TLBWI_OP:
    case Y_TLBWR_OP:
      /* what to do? */
      break;

    case Y_XOR_OP:
      VALUE (ps) = Operand1 (ps) ^ Operand2 (ps);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RD (inst), VALUE (ps));
      break;

    case Y_XORI_OP:
      /* why is the 0xffff necessary */
      VALUE (ps) = Operand1(ps) ^ (Operand2(ps) & 0xffff);
      faultChecker.checkAndInject_ALU(VALUE(ps));
      set_ex_bypass(RT (inst), VALUE (ps));
      break;


    default:
      /* What to do? */
      break;
    }

}


MIPS4KC::memReq *MIPS4KC::check_cacheComplete(PIPE_STAGE ps) {
    std::map<PIPE_STAGE, memReq *>::iterator i = requestsIn.find(ps);
    memReq *req = 0;
    if (i != requestsIn.end()) { // found it
        req = i->second;
        requestsIn.erase(i);
    }
    return req;
}

/* process memory stage */
/* need to handle misses and faults */

void MIPS4KC::process_MEM (PIPE_STAGE ps, memReq *req)
{
    assert(req);
    instruction *inst;
    int cmiss;

  inst = ps->inst;

  if (outputLevel > 0) {
      printf("process_MEM ");  print_inst(ps->pc.getData()); printf("\n");
  }

  /* only load instructions update the MEM bypass variables
     to something other than the values generated by the EX stage */
  set_mem_bypass(EX_bp_reg, EX_bp_val);
  /* Kill old CP bypass values */
  set_CP_bypass(-1, -1 , -1, -1);

  switch (OPCODE (inst))
    {
    case Y_LB_OP:
        CL_READ_MEM_BYTE(VALUE(ps), ADDR (ps), PADDR (ps), EXCPT (ps), req);
      if (EXCPT (ps) == 0)
	set_mem_bypass(RT (inst), VALUE (ps));
      break;

    case Y_LBU_OP:
      CL_READ_MEM_BYTE( VALUE(ps), ADDR (ps), PADDR (ps), EXCPT (ps), req);
      if (EXCPT (ps) == 0) {
	/* 0xff is probably not necessary */
	VALUE (ps) &= 0xff;
	set_mem_bypass(RT (inst), VALUE (ps));
      }
      break;

    case Y_LH_OP:
      CL_READ_MEM_HALF( VALUE(ps), ADDR (ps), PADDR (ps), EXCPT (ps), req);
      if (EXCPT (ps) == 0)
	set_mem_bypass(RT (inst), VALUE (ps));
      break;

    case Y_LHU_OP:
      CL_READ_MEM_HALF( VALUE(ps), ADDR (ps), PADDR (ps), EXCPT (ps), req);
      if (EXCPT (ps) == 0) {
	/* 0xffff is probably not necessary */
	VALUE (ps) &= 0xffff;
	set_mem_bypass(RT (inst), VALUE (ps));
      }
      break;

    case Y_LW_OP:
      CL_READ_MEM_WORD( VALUE(ps), ADDR (ps), PADDR (ps), EXCPT (ps), req);
      if (EXCPT (ps) == 0)
	set_mem_bypass(RT (inst), VALUE (ps));
      break;

    case Y_LWC0_OP:
    case Y_LWC2_OP:
    case Y_LWC3_OP:
      CL_READ_MEM_WORD( VALUE(ps), ADDR (ps), PADDR (ps), EXCPT (ps), req);
      if (EXCPT (ps) == 0)
	set_CP_bypass((OPCODE (inst) - Y_LWC0_OP), RT (inst), VALUE (ps), 1)
      else
	/* Exception occurred.  Need to reset the register
	 * presence bits, so that after the fault, the instruction
	 * can restart */
	set_single_present(FT (inst));
      break;


    case Y_LWC1_OP:
      CL_READ_MEM_WORD( VALUE(ps), ADDR (ps), PADDR (ps), EXCPT (ps), req);
      if (EXCPT (ps) == 0)
	set_CP_bypass((OPCODE (inst) - Y_LWC0_OP), RT (inst), VALUE (ps), 1)
      else
	/* Exception occurred.  Need to reset the register
	 * presence bits, so that after the fault, the instruction
	 * can restart */
	set_single_present(FT (inst));
      break;

    case Y_LWL_OP:
      {
        reg_word addr = ADDR (ps);
	reg_word word;	/* Can't be register */
	int byte = addr.getData() & 0x3;
	/* is this right? -- NO need bypass value from memory stage of previous
	   instruction. */
	reg_word reg_val = R[RT (inst)];

	CL_READ_MEM_WORD( word, addr & 0xfffffffc, PADDR (ps), 
			 EXCPT (ps), req);
	/* Fix this */
/*	if ((Cause >> 2) > LAST_REAL_EXCEPT) */
	if (Cause == 0)
#ifdef BIGENDIAN
	  switch (byte)
	    {
	    case 0:
	      VALUE (ps) = word;
	      break;

	    case 1:
	      VALUE (ps) = (word & 0xffffff) << 8 | (reg_val & 0xff);
	      break;

	    case 2:
	      VALUE (ps) = (word & 0xffff) << 16 | (reg_val & 0xffff);
	      break;

	    case 3:
	      VALUE (ps) = (word & 0xff) << 24 | (reg_val & 0xffffff);
	      break;
	      }
#else
	switch (byte)
	  {
	  case 0:
	    VALUE (ps) = (word & 0xff) << 24 | (reg_val & 0xffffff);
	    break;

	  case 1:
	    VALUE (ps) = (word & 0xffff) << 16 | (reg_val & 0xffff);
	    break;

	  case 2:
	    VALUE (ps) = (word & 0xffffff) << 8 | (reg_val & 0xff);
	    break;

	  case 3:
	    VALUE (ps) = word;
	    break;
	  }
#endif
	set_mem_bypass(RT (inst), VALUE (ps));

	break;
      }

    case Y_LWR_OP:
      {
	reg_word addr = ADDR (ps);
	reg_word word;	/* Can't be register */
	int byte = addr.getData() & 0x3;
	/* is this right? */
	reg_word reg_val = R[RT (inst)];

        CL_READ_MEM_WORD( word, addr & 0xfffffffc, PADDR (ps), 
			 EXCPT (ps), req);
	/* fix this */

/*	if ((Cause >> 2) > LAST_REAL_EXCEPT) */
	if (Cause == 0)
#ifdef BIGENDIAN
	  switch (byte)
	    {
	    case 0:
	      VALUE (ps) = (reg_val & 0xffffff00)
		| ((word & 0xff000000) >> 24);
	      break;

	    case 1:
	      VALUE (ps) = (reg_val & 0xffff0000)
		| ((word & 0xffff0000) >> 16);
	      break;

	    case 2:
	      VALUE (ps) = (reg_val & 0xff000000)
		| ((word & 0xffffff00) >> 8);
	      break;

	    case 3:
	      VALUE (ps) = word;
	      break;
	    }
#else
	switch (byte)
	  {
	    /* NB: The description of the little-endian case in Kane is
	       totally wrong. */
	  case 0:		/* 3 in book */
	    VALUE (ps) = reg_val;
	    break;

	  case 1:		/* 0 in book */
	    VALUE (ps) = (reg_val & 0xff000000)
	      | ((word & 0xffffff00) >> 8);
	    break;

	  case 2:		/* 1 in book */
	    VALUE (ps) = (reg_val & 0xffff0000)
	      | ((word & 0xffff0000) >> 16);
	    break;

	  case 3:		/* 2 in book */
	    VALUE (ps) = (reg_val & 0xffffff00)
	      | ((word & 0xff000000) >> 24);
	    break;
	  }
#endif
	set_mem_bypass(RT (inst), VALUE (ps));
	break;
      }


    case Y_SB_OP:
        CL_SET_MEM_BYTE( ADDR (ps), PADDR (ps), VALUE (ps), EXCPT(ps), req);
      break;

    case Y_SH_OP:
        CL_SET_MEM_HALF( ADDR (ps), PADDR (ps), VALUE (ps), EXCPT(ps), req);
      break;

    case Y_SW_OP:
        CL_SET_MEM_WORD( ADDR (ps), PADDR (ps), VALUE (ps), EXCPT(ps), req);
      break;

    case Y_SWC1_OP:
      {
	float val = FGR [RT (inst)];
	reg_word *vp = (reg_word *) &val;

	CL_SET_MEM_WORD( ADDR (ps), PADDR (ps), *vp, EXCPT (ps), req);

	break;
      }

    case Y_SWC0_OP:
    case Y_SWC3_OP:
        CL_SET_MEM_WORD( ADDR (ps), PADDR (ps), VALUE (ps), EXCPT(ps), req);
      break;


    case Y_SWC2_OP:
      /* do nothing, SWC2 is being used to print memory stats */
      break;

    case Y_SWL_OP:
      {
        reg_word addr = ADDR (ps);
	reg_word data;
	reg_word reg = R[RT (inst)];
	int byte = addr.getData() & 0x3;

	CL_READ_MEM_WORD ( data, addr & 0xfffffffc, PADDR (ps), 
			  EXCPT (ps), req);

#ifdef BIGENDIAN
	switch (byte)
	  {
	  case 0:
	    data = reg;
	    break;

	  case 1:
	    data = (data & 0xff000000) | (reg >> 8 & 0xffffff);
	    break;

	  case 2:
	    data = (data & 0xffff0000) | (reg >> 16 & 0xffff);
	    break;

	  case 3:
	    data = (data & 0xffffff00) | (reg >> 24 & 0xff);
	    break;
	  }
#else
	switch (byte)
	  {
	  case 0:
	    data = (data & 0xffffff00) | (reg >> 24 & 0xff);
	    break;

	  case 1:
	    data = (data & 0xffff0000) | (reg >> 16 & 0xffff);
	    break;

	  case 2:
	    data = (data & 0xff000000) | (reg >> 8 & 0xffffff);
	    break;

	  case 3:
	    data = reg;
	    break;
	  }
#endif
	CL_SET_MEM_WORD ( addr & 0xfffffffc, PADDR (ps), data, 
                          EXCPT(ps), req);
	break;
      }

    case Y_SWR_OP:
      {
        reg_word addr = R[BASE (inst)] + IOFFSET (inst);
	reg_word data;
	reg_word reg = R[RT (inst)];
	int byte = addr.getData() & 0x3;

	CL_READ_MEM_WORD ( data, addr & 0xfffffffc, PADDR (ps), 
                           EXCPT (ps), req);

#ifdef BIGENDIAN
	switch (byte)
	  {
	  case 0:
	    data = ((reg << 24) & 0xff000000) | (data & 0xffffff);
	    break;

	  case 1:
	    data = ((reg << 16) & 0xffff0000) | (data & 0xffff);
	    break;

	  case 2:
	    data = ((reg << 8) & 0xffffff00) | (data & 0xff) ;
	    break;

	  case 3:
	    data = reg;
	    break;
	  }
#else
	switch (byte)
	  {
	  case 0:
	    data = reg;
	    break;

	  case 1:
	    data = ((reg << 8) & 0xffffff00) | (data & 0xff) ;
	    break;

	  case 2:
	    data = ((reg << 16) & 0xffff0000) | (data & 0xffff);
	    break;

	  case 3:
	    data = ((reg << 24) & 0xff000000) | (data & 0xffffff);
	    break;
	  }
#endif
	CL_SET_MEM_WORD ( addr & 0xfffffffc, PADDR (ps), data, 
                          EXCPT(ps), req);
	break;
      }

    case Y_SYSCALL_OP:
      break;

    case Y_TLBP_OP:
      tlbp();
      break;

    case Y_TLBR_OP:
      tlbr();
      break;

    case Y_TLBWI_OP:
      tlbwi();
      break;

    case Y_TLBWR_OP:
      tlbwr();
      break;

    case Y_CFC0_OP:
    case Y_CFC2_OP:
    case Y_CFC3_OP:
    case Y_MFC0_OP:
    case Y_MFC2_OP:
    case Y_MFC3_OP:
      set_mem_bypass(RT (inst), VALUE (ps));
      break;

    case Y_MFC1_OP:
      {  /* Read value from register file to avoid extra
	 conversion to double precision */
	float val = FGR [RD (inst)]; /* RD not FS */
	reg_word *vp = (reg_word *) &val;

	set_mem_bypass(RT (inst), *vp);
	break;
      }

    case Y_CTC0_OP:
    case Y_CTC2_OP:
    case Y_CTC3_OP:
      set_CP_bypass((OPCODE (inst) - Y_CTC0_OP), RD (inst), VALUE (ps), 0);
      break;

    case Y_MTC0_OP:
    case Y_MTC2_OP:
    case Y_MTC3_OP:
      set_CP_bypass((OPCODE (inst) - Y_MTC0_OP), RD (inst), VALUE (ps), 1);
      break;

    default:
      /* nothing to do for the following op codes
	case Y_ADD_OP:	case Y_ADDI_OP:	      case Y_ADDIU_OP:     case Y_ADDU_OP:
	case Y_AND_OP:     case Y_ANDI_OP:    case Y_BC0F_OP:      case Y_BC2F_OP:
	case Y_BC3F_OP:    case Y_BC0T_OP:    case Y_BC2T_OP:      case Y_BC3T_OP:
	case Y_BEQ_OP:     case Y_BGEZ_OP:    case Y_BGEZAL_OP:    case Y_BGTZ_OP:
	case Y_BLEZ_OP:    case Y_BLTZ_OP:    case Y_BLTZAL_OP:    case Y_BNE_OP:
	case Y_BREAK_OP:   case Y_COP0_OP:    case Y_COP1_OP:      case Y_COP2_OP:
	case Y_COP3_OP:    case Y_DIV_OP:     case Y_DIVU_OP:      case Y_J_OP:
	case Y_JAL_OP:     case Y_JALR_OP:    case Y_JR_OP:        case Y_LUI_OP
	case Y_MFHI_OP:    case Y_MFLO_OP:    case Y_MTHI_OP:      case Y_MTLO_OP:
	case Y_MULT_OP:	   case Y_MULTU_OP:   case Y_NOR_OP:	   case Y_OR_OP:
	case Y_ORI_OP:	   case Y_RFE_OP:     case Y_SLL_OP:	   case Y_SLLV_OP:
	case Y_SLT_OP:	   case Y_SLTI_OP:    case Y_SLTIU_OP:	   case Y_SLTU_OP:
	case Y_SRA_OP:	   case Y_SRAV_OP:    case Y_SRL_OP:	   case Y_SRLV_OP:
	case Y_SUB_OP:	   case Y_SUBU_OP:    case Y_XOR_OP:	   case Y_XORI_OP:
	*/
      break;
    }

  // possibly add other faults
  faultChecker.checkAndInject_MEM_POST(VALUE(ps));
}

/* Simulate WB stage */
/* when sure it's right combine cases that have the same code. */

int MIPS4KC::process_WB (PIPE_STAGE ps)
{
  instruction *inst;

  inst = ps->inst;
  reg_word &value = VALUE (ps);

  faultChecker.checkAndInject_WB(value);
  reg_word::countWBFaults(value);

  switch (OPCODE (inst))
    {
    case Y_ADD_OP:   case Y_ADDU_OP:  case Y_AND_OP:  case Y_JALR_OP:
    case Y_MFHI_OP:  case Y_MFLO_OP:  case Y_NOR_OP:  case Y_OR_OP:
    case Y_SRLV_OP:  case Y_SRL_OP:   case Y_SRAV_OP: case Y_SRA_OP:
    case Y_SLTU_OP:  case Y_SLT_OP:   case Y_SLLV_OP: case Y_SLL_OP:
    case Y_SUBU_OP:  case Y_SUB_OP:   case Y_XOR_OP:
        //R[RD (inst)] = value;
        faultChecker.checkAndInject_WB_ADDR_FAULT(R,RD(inst),value);
      break;

    case Y_ADDI_OP:  case Y_ADDIU_OP:  case Y_ANDI_OP:  case Y_CFC0_OP:
    case Y_CFC2_OP:  case Y_CFC3_OP:   case Y_LB_OP:
    case Y_LBU_OP:   case Y_LH_OP:     case Y_LHU_OP:   case Y_LUI_OP:
    case Y_LW_OP:    case Y_LWL_OP:    case Y_LWR_OP:   case Y_MFC0_OP:
    case Y_MFC2_OP:  case Y_MFC3_OP:   case Y_ORI_OP:   case Y_SLTI_OP:
    case Y_SLTIU_OP: case Y_XORI_OP:
        //R[RT (inst)] = value;
        faultChecker.checkAndInject_WB_ADDR_FAULT(R,RT(inst),value);
      break;

    case Y_MFC1_OP:
      /* Read value from register file to avoid extra
	 conversion to double precision */
      { float val = FGR [RD (inst)]; /* RD not FS */
	reg_word *vp = (reg_word *) &val;

	//R[RT (inst)] = *vp;	/* Fool coercion */
        faultChecker.checkAndInject_WB_ADDR_FAULT(R,RT(inst),*vp);
	break;
      }


    case Y_BC0F_OP:  case Y_BC2F_OP:  case Y_BC3F_OP:  case Y_BC0T_OP:
    case Y_BC1F_OP:  case Y_BC1T_OP:
    case Y_BC2T_OP:  case Y_BC3T_OP:  case Y_BEQ_OP:   case Y_BGEZ_OP:
    case Y_BGTZ_OP:  case Y_BLEZ_OP:  case Y_BLTZ_OP:  case Y_BNE_OP:
    case Y_J_OP:     case Y_JR_OP:    case Y_SB_OP:    case Y_SH_OP:
    case Y_SW_OP:    case Y_SWC0_OP:  case Y_SWC3_OP:
    case Y_SWL_OP:   case Y_SWR_OP:   case Y_TLBP_OP:  case Y_TLBR_OP:
    case Y_TLBWI_OP: case Y_TLBWR_OP: case Y_SWC1_OP:
      /* no write back */    break;

    case Y_SWC2_OP:
      /* do nothing, SWC2 is being used to print memory stats */
      break;

    case Y_BGEZAL_OP: case Y_BLTZAL_OP: case Y_JAL_OP:
        if (!DSLOT (ps)) {
            //R[31] = value;
            faultChecker.checkAndInject_WB_ADDR_FAULT(R,31,value);
        }
      break;

    case Y_BREAK_OP:
      break;

    case Y_COP0_OP: case Y_COP1_OP:
    case Y_COP2_OP: case Y_COP3_OP:
        //CCR [OPCODE (inst) - Y_COP0_OP] [RD (inst)] = value;
        faultChecker.checkAndInject_WB_ADDR_FAULT(CCR[OPCODE (inst) - Y_COP0_OP],RD(inst),value);
      break;

    case Y_CTC0_OP:
    case Y_CTC2_OP:
    case Y_CTC3_OP:
        //CCR [OPCODE (inst) - Y_CTC0_OP] [RD (inst)] = value;
        faultChecker.checkAndInject_WB_ADDR_FAULT(CCR [OPCODE (inst) - Y_CTC0_OP],RD(inst),value);
      break;

    case Y_DIV_OP:
    case Y_DIVU_OP:
    case Y_MTHI_OP:
    case Y_MTLO_OP:
    case Y_MULT_OP:
    case Y_MULTU_OP:
      /* nothing to do...these values are set in EX/MDU */
      break;


    case Y_LWC0_OP:
    case Y_LWC2_OP:
    case Y_LWC3_OP:
        //CPR [OPCODE (inst) - Y_LWC0_OP] [RT (inst)] = value;
        faultChecker.checkAndInject_WB_ADDR_FAULT(CCR[OPCODE (inst) - Y_LWC0_OP],RT(inst),value);
      break;

    case Y_MTC0_OP:
    case Y_MTC2_OP:
    case Y_MTC3_OP:
        //CPR [OPCODE (inst) - Y_MTC0_OP] [RD (inst)] = value;
        faultChecker.checkAndInject_WB_ADDR_FAULT(CCR[OPCODE (inst) - Y_MTC0_OP],RD(inst),value);
      break;


    case Y_RFE_OP:
      /* Do nothing.  Status_Reg is set in EX */
      break;

    case Y_SYSCALL_OP:
      /* we won't ever get here, syscall causes exception */
      break;

    default:
      /* no EX */
      break;
    }

  return (0);
  /* Executed enough steps, return, but are able to continue. */
  /*  return (1);  */

}



/* Auxiliary functions. */
PIPE_STAGE head_pool;

void MIPS4KC::init_stage_pool (void)
{
  PIPE_STAGE tmp;
  int i;

  //head_pool = (PIPE_STAGE) malloc(12 * sizeof(struct pipe_stage));
  head_pool = (PIPE_STAGE) new pipe_stage[14];
  tmp = head_pool;

  /* chain up the pool of entries */
  for (i=0; i < 13; i++) {
    tmp->next = tmp + 1;
    tmp = tmp->next;
  }

  /* fix up the end */
  tmp->next = NULL;
}


PIPE_STAGE MIPS4KC::stage_alloc (void)
{
  PIPE_STAGE tmp;

  if (head_pool == NULL)
    init_stage_pool();

  tmp = head_pool;
  head_pool = head_pool->next;
  tmp->clear();
  tmp->exception = 0;
  tmp->next = NULL;
  return tmp;
}


void MIPS4KC::stage_dealloc (PIPE_STAGE ps)
{
  if (ps == NULL)
    return;

  // faulted instructions are created for this pipe stage only (they
  // are not in memory), so they should be freed
  if (ps->inst->faulted) {
      free_inst(ps->inst);
  } 
  
  if (head_pool == NULL) {
    head_pool = ps;
    ps->next = NULL;
  }
  else {
    ps->next = head_pool;
    head_pool = ps;
  }
}


void MIPS4KC::pipe_dealloc (int stage, PIPE_STAGE alu[])
{
  int i;

  for(i=0; i < stage; i++) {
    stage_dealloc(alu[i]);
    alu[i] = NULL;    
  }
}






void MIPS4KC::print_pipeline (void)
{
  char *buf;
  int limit;

  buf = (char *) malloc (8*K);
  *buf = '\0';
  limit = 8*K;

  print_pipeline_internal (buf);
  write_output (pipe_out, buf);
  free (buf);
}


void MIPS4KC::print_pipeline_internal (char *buf)
{
  int i;
  PIPE_STAGE tmp;

  sprintf(buf,"*** Control processor pipeline\n");
  buf += strlen(buf);

  sprintf (buf, "WB\t"); buf += strlen(buf);
  if (alu[WB]) {
      buf += print_inst_internal (buf, 8*K, alu[WB]->inst, alu[WB]->pc.getData()) - 1;
    if (EXCPT(alu[WB])) {
      sprintf (buf, "\t[%s EXCPT]", EXCPT_STR((EXCPT(alu[WB])>>2) & 0xf));
      buf += strlen(buf);
    }
  }
  else {
    sprintf (buf, "<none>");
    buf += strlen(buf);
  }
  sprintf (buf++, "\n");

  sprintf (buf, "MEM\t"); buf += strlen(buf);
  if (alu[MEM]) {
      buf += print_inst_internal (buf, 8*K, alu[MEM]->inst, alu[MEM]->pc.getData()) - 1;
    if (EXCPT(alu[MEM])) {
      sprintf (buf, "\t[%s EXCPT]", EXCPT_STR((EXCPT(alu[MEM])>>2) & 0xf));
      buf += strlen(buf);
    }
    if (STAGE(alu[MEM]) == MEM_STALL) {
      sprintf(buf,"\t(STALLED)");
      buf += strlen(buf);
    }
  }
  else {
    sprintf (buf, "<none>");
    buf += strlen(buf);
  }
  sprintf (buf++, "\n");

  sprintf (buf, "EX\t"); buf += strlen(buf);
  if (alu[EX]) {
      buf += print_inst_internal (buf, 8*K, alu[EX]->inst, alu[EX]->pc.getData()) - 1;
    if (EXCPT(alu[EX])) {
      sprintf (buf, "\t[%s EXCPT]", EXCPT_STR(EXCPT(alu[EX])>>2 & 0xf));
      buf += strlen(buf);
    }
  }
  else {
    sprintf (buf,"<none>");
    buf += strlen(buf);
  }
  sprintf (buf++, "\n");

  sprintf (buf, "ID\t"); buf += strlen(buf);
  if (alu[ID]) {
      buf += print_inst_internal (buf, 8*K, alu[ID]->inst, alu[ID]->pc.getData()) - 1;
    if (EXCPT(alu[ID])) {
      sprintf (buf, "\t[%s EXCPT]", EXCPT_STR((EXCPT(alu[ID])>>2) & 0xf));
      buf += strlen(buf);
    }
  }
  else {
    sprintf (buf,"<none>");
    buf += strlen(buf);
  }
  sprintf (buf++, "\n");

  sprintf (buf, "IF\t"); buf += strlen(buf);
  if (alu[IF]) {
      buf += print_inst_internal (buf, 8*K, alu[IF]->inst, alu[IF]->pc.getData()) - 1;
    if (STAGE(alu[IF]) == IF_STALL) {
      sprintf(buf,"\t(STALLED)");
      buf += strlen(buf);
    }
  }
  else {
    sprintf (buf, "<none>");
    buf += strlen(buf);
  }
  sprintf (buf++, "\n");

#if 0 //AFR
  sprintf (buf, "*** Multiply/Divide Unit\n"); buf += strlen(buf);
  //sprintf (buf, "HI 0x%08x\tLO 0x%08x\n", MDU.hi_val, MDU.lo_val);
  buf += strlen(buf);

  sprintf (buf, "*** Floating point pipeline\n"); buf += strlen(buf);

  sprintf (buf, "FWB\t"); buf += strlen(buf);

  tmp = fpa[FPA_FWB];
  for (i = 0; i < 2; i++)
    {
      if (i>0) {
	sprintf (buf, "\t");
	buf++;
      }
      if (!tmp)	{
	sprintf (buf,"<none>");
	buf += strlen(buf);
      }
      else {
	buf += print_inst_internal (buf, 8*K, tmp->inst, tmp->pc) - 1;
	tmp = tmp->next;
      }
      sprintf (buf++, "\n");
    }

  sprintf (buf, "FEX3\t"); buf += strlen(buf);
  tmp = fpa[FPA_EX3];
  for (i = 0; i < 4; i++)
    {
      if (i>0) {
	sprintf (buf, "\t");
	buf++;
      }
      if (!tmp)	{
	sprintf (buf,"<none>");
	buf += strlen(buf);
      }
      else {
	buf += print_inst_internal (buf, 8*K, tmp->inst, tmp->pc) - 1;
	tmp = tmp->next;
      }
      sprintf (buf++, "\n");
    }

  sprintf (buf, "FEX2\t"); buf += strlen(buf);
  if (fpa[FPA_EX2])
    buf += print_inst_internal(buf, 8*K,fpa[FPA_EX2]->inst,fpa[FPA_EX2]->pc) - 1;
  else {
    sprintf (buf,"<none>");
    buf += strlen(buf);
  }
  sprintf (buf++, "\n");

   sprintf (buf, "FEX1\t"); buf += strlen(buf);
  if (fpa[FPA_EX1])
    buf += print_inst_internal(buf, 8*K,fpa[FPA_EX1]->inst,fpa[FPA_EX1]->pc) - 1;
  else {
    sprintf (buf,"<none>");
    buf += strlen(buf);
  }
  sprintf (buf++, "\n");
#endif // omit FP pipe

  sprintf (buf, "*** Bypass Registers, Values\n");
  buf += strlen(buf);

  sprintf (buf, "MEM %d 0x%08x   EX %d 0x%08x\n",MEM_bp_reg,  uint(MEM_bp_val.getData()), EX_bp_reg, uint(EX_bp_val.getData()));
  buf += strlen(buf);

  //sprintf (buf, "*** Write Back Buffer\n");
  //buf += strlen(buf);

  //sprintf (buf, "%s\n", print_write_buffer());
  //buf += strlen(buf);
}


