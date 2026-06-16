
// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include "decoder/vmipsdecoder.h"

#include "inst/vinstall.h"
#include "inst/isatable.h"
#include "vinsbundle.h"

#include "util/vsignx.h"

using namespace SST::Vanadis;

VanadisMIPSDecoder::VanadisMIPSDecoder(ComponentId_t id, Params& params, SST::Output* output) : VanadisDecoder(id, params, output)
{

    // 32 int + hi/lo (2) = 34
    // 32 fp + ver + status (2) = 34
    // reg-2 is for sys-call codes
    // plus 2 for LO/HI registers in INT
    options_ = new VanadisDecoderOptions((uint16_t)0, 34, 34, 2, VANADIS_REGISTER_MODE_FP32);

    // See if we get an entry point the sub-component says we have to use
    // if not, we will fall back to ELF reading at the core level to work this
    // out
    setInstructionPointer(params.find<uint64_t>("entry_point", 0));

    stat_decode_add_       = registerStatistic<uint64_t>("ins_decode_add", "1");
    stat_decode_addu_      = registerStatistic<uint64_t>("ins_decode_addu", "1");
    stat_decode_and_       = registerStatistic<uint64_t>("ins_decode_and", "1");
    stat_decode_dadd_      = registerStatistic<uint64_t>("ins_decode_dadd", "1");
    stat_decode_daddu_     = registerStatistic<uint64_t>("ins_decode_daddu", "1");
    stat_decode_ddiv_      = registerStatistic<uint64_t>("ins_decode_ddiv", "1");
    stat_decode_div_       = registerStatistic<uint64_t>("ins_decode_div", "1");
    stat_decode_divu_      = registerStatistic<uint64_t>("ins_decode_divu", "1");
    stat_decode_dmult_     = registerStatistic<uint64_t>("ins_decode_dmult", "1");
    stat_decode_dmultu_    = registerStatistic<uint64_t>("ins_decode_dmultu", "1");
    stat_decode_dsllv_     = registerStatistic<uint64_t>("ins_decode_dsllv", "1");
    stat_decode_dsrav_     = registerStatistic<uint64_t>("ins_decode_dsrav", "1");
    stat_decode_dsrlv_     = registerStatistic<uint64_t>("ins_decode_dsrlv", "1");
    stat_decode_dsub_      = registerStatistic<uint64_t>("ins_decode_dsub", "1");
    stat_decode_dsubu_     = registerStatistic<uint64_t>("ins_decode_dsubu", "1");
    stat_decode_jr_        = registerStatistic<uint64_t>("ins_decode_jr", "1");
    stat_decode_jalr_      = registerStatistic<uint64_t>("ins_decode_jalr", "1");
    stat_decode_mfhi_      = registerStatistic<uint64_t>("ins_decode_mfhi", "1");
    stat_decode_mflo_      = registerStatistic<uint64_t>("ins_decode_mflo", "1");
    stat_decode_mult_      = registerStatistic<uint64_t>("ins_decode_mult", "1");
    stat_decode_multu_     = registerStatistic<uint64_t>("ins_decode_multu", "1");
    stat_decode_nor_       = registerStatistic<uint64_t>("ins_decode_nor", "1");
    stat_decode_or_        = registerStatistic<uint64_t>("ins_decode_or", "1");
    stat_decode_sllv_      = registerStatistic<uint64_t>("ins_decode_sllv", "1");
    stat_decode_slt_       = registerStatistic<uint64_t>("ins_decode_slt", "1");
    stat_decode_sltu_      = registerStatistic<uint64_t>("ins_decode_sltu", "1");
    stat_decode_srav_      = registerStatistic<uint64_t>("ins_decode_srav", "1");
    stat_decode_srlv_      = registerStatistic<uint64_t>("ins_decode_srlv", "1");
    stat_decode_sub_       = registerStatistic<uint64_t>("ins_decode_sub", "1");
    stat_decode_subu_      = registerStatistic<uint64_t>("ins_decode_subu", "1");
    stat_decode_syscall_   = registerStatistic<uint64_t>("ins_decode_syscall", "1");
    stat_decode_sync_      = registerStatistic<uint64_t>("ins_decode_sync", "1");
    stat_decode_xor_       = registerStatistic<uint64_t>("ins_decode_xor", "1");
    stat_decode_sll_       = registerStatistic<uint64_t>("ins_decode_sll", "1");
    stat_decode_srl_       = registerStatistic<uint64_t>("ins_decode_srl", "1");
    stat_decode_sra_       = registerStatistic<uint64_t>("ins_decode_sra", "1");
    stat_decode_bltz_      = registerStatistic<uint64_t>("ins_decode_bltz", "1");
    stat_decode_bgezal_    = registerStatistic<uint64_t>("ins_decode_bgezal", "1");
    stat_decode_bgez_      = registerStatistic<uint64_t>("ins_decode_bgez", "1");
    stat_decode_lui_       = registerStatistic<uint64_t>("ins_decode_lui", "1");
    stat_decode_lb_        = registerStatistic<uint64_t>("ins_decode_lb", "1");
    stat_decode_lbu_       = registerStatistic<uint64_t>("ins_decode_lbu", "1");
    stat_decode_lhu_       = registerStatistic<uint64_t>("ins_decode_lhu", "1");
    stat_decode_lh_        = registerStatistic<uint64_t>("ins_decode_lh", "1");
    stat_decode_lw_        = registerStatistic<uint64_t>("ins_decode_lw", "1");
    stat_decode_lfp32_     = registerStatistic<uint64_t>("ins_decode_lfp32", "1");
    stat_decode_ll_        = registerStatistic<uint64_t>("ins_decode_ll", "1");
    stat_decode_lwl_       = registerStatistic<uint64_t>("ins_decode_lwl", "1");
    stat_decode_lwr_       = registerStatistic<uint64_t>("ins_decode_lwr", "1");
    stat_decode_sb_        = registerStatistic<uint64_t>("ins_decode_sb", "1");
    stat_decode_sc_        = registerStatistic<uint64_t>("ins_decode_sc", "1");
    stat_decode_sw_        = registerStatistic<uint64_t>("ins_decode_sw", "1");
    stat_decode_sh_        = registerStatistic<uint64_t>("ins_decode_sh", "1");
    stat_decode_sfp32_     = registerStatistic<uint64_t>("ins_decode_sfp32", "1");
    stat_decode_swr_       = registerStatistic<uint64_t>("ins_decode_swr", "1");
    stat_decode_swl_       = registerStatistic<uint64_t>("ins_decode_swl", "1");
    stat_decode_addiu_     = registerStatistic<uint64_t>("ins_decode_addiu", "1");
    stat_decode_beq_       = registerStatistic<uint64_t>("ins_decode_beq", "1");
    stat_decode_bgtz_      = registerStatistic<uint64_t>("ins_decode_bgtz", "1");
    stat_decode_blez_      = registerStatistic<uint64_t>("ins_decode_blez", "1");
    stat_decode_bne_       = registerStatistic<uint64_t>("ins_decode_bne", "1");
    stat_decode_slti_      = registerStatistic<uint64_t>("ins_decode_slti", "1");
    stat_decode_sltiu_     = registerStatistic<uint64_t>("ins_decode_sltiu", "1");
    stat_decode_andi_      = registerStatistic<uint64_t>("ins_decode_andi", "1");
    stat_decode_ori_       = registerStatistic<uint64_t>("ins_decode_ori", "1");
    stat_decode_j_         = registerStatistic<uint64_t>("ins_decode_j", "1");
    stat_decode_jal_       = registerStatistic<uint64_t>("ins_decode_jal", "1");
    stat_decode_xori_      = registerStatistic<uint64_t>("ins_decode_xori", "1");
    stat_decode_rdhwr_     = registerStatistic<uint64_t>("ins_decode_rdhwr", "1");
    stat_decode_cop1_mtc_  = registerStatistic<uint64_t>("ins_decode_cop1_mtc", "1");
    stat_decode_cop1_mfc_  = registerStatistic<uint64_t>("ins_decode_cop1_mfc", "1");
    stat_decode_cop1_cf_   = registerStatistic<uint64_t>("ins_decode_cop1_cf", "1");
    stat_decode_cop1_ct_   = registerStatistic<uint64_t>("ins_decode_cop1_ct", "1");
    stat_decode_cop1_mov_  = registerStatistic<uint64_t>("ins_decode_cop1_mov", "1");
    stat_decode_cop1_mul_  = registerStatistic<uint64_t>("ins_decode_cop1_mul", "1");
    stat_decode_cop1_div_  = registerStatistic<uint64_t>("ins_decode_cop1_div", "1");
    stat_decode_cop1_sub_  = registerStatistic<uint64_t>("ins_decode_cop1_sub", "1");
    stat_decode_cop1_cvts_ = registerStatistic<uint64_t>("ins_decode_cop1_cvts", "1");
    stat_decode_cop1_cvtd_ = registerStatistic<uint64_t>("ins_decode_cop1_cvtd", "1");
    stat_decode_cop1_cvtw_ = registerStatistic<uint64_t>("ins_decode_cop1_cvtw", "1");
    stat_decode_cop1_lt_   = registerStatistic<uint64_t>("ins_decode_cop1_lt", "1");
    stat_decode_cop1_ult_  = registerStatistic<uint64_t>("ins_decode_cop1_ult", "1");
    stat_decode_cop1_lte_  = registerStatistic<uint64_t>("ins_decode_cop1_lte", "1");
    stat_decode_cop1_eq_   = registerStatistic<uint64_t>("ins_decode_cop1_eq", "1");
}

VanadisMIPSDecoder::~VanadisMIPSDecoder() {}

void VanadisMIPSDecoder::setStackPointer( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t start_stack_address )
{
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(
        CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> Setting SP to (64B-aligned):          %" PRIu64 " / 0x%0" PRI_ADDR "\n", start_stack_address,
        start_stack_address);
    #endif
    const int16_t sp_phys_reg = isa_tbl->getIntPhysReg(29);
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> Stack Pointer (r29) maps to phys-reg: %" PRIu16 "\n", sp_phys_reg);
    #endif
    // Set up the stack pointer
    // Register 29 is MIPS for Stack Pointer
    regFile->setIntReg(sp_phys_reg, start_stack_address);
}

void VanadisMIPSDecoder::setArg1Register( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value )
{
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(
        CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> Setting argument 1 register to (64B-aligned):          %" PRIu64 " / 0x%0" PRI_ADDR "\n", value,
        value);
    #endif
    const int16_t sp_phys_reg = isa_tbl->getIntPhysReg(4);
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> argument 1 (r4) maps to phys-reg: %" PRIu16 "\n", sp_phys_reg);
    #endif
    regFile->setIntReg(sp_phys_reg, value);
}

void VanadisMIPSDecoder::setFuncPointer( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value )
{
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(
        CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> Setting register 25 to (64B-aligned):          %" PRIu64 " / 0x%0" PRI_ADDR "\n", value,
        value);
    #endif
    const int16_t sp_phys_reg = isa_tbl->getIntPhysReg(25);
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> r25 maps to phys-reg: %" PRIu16 "\n", sp_phys_reg);
    #endif
    regFile->setIntReg(sp_phys_reg, value);
}

void VanadisMIPSDecoder::setReturnRegister( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value )
{
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(
        CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> Setting register 2 to (64B-aligned):          %" PRIu64 " / 0x%0" PRI_ADDR "\n", value,
        value);
    #endif
    const int16_t sp_phys_reg = isa_tbl->getIntPhysReg(2);
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> r2 maps to phys-reg: %" PRIu16 "\n", sp_phys_reg);
    #endif
    regFile->setIntReg(sp_phys_reg, value);
}

void VanadisMIPSDecoder::setSuccessRegister( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value )
{
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(
        CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> Setting register 7 to (64B-aligned):          %" PRIu64 " / 0x%0" PRI_ADDR "\n", value,
        value);
    #endif
    const int16_t sp_phys_reg = isa_tbl->getIntPhysReg(7);
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> r7 maps to phys-reg: %" PRIu16 "\n", sp_phys_reg);
    #endif
    regFile->setIntReg(sp_phys_reg, value);
}

bool VanadisMIPSDecoder::tick( uint64_t cycle )
{
    cycle_count_ = cycle;

    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> Decode step for thr: %" PRIu32 "\n", hw_thr);
    ins_loader->printStatus();
    uint16_t decodes_performed = 0;
    #endif


    if ( thread_rob->full() ) {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "---> Decoded pending issue queue is full, no decodes permitted.\n");
        #endif
        return false;
    }


    uint16_t uop_bundles_used  = 0;
    bool success = false;

    // if the ROB has space, then lets go ahead and
    // decode the input, put it in the queue for issue.
    VanadisInstructionBundle* bundle = ins_loader->getBundleAt(ip);
    if ( bundle ) {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "---> Found uop bundle for ip=0x0%" PRI_ADDR ", loading from cache...\n", ip);
        output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-----> Bundle contains %" PRIu32 " entries.\n", bundle->getInstructionCount());
        #endif

        stat_uop_hit_->addData(1);

        if ( 0 == bundle->getInstructionCount() ) {
            output_->fatal(CALL_INFO, -1, "------> STOP, ERROR - bundle at 0x%0" PRI_ADDR " contains zero entries.\n", ip);
        }

        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "----> thr-rob contains %" PRIu32 " entries.\n", (uint32_t)thread_rob->size());
        #endif

        // Check if last instruction is a BRANCH, if yes, we need to also
        // decode the branch-delay slot AND handle the prediction
        if ( bundle->getInstructionByIndex(bundle->getInstructionCount() - 1)->getInstFuncType() == INST_BRANCH ) {
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, 
                "-----> Last instruction in the bundle causes potential branch, checking on branch delay slot\n");
            #endif

            VanadisInstructionBundle* delay_bundle = ins_loader->getBundleAt(ip + 4);
            uint32_t                  temp_delay   = 0;
            

            if ( delay_bundle ) {
                // We have also decoded the branch-delay
                stat_uop_hit_->addData(1);
            }
            else {
                #ifdef VANADIS_BUILD_DEBUG
                output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-----> Branch delay slot is not currently decoded into a bundle.\n");
                #endif
                if ( ins_loader->hasPredecodeAt(ip + 4, 4) ) {
                    #ifdef VANADIS_BUILD_DEBUG
                    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, 
                        "-----> Branch delay slot is a pre-decode cache item, decode it and keep bundle.\n");
                    decodes_performed++;
                    #endif
                    delay_bundle = new VanadisInstructionBundle(ip + 4);

                    if ( ins_loader->getPredecodeBytes(ip + 4, (uint8_t*)&temp_delay, sizeof(temp_delay)) ) {
                        stat_predecode_hit_->addData(1);

                        decode(ip + 4, temp_delay, delay_bundle);
                        ins_loader->cacheDecodedBundle(delay_bundle);
                    }
                    else {
                        output_->fatal(CALL_INFO, -1,
                            "Error: instruction loader has bytes for delay slot at %0" PRI_ADDR ", but they cannot be retrieved.\n", (ip + 4));
                    }
                }
                else {
                    #ifdef VANADIS_BUILD_DEBUG
                    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, 
                        "-----> Branch delay slot also misses in pre-decode cache, need to request it.\n");
                    #endif
                    ins_loader->requestLoadAt(ip + 4, 4);
                    stat_ins_bytes_loaded_->addData(4);
                    stat_predecode_miss_->addData(1);
                }
            }

            // We have the branch AND the delay, now lets issue them.
            if ( nullptr != delay_bundle ) {
                if ( (bundle->getInstructionCount() + delay_bundle->getInstructionCount()) < (thread_rob->capacity() - thread_rob->size()) ) {

                    #ifdef VANADIS_BUILD_DEBUG
                    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "---> Proceeding with issue the branch and its delay slot...\n");
                    #endif

                    for ( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
                        VanadisInstruction* next_ins = bundle->getInstructionByIndex(i)->clone();

                        #ifdef VANADIS_BUILD_DEBUG
                        output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, 
                            "---> --> issuing ins addr: 0x0%" PRI_ADDR ", %s...\n", next_ins->getInstructionAddress(), next_ins->getInstCode());
                        #endif

                        thread_rob->push(next_ins);

                        // if this is the last instruction in the bundle
                        // we need to obtain a branch prediction
                        if ( i == (bundle->getInstructionCount() - 1) ) {
                            VanadisSpeculatedInstruction* speculated_ins =
                                dynamic_cast<VanadisSpeculatedInstruction*>(next_ins);

                            // Do we have an entry for the branch instruction we just
                            // issued
                            auto [predicted, predicted_address] = branch_predictor->predictAddressIfAvailable(ip);
                            if ( predicted ) {
                                speculated_ins->setSpeculatedAddress(predicted_address);

                                // This is essentially a predicted not taken branch
                                #ifdef VANADIS_BUILD_DEBUG
                                if ( predicted_address == (ip + 8) ) {
                                    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                                        "---> Branch 0x%" PRI_ADDR " predicted not taken, ip set to: 0x%0" PRI_ADDR "\n", ip, predicted_address);
                                }
                                else {
                                    output_->verbose( CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                                        "---> Branch 0x%" PRI_ADDR " predicted taken, jump to 0x%0" PRI_ADDR "\n", ip, predicted_address);
                                }
                                output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                                    "---> Forcing IP update according to branch prediction table, new-ip: %0" PRI_ADDR "\n", predicted_address);
                                #endif

                                ip = predicted_address;
                            }
                            else {
                                #ifdef VANADIS_BUILD_DEBUG
                                output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                                    "---> Branch table does not contain an entry for ins: 0x%0" PRI_ADDR ", continue with "
                                    "normal ip += 8 = 0x%0" PRI_ADDR "\n", ip, (ip + 8));
                                #endif

                                speculated_ins->setSpeculatedAddress(ip + 8);

                                // We don't urgh.. let's just carry on
                                // remember we increment the IP by 2 instructions (me +
                                // delay)
                                ip += 8;
                            }
                        }
                    }

                    for ( uint32_t i = 0; i < delay_bundle->getInstructionCount(); ++i ) {
                        VanadisInstruction* next_ins = delay_bundle->getInstructionByIndex(i)->clone();

                        #ifdef VANADIS_BUILD_DEBUG
                        output_->verbose(
                            CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "---> --> issuing ins addr: 0x0%" PRI_ADDR ", %s...\n",
                            next_ins->getInstructionAddress(), next_ins->getInstCode());
                        #endif
                        thread_rob->push(next_ins);
                    }

                    uop_bundles_used += 2;
                    success = true;
                }
                else {
                    #ifdef VANADIS_BUILD_DEBUG
                    output_->verbose(
                        CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                        "---> --> micro-op for branch and delay exceed decode-q space. Cannot issue this cycle.\n");
                    #endif
                    stat_uop_delayed_rob_full_->addData(1);
                }
            }
        }
        else {
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(
                CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                "---> Instruction for issue is not a branch, continuing with normal copy to issue-queue...\n");
            #endif
            // Do we have enough space in the decode queue for the bundle contents?
            if ( bundle->getInstructionCount() < (thread_rob->capacity() - thread_rob->size()) ) {
                // Put in the queue
                for ( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
                    VanadisInstruction* next_ins = bundle->getInstructionByIndex(i);

                    #ifdef VANADIS_BUILD_DEBUG
                    output_->verbose(
                        CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "---> --> issuing ins addr: 0x0%" PRI_ADDR ", %s...\n",
                        next_ins->getInstructionAddress(), next_ins->getInstCode());
                    #endif
                    thread_rob->push(next_ins->clone());
                }

                uop_bundles_used++;
                success = true;

                // Push the instruction pointer along by the standard amount
                ip += 4;
            }
            else {
                #ifdef VANADIS_BUILD_DEBUG
                output_->verbose(
                    CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                    "---> --> micro-op bundle for %p contains %" PRIu32 " ops, we only have %" PRIu32
                    " slots available in the decode q, wait for resources to become available.\n",
                    (void*)ip, (uint32_t)bundle->getInstructionCount(),
                    (uint32_t)(thread_rob->capacity() - thread_rob->size()));
                #endif
                // We don't have enough space, so we have to stop and wait for
                // more entries to free up.
                stat_uop_delayed_rob_full_->addData(1);
            }
        }
    }
    else if ( ins_loader->hasPredecodeAt(ip, 4) ) {
        // We do have a locally cached copy of the data at the IP though, so
        // decode into a bundle
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(
            CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
            "---> uop not found, but matched in predecoded L0-icache (ip=%p)\n", (void*)ip);
        #endif
        stat_predecode_hit_->addData(1);

        uint32_t                  temp_ins       = 0;
        VanadisInstructionBundle* decoded_bundle = new VanadisInstructionBundle(ip);

        if ( ins_loader->getPredecodeBytes(ip, (uint8_t*)&temp_ins, sizeof(temp_ins)) ) {
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                "---> performing a decode of the bytes found (ins-bytes: 0x%x)\n", temp_ins);
            #endif
            decode(ip, temp_ins, decoded_bundle);

            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                "---> performing a decode of the bytes found (generates %" PRIu32 " micro-op bundle).\n",
                (uint32_t)decoded_bundle->getInstructionCount());
            decodes_performed++;
            #endif
            ins_loader->cacheDecodedBundle(decoded_bundle);
            success = true;
        }
        else {
            output_->fatal(CALL_INFO, -1,
                "Error: predecoded bytes found at ip=%p, but %d byte retrieval failed.\n", (void*)ip, (int)sizeof(temp_ins));
        }
    }
    else if ( !ins_loader->pendingLoad(ip, 4) ) {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
            "---> uop bundle and pre-decoded bytes are not found (ip=%p), requesting icache read (line-width=%" PRIu64 ")\n",
            (void*)ip, ins_loader->getCacheLineWidth());
        #endif
        ins_loader->requestLoadAt(ip, 4);
        stat_ins_bytes_loaded_->addData(4);
        stat_predecode_miss_->addData(1);
        success = true;
    }
    
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
        "---> Performed %" PRIu16 " decodes this cycle, %" PRIu16 " uop-bundles used / updated-ip: 0x%" PRI_ADDR ".\n",
        decodes_performed, uop_bundles_used, ip);
    #endif
    return success;
}

void VanadisMIPSDecoder::decode( const uint64_t ins_addr, const uint32_t next_ins, VanadisInstructionBundle* bundle )
{
    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "[decode] > addr: 0x%" PRI_ADDR " ins: 0x%08x\n", ins_addr, next_ins);
    #endif

    const uint32_t hw_thr    = getHardwareThread();
    const uint32_t ins_mask  = next_ins & MIPS_OP_MASK;
    const uint32_t func_mask = next_ins & MIPS_FUNC_MASK;

    // Unaligned address *or* IP is 0
    if ( 0 != (ins_addr & 0x3) || 0 == ins_addr ) {
        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(
            CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "[decode] ---> fault address 0x%" PRIu64 " is not aligned at 4 bytes.\n", ins_addr);
        #endif
        bundle->addInstruction(new VanadisInstructionDecodeFault(ins_addr, hw_thr, options_));
        return;
    }

    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "[decode] ---> ins-mask: 0x%08x / 0x%08x\n", ins_mask, func_mask);
    #endif

    // Check if this is a NOP, this is fairly frequent due to use in delay slots, do not spend time decoding this
    if ( 0 == next_ins ) {
        bundle->addInstruction(new VanadisNoOpInstruction(ins_addr, hw_thr, options_));
        stat_uop_generated_->addData(bundle->getInstructionCount());
        #ifdef VANADIS_BUILD_DEBUG
        for ( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> [%3" PRIu32 "]: %s\n", 
                i, bundle->getInstructionByIndex(i)->getInstCode());
        }
        #endif
        return;
    }

    // Regular, non-NOP instruction
    bool insert_decode_fault = false;

    uint16_t rt = 0;
    uint16_t rs = 0;
    uint16_t rd = 0;
    uint32_t imm = 0;

    // Perform a register and parameter extract in case we need later
    extract_three_regs(next_ins, &rt, &rs, &rd);
    extract_imm(next_ins, &imm);
    const uint64_t imm64 = (uint64_t)imm;

    #ifdef VANADIS_BUILD_DEBUG
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "[decode] rt=%" PRIu32 ", rs=%" PRIu32 ", rd=%" PRIu32 "\n", rt, rs, rd);
    output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "[decode] -> inst-mask: 0x%08x\n", ins_mask);
    #endif

    switch ( ins_mask ) {
    case 0:
    {
        // The SHIFT 5 bits must be zero for these operations according to the manual
        if ( 0 == (next_ins & MIPS_SHFT_MASK) ) {
            #ifdef VANADIS_BUILD_DEBUG
            output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "[decode] -> special-class, func-mask: 0x%x\n", func_mask);
            #endif

            if ( (0 == func_mask) && (0 == rs) ) {
                #ifdef VANADIS_BUILD_DEBUG
                output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG,
                    "[decode] -> rs is also zero, implies truncate (generate: 64 to 32 truncate)\n");
                #endif
                bundle->addInstruction(
                    new VanadisTruncateInstruction<
                        VanadisRegisterFormat::VANADIS_FORMAT_INT64,
                        VanadisRegisterFormat::VANADIS_FORMAT_INT32>(ins_addr, hw_thr, options_, rd, rt));
            }
            else {
                switch ( func_mask ) {
                case MIPS_SPEC_OP_MASK_ADD:
                {
                    bundle->addInstruction(new VanadisAddInstruction<int32_t>(ins_addr, hw_thr, options_, rd, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_add_);
                } break;

                case MIPS_SPEC_OP_MASK_ADDU:
                {
                    bundle->addInstruction(new VanadisAddInstruction<int32_t>(ins_addr, hw_thr, options_, rd, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_addu_);
                } break;

                case MIPS_SPEC_OP_MASK_AND:
                {
                    bundle->addInstruction(new VanadisAndInstruction(ins_addr, hw_thr, options_, rd, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_and_);
                } break;

                case MIPS_SPEC_OP_MASK_DADD:
                case MIPS_SPEC_OP_MASK_DADDU:
                case MIPS_SPEC_OP_MASK_DDIV:
                case MIPS_SPEC_OP_MASK_DDIVU:
                    insert_decode_fault = true;
                    break;

                case MIPS_SPEC_OP_MASK_DIV:
                {
                    bundle->addInstruction(new VanadisDivideRemainderInstruction<int32_t>(
                        ins_addr, hw_thr, options_, MIPS_REG_LO, MIPS_REG_HI, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_div_);
                } break;

                case MIPS_SPEC_OP_MASK_DIVU:
                {
                    bundle->addInstruction(new VanadisDivideRemainderInstruction<uint32_t>(
                        ins_addr, hw_thr, options_, MIPS_REG_LO, MIPS_REG_HI, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_divu_);
                } break;

                case MIPS_SPEC_OP_MASK_DMULT:
                case MIPS_SPEC_OP_MASK_DMULTU:
                case MIPS_SPEC_OP_MASK_DSLLV:
                case MIPS_SPEC_OP_MASK_DSRAV:
                case MIPS_SPEC_OP_MASK_DSRLV:
                case MIPS_SPEC_OP_MASK_DSUB:
                case MIPS_SPEC_OP_MASK_DSUBU:
                    insert_decode_fault = true;
                    break;

                case MIPS_SPEC_OP_MASK_JR:
                {
                    bundle->addInstruction(new VanadisJumpRegInstruction(ins_addr, hw_thr, options_, 4, rs, VANADIS_SINGLE_DELAY_SLOT));
                    MIPS_INC_DECODE_STAT(stat_decode_jr_);
                } break;

                case MIPS_SPEC_OP_MASK_JALR:
                {
                    bundle->addInstruction(new VanadisJumpRegLinkInstruction(ins_addr, hw_thr, options_, 4, rd, rs, 0, VANADIS_SINGLE_DELAY_SLOT));
                    MIPS_INC_DECODE_STAT(stat_decode_jalr_);
                } break;

                case MIPS_SPEC_OP_MASK_MFHI:
                {
                    // Special instruction_, 32 is LO, 33 is HI
                    bundle->addInstruction(new VanadisAddImmInstruction<int32_t>(ins_addr, hw_thr, options_, rd, MIPS_REG_HI, 0));
                    MIPS_INC_DECODE_STAT(stat_decode_mfhi_);
                } break;

                case MIPS_SPEC_OP_MASK_MFLO:
                {
                    // Special instruction, 32 is LO, 33 is HI
                    bundle->addInstruction(new VanadisAddImmInstruction<int32_t>(ins_addr, hw_thr, options_, rd, MIPS_REG_LO, 0));
                    MIPS_INC_DECODE_STAT(stat_decode_mflo_);
                } break;

                case MIPS_SPEC_OP_MASK_MOVN:
                {
                    bundle->addInstruction(new VanadisMoveCompareImmInstruction<int32_t>(
                        ins_addr, hw_thr, options_, rd, rs, rt, 0, REG_COMPARE_NEQ));
                } break;

                case MIPS_SPEC_OP_MASK_MOVZ:
                {
                    bundle->addInstruction(new VanadisMoveCompareImmInstruction<int32_t>(
                        ins_addr, hw_thr, options_, rd, rs, rt, 0, REG_COMPARE_EQ));
                } break;

                case MIPS_SPEC_OP_MASK_MTHI:
                case MIPS_SPEC_OP_MASK_MTLO:
                    insert_decode_fault = true;
                    break;

                case MIPS_SPEC_OP_MASK_MULT:
                {
                    bundle->addInstruction(
                        new VanadisMultiplySplitInstruction<int32_t>(ins_addr, hw_thr, options_, MIPS_REG_LO, MIPS_REG_HI, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_mult_);
                } break;

                case MIPS_SPEC_OP_MASK_MULTU:
                {
                    bundle->addInstruction(
                        new VanadisMultiplySplitInstruction<uint32_t>(ins_addr, hw_thr, options_, MIPS_REG_LO, MIPS_REG_HI, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_multu_);
                } break;

                case MIPS_SPEC_OP_MASK_NOR:
                {
                    bundle->addInstruction(new VanadisNorInstruction(ins_addr, hw_thr, options_, rd, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_nor_);
                } break;

                case MIPS_SPEC_OP_MASK_OR:
                {
                    bundle->addInstruction(new VanadisOrInstruction(ins_addr, hw_thr, options_, rd, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_or_);
                } break;

                case MIPS_SPEC_OP_MASK_SLLV:
                {
                    bundle->addInstruction(
                        new VanadisShiftLeftLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                            ins_addr, hw_thr, options_, rd, rt, rs));
                    MIPS_INC_DECODE_STAT(stat_decode_sllv_);
                } break;

                case MIPS_SPEC_OP_MASK_SLT:
                {
                    bundle->addInstruction(
                        new VanadisSetRegCompareInstruction<REG_COMPARE_LT, int32_t>(ins_addr, hw_thr, options_, rd, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_slt_);
                } break;

                case MIPS_SPEC_OP_MASK_SLTU:
                {
                    bundle->addInstruction(
                        new VanadisSetRegCompareInstruction<REG_COMPARE_LT, uint32_t>(ins_addr, hw_thr, options_, rd, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_sltu_);
                } break;

                case MIPS_SPEC_OP_MASK_SRAV:
                {
                    bundle->addInstruction(
                        new VanadisShiftRightArithmeticInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                            ins_addr, hw_thr, options_, rd, rt, rs));
                    MIPS_INC_DECODE_STAT(stat_decode_srav_);
                } break;

                case MIPS_SPEC_OP_MASK_SRLV:
                {
                    bundle->addInstruction(
                        new VanadisShiftRightLogicalInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                            ins_addr, hw_thr, options_, rd, rt, rs));
                    MIPS_INC_DECODE_STAT(stat_decode_srlv_);
                } break;

                case MIPS_SPEC_OP_MASK_SUB:
                {
                    bundle->addInstruction(new VanadisSubInstruction<int32_t>(ins_addr, hw_thr, options_, rd, rs, rt, true));
                    MIPS_INC_DECODE_STAT(stat_decode_sub_);
                } break;

                case MIPS_SPEC_OP_MASK_SUBU:
                {
                    bundle->addInstruction(
                        new VanadisSubInstruction<int32_t>(ins_addr, hw_thr, options_, rd, rs, rt, false));
                    MIPS_INC_DECODE_STAT(stat_decode_subu_);
                } break;

                case MIPS_SPEC_OP_MASK_SYSCALL:
                {
                    bundle->addInstruction(new VanadisFenceInstruction(ins_addr, hw_thr, options_, VANADIS_LOAD_STORE_FENCE));
                    bundle->addInstruction(new VanadisSysCallInstruction(ins_addr, hw_thr, options_));
                    MIPS_INC_DECODE_STAT(stat_decode_syscall_);
                } break;

                case MIPS_SPEC_OP_MASK_SYNC:
                {
                    bundle->addInstruction(
                        new VanadisFenceInstruction(ins_addr, hw_thr, options_, VANADIS_LOAD_STORE_FENCE));
                    MIPS_INC_DECODE_STAT(stat_decode_sync_);
                } break;

                case MIPS_SPEC_OP_MASK_XOR:
                {
                    bundle->addInstruction(new VanadisXorInstruction(ins_addr, hw_thr, options_, rd, rs, rt));
                    MIPS_INC_DECODE_STAT(stat_decode_xor_);
                } break;

                default:
                    insert_decode_fault = true;
                    break;
                }
            }
        }
        else {
            switch ( func_mask ) {
            case MIPS_SPEC_OP_MASK_SLL:
            {
                const uint64_t shf_amnt = ((uint64_t)(next_ins & MIPS_SHFT_MASK)) >> 6;

                #ifdef VANADIS_BUILD_DEBUG
                output_->verbose(CALL_INFO, 16, 0, "[decode/SLL]-> out: %" PRIu16 " / in: %" PRIu16 " shft: %" PRIu64 "\n", rd, rt, shf_amnt);
                #endif

                bundle->addInstruction(new VanadisShiftLeftLogicalImmInstruction<uint32_t>(ins_addr, hw_thr, options_, rd, rt, shf_amnt));
                MIPS_INC_DECODE_STAT(stat_decode_sll_);
            } break;

            case MIPS_SPEC_OP_MASK_SRL:
            {
                const uint64_t shf_amnt = ((uint64_t)(next_ins & MIPS_SHFT_MASK)) >> 6;

                #ifdef VANADIS_BUILD_DEBUG
                output_->verbose(CALL_INFO, 16, 0, "[decode/SRL]-> out: %" PRIu16 " / in: %" PRIu16 " shft: %" PRIu64 "\n", rd, rt, shf_amnt);
                #endif

                bundle->addInstruction(
                    new VanadisShiftRightLogicalImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                        ins_addr, hw_thr, options_, rd, rt, shf_amnt));
                MIPS_INC_DECODE_STAT(stat_decode_srl_);
            } break;

            case MIPS_SPEC_OP_MASK_SRA:
            {
                const uint64_t shf_amnt = ((uint64_t)(next_ins & MIPS_SHFT_MASK)) >> 6;

                bundle->addInstruction(
                    new VanadisShiftRightArithmeticImmInstruction<VanadisRegisterFormat::VANADIS_FORMAT_INT32>(
                        ins_addr, hw_thr, options_, rd, rt, shf_amnt));
                MIPS_INC_DECODE_STAT(stat_decode_sra_);
            } break;

            default:
                insert_decode_fault = true;
                break;
            }
        }
    } break; // End case 0

    case MIPS_SPEC_OP_MASK_LW:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/LW]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 4, true, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_lw_);
    } break;

    case MIPS_SPEC_OP_MASK_SW:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/SW]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisStoreInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 4, MEM_TRANSACTION_NONE, STORE_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_sw_);
    } break;

    case MIPS_SPEC_OP_MASK_LB:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/LB]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);

        bundle->addInstruction(new VanadisLoadInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 1, true, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
    } break;

    case MIPS_SPEC_OP_MASK_LBU:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/LBU]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);

        bundle->addInstruction(new VanadisLoadInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 1, false, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_lbu_);
    } break;

    case MIPS_SPEC_OP_MASK_LFP32:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/LFP32]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 4, true, MEM_TRANSACTION_NONE, LOAD_FP_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_lfp32_);
    } break;

    case MIPS_SPEC_OP_MASK_LL:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/LL]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 4, true, MEM_TRANSACTION_LLSC_LOAD, LOAD_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_ll_);
    } break;

    case MIPS_SPEC_OP_MASK_LWL:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/LWL (PARTLOAD)]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisPartialLoadInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 4, true, false, LOAD_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_lwl_);
    } break;

    case MIPS_SPEC_OP_MASK_LWR:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/LWR (PARTLOAD)]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisPartialLoadInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 4, true, true, LOAD_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_lwr_);
    } break;

    case MIPS_SPEC_OP_MASK_LHU:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/LHU]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 2, false, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_lhu_);
    } break;

    case MIPS_SPEC_OP_MASK_LH:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/LH]: -> reg: %" PRIu16 " <- base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisLoadInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 2, true, MEM_TRANSACTION_NONE, LOAD_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_lh_);
    } break;

    case MIPS_SPEC_OP_MASK_SB:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/SB]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisStoreInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 1, MEM_TRANSACTION_NONE, STORE_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_sb_);
    } break;

    case MIPS_SPEC_OP_MASK_SH:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/SH]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisStoreInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 2, MEM_TRANSACTION_NONE, STORE_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_sh_);
    } break;

    case MIPS_SPEC_OP_MASK_SFP32:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/SFP32]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisStoreInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 4, MEM_TRANSACTION_NONE, STORE_FP_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_sfp32_);
    } break;

    case MIPS_SPEC_OP_MASK_SWL:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/SWL]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisPartialStoreInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 4, true, STORE_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_swl_);
    } break;

    case MIPS_SPEC_OP_MASK_SWR:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/SWR]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisPartialStoreInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, 4, false, STORE_INT_REGISTER));
        MIPS_INC_DECODE_STAT(stat_decode_swr_);
    } break;

    case MIPS_SPEC_OP_MASK_SC:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/SC]: -> reg: %" PRIu16 " -> base: %" PRIu16 " + offset=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisStoreConditionalInstruction(
            ins_addr, hw_thr, options_, rs, imm_value_64, rt, rt, 4, STORE_INT_REGISTER, 1, 0));
        MIPS_INC_DECODE_STAT(stat_decode_sc_);
    } break;

    case MIPS_SPEC_OP_MASK_REGIMM:
    {
        const uint64_t offset_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

        #ifdef VANADIS_BUILD_DEBUG
        output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "[decoder/REGIMM] -> imm: %" PRIu64 "\n", offset_value_64);
        output_->verbose(CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "[decoder]        -> rt: 0x%08x\n", (next_ins & MIPS_RT_MASK));
        #endif

        switch ( (next_ins & MIPS_RT_MASK) ) {
        case MIPS_SPEC_OP_MASK_BLTZ:
        {
            bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<int32_t, REG_COMPARE_LT>(
                ins_addr, hw_thr, options_, 4, rs, 0, offset_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
            MIPS_INC_DECODE_STAT(stat_decode_bltz_);
        } break;
        case MIPS_SPEC_OP_MASK_BGEZAL:
        {
            bundle->addInstruction(new VanadisBranchRegCompareImmLinkInstruction<int32_t, REG_COMPARE_GTE>(
                ins_addr, hw_thr, options_, 4, rs, 0, offset_value_64 + 4, (uint16_t) 31,
                VANADIS_SINGLE_DELAY_SLOT));
            MIPS_INC_DECODE_STAT(stat_decode_bgezal_);
        } break;
        case MIPS_SPEC_OP_MASK_BGEZ:
        {
            bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<int32_t, REG_COMPARE_GTE>(
                ins_addr, hw_thr, options_, 4, rs, 0, offset_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
            MIPS_INC_DECODE_STAT(stat_decode_bgez_);
        } break;
        default:
            insert_decode_fault = true;
            break;
        }
    } break;

    case MIPS_SPEC_OP_MASK_LUI:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 16);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/LUI] -> reg: %" PRIu16 " / imm=%" PRId64 "\n", rt, imm_value_64);
        bundle->addInstruction(new VanadisSetRegisterInstruction<int32_t>(ins_addr, hw_thr, options_, rt, imm_value_64));
        MIPS_INC_DECODE_STAT(stat_decode_lui_);
    } break;

    case MIPS_SPEC_OP_MASK_ADDIU:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);
        //output_->verbose(CALL_INFO, 16, 0, "[decoder/ADDIU]: -> reg: %" PRIu16 " rs=%" PRIu16 " / imm=%" PRId64 "\n", rt, rs, imm_value_64);
        bundle->addInstruction(new VanadisAddImmInstruction<int32_t>(ins_addr, hw_thr, options_, rt, rs, imm_value_64));
        MIPS_INC_DECODE_STAT(stat_decode_addiu_);
    } break;

    case MIPS_SPEC_OP_MASK_BEQ:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/BEQ]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64 "\n", rt, rs, imm_value_64 );
        bundle->addInstruction(new VanadisBranchRegCompareInstruction<int32_t, REG_COMPARE_EQ>(
            ins_addr, hw_thr, options_, 4, rt, rs, imm_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
        MIPS_INC_DECODE_STAT(stat_decode_beq_);
    } break;

    case MIPS_SPEC_OP_MASK_BGTZ:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/BGTZ]: -> r1: %" PRIu16 " offset: %" PRId64 "\n", rs, imm_value_64);
        bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<int32_t, REG_COMPARE_GT>(
            ins_addr, hw_thr, options_, 4, rs, 0, imm_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
        MIPS_INC_DECODE_STAT(stat_decode_bgtz_);
    } break;

    case MIPS_SPEC_OP_MASK_BLEZ:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/BLEZ]: -> r1: %" PRIu16 " offset: %" PRId64 "\n", rs, imm_value_64);
        bundle->addInstruction(new VanadisBranchRegCompareImmInstruction<int32_t, REG_COMPARE_LTE>(
            ins_addr, hw_thr, options_, 4, rs, 0, imm_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
        MIPS_INC_DECODE_STAT(stat_decode_blez_);
    } break;

    case MIPS_SPEC_OP_MASK_BNE:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/BNE]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64 "\n", rt, rs, imm_value_64 );
        bundle->addInstruction(new VanadisBranchRegCompareInstruction<int32_t, REG_COMPARE_NEQ>(
            ins_addr, hw_thr, options_, 4, rt, rs, imm_value_64 + 4, VANADIS_SINGLE_DELAY_SLOT));
        MIPS_INC_DECODE_STAT(stat_decode_bne_);
    } break;

    case MIPS_SPEC_OP_MASK_SLTI:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/SLTI]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64 "\n", rt, rs, imm_value_64 );
        bundle->addInstruction(new VanadisSetRegCompareImmInstruction<REG_COMPARE_LT, int32_t>(ins_addr, hw_thr, options_, rt, rs, imm_value_64));
        MIPS_INC_DECODE_STAT(stat_decode_slti_);
    } break;

    case MIPS_SPEC_OP_MASK_SLTIU:
    {
        const int64_t imm_value_64 = vanadis_sign_extend_offset_16(next_ins);

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/SLTIU]: -> r1: %" PRIu16 " r2: %" PRIu16 " offset: %" PRId64 "\n", rt, rs, imm_value_64 );
        bundle->addInstruction(new VanadisSetRegCompareImmInstruction<REG_COMPARE_LT, uint32_t>(ins_addr, hw_thr, options_, rt, rs, imm_value_64));
        MIPS_INC_DECODE_STAT(stat_decode_sltiu_);
    } break;

    case MIPS_SPEC_OP_MASK_ANDI:
    {
        // note - ANDI is zero extended, not sign extended
        const uint64_t imm_value_64 = static_cast<uint64_t>(next_ins & MIPS_IMM_MASK);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/ANDI]: -> %" PRIu16 " <- r2: %" PRIu16 " imm: %" PRIu64 "\n", rt, rs, imm_value_64 );
        bundle->addInstruction(new VanadisAndImmInstruction(ins_addr, hw_thr, options_, rt, rs, imm_value_64));
        MIPS_INC_DECODE_STAT(stat_decode_andi_);
    } break;

    case MIPS_SPEC_OP_MASK_ORI:
    {
        const uint64_t imm_value_64 = static_cast<uint64_t>(next_ins & MIPS_IMM_MASK);

        //output_->verbose(CALL_INFO, 16, 0,
        //"[decoder/ORI]: -> %" PRIu16 " <- r2: %" PRIu16 " imm: %" PRId64 "\n", rt, rs, imm_value_64 );
        bundle->addInstruction(new VanadisOrImmInstruction(ins_addr, hw_thr, options_, rt, rs, imm_value_64));
        MIPS_INC_DECODE_STAT(stat_decode_ori_);
    } break;

    case MIPS_SPEC_OP_MASK_J:
    {
        const uint32_t j_addr_index = (next_ins & MIPS_J_ADDR_MASK) << 2;
        const uint32_t upper_bits   = ((ins_addr + 4) & MIPS_J_UPPER_MASK);

        uint64_t jump_to = 0;
        jump_to += (uint64_t)j_addr_index;
        jump_to |= (uint64_t)upper_bits;

        //output_->verbose(CALL_INFO, 16, 0, "[decoder/J]: -> jump-to: %" PRIu64 " / 0x%0" PRI_ADDR "\n", jump_to, jump_to);

        bundle->addInstruction(
            new VanadisJumpInstruction(ins_addr, hw_thr, options_, 4, jump_to, VANADIS_SINGLE_DELAY_SLOT));
        MIPS_INC_DECODE_STAT(stat_decode_j_);
    } break;

    case MIPS_SPEC_OP_MASK_JAL:
    {
        const uint32_t j_addr_index = (next_ins & MIPS_J_ADDR_MASK) << 2;
        const uint32_t upper_bits   = ((ins_addr + 4) & MIPS_J_UPPER_MASK);

        uint64_t jump_to = 0;
        jump_to          = jump_to + (uint64_t)j_addr_index;
        jump_to          = jump_to + (uint64_t)upper_bits;

        bundle->addInstruction(new VanadisJumpLinkInstruction(ins_addr, hw_thr, options_, 4, 31, jump_to, VANADIS_SINGLE_DELAY_SLOT));
        MIPS_INC_DECODE_STAT(stat_decode_jal_);
    } break;

    case MIPS_SPEC_OP_MASK_XORI:
    {
        const uint64_t xor_mask = static_cast<uint64_t>(next_ins & MIPS_IMM_MASK);
        bundle->addInstruction(new VanadisXorImmInstruction(ins_addr, hw_thr, options_, rt, rs, xor_mask));
        MIPS_INC_DECODE_STAT(stat_decode_xori_);
    } break;

    case MIPS_SPEC_OP_MASK_COP1:
    {
        //output_->verbose(CALL_INFO, 16, 0, "[decode] --> reached co-processor function decoder\n");
        uint16_t fr = 0;
        uint16_t ft = 0;
        uint16_t fs = 0;
        uint16_t fd = 0;

        extract_fp_regs(next_ins, &fr, &ft, &fs, &fd);

        if ( (next_ins & 0x3E30000) == 0x1010000 ) {
            // this decodes to a BRANCH on TRUE
            const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

            bundle->addInstruction(new VanadisBranchFPInstruction(
                ins_addr, hw_thr, options_, 4, MIPS_FP_STATUS_REG, imm_value_64 + 4,
                /* branch on true */ true, VANADIS_SINGLE_DELAY_SLOT));
        }
        else if ( (next_ins & 0x3E30000) == 0x1000000 ) {
            // this decodes to a BRANCH on FALSE
            const int64_t imm_value_64 = vanadis_sign_extend_offset_16_and_shift(next_ins, 2);

            bundle->addInstruction(new VanadisBranchFPInstruction(
                ins_addr, hw_thr, options_, 4, MIPS_FP_STATUS_REG, imm_value_64 + 4,
                /* branch on false */ false, VANADIS_SINGLE_DELAY_SLOT));
        }
        else {
            //output_->verbose(CALL_INFO, 16, 0, "[decoder] ---->
            // decoding function mask: %" PRIu32 " / 0x%x\n", (next_ins & MIPS_FUNC_MASK), (next_ins & MIPS_FUNC_MASK) );
            
            switch ( next_ins & MIPS_FUNC_MASK ) {
            case 0:
            {
                if ( (0 == fd) && (MIPS_SPEC_COP_MASK_MTC == fr) ) {
                    bundle->addInstruction(new VanadisGPR2FPInstruction<uint32_t, uint32_t, true>(ins_addr, hw_thr, options_, fpflags, fs, rt));
                }
                else if ( (0 == fd) && (MIPS_SPEC_COP_MASK_MFC == fr) ) {
                    bundle->addInstruction(new VanadisFP2GPRInstruction<uint32_t, uint32_t, true>(ins_addr, hw_thr, options_, fpflags, rt, fs));
                }
                else if ( (0 == fd) && (MIPS_SPEC_COP_MASK_CF == fr) ) {
                    uint16_t fp_ctrl_reg = 0;
                    bool     fp_matched  = false;

                    switch ( rd ) {
                    case 0:
                        fp_ctrl_reg = MIPS_FP_VER_REG;
                        fp_matched  = true;
                        break;
                    case 31:
                        fp_ctrl_reg = MIPS_FP_STATUS_REG;
                        fp_matched  = true;
                        break;
                    default:
                        break;
                    }

                    if ( fp_matched ) {
                        bundle->addInstruction(new VanadisFP2GPRInstruction<uint32_t, uint32_t, true>(
                            ins_addr, hw_thr, options_, fpflags, rt, fp_ctrl_reg));
                    } else {
                        insert_decode_fault = true;
                    }
                }
                else if ( (0 == fd) && (MIPS_SPEC_COP_MASK_CT == fr) ) {
                    uint16_t fp_ctrl_reg = 0;
                    bool     fp_matched  = false;

                    switch ( rd ) {
                    case 0:
                        fp_ctrl_reg = MIPS_FP_VER_REG;
                        fp_matched  = true;
                        break;
                    case 31:
                        fp_ctrl_reg = MIPS_FP_STATUS_REG;
                        fp_matched  = true;
                        break;
                    default:
                        break;
                    }

                    if ( fp_matched ) {
                        bundle->addInstruction(new VanadisGPR2FPInstruction<uint32_t, uint32_t,true>(
                            ins_addr, hw_thr, options_, fpflags, fp_ctrl_reg, rt));
                    } else {
                        insert_decode_fault = true;
                    }
                }
                else {
                    switch ( fr ) {
                    case 16:
                        bundle->addInstruction(
                            new VanadisFPAddInstruction<float>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                        break;
                    case 17:
                        bundle->addInstruction(
                            new VanadisFPAddInstruction<double>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                        break;
                    case 20:
                        bundle->addInstruction(
                            new VanadisFPAddInstruction<int32_t>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                        break;
                    case 21:
                        bundle->addInstruction(
                            new VanadisFPAddInstruction<int64_t>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                        break;
                    default:
                        insert_decode_fault = true;
                        break;
                    }
                }
            } break;

            case MIPS_SPEC_COP_MASK_MOV:
            {
                // Decide operand format
                switch ( fr ) {
                case 16:
                {
                    bundle->addInstruction(
                        new VanadisFP2FPInstruction<int32_t>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                } break;
                case 17:
                {
                    bundle->addInstruction(
                        new VanadisFP2FPInstruction<int64_t>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                } break;
                default:
                    insert_decode_fault = true;
                    break;
                }

                MIPS_INC_DECODE_STAT(stat_decode_cop1_mov_);
            } break;

            case MIPS_SPEC_COP_MASK_MUL:
            {
                switch ( fr ) {
                case 16:
                    bundle->addInstruction(
                        new VanadisFPMultiplyInstruction<float>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                case 17:
                    bundle->addInstruction(
                        new VanadisFPMultiplyInstruction<double>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                case 20:
                    bundle->addInstruction(
                        new VanadisFPMultiplyInstruction<int32_t>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                case 21:
                    bundle->addInstruction(
                        new VanadisFPMultiplyInstruction<int64_t>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                default:
                    insert_decode_fault = true;
                    break;
                }

                MIPS_INC_DECODE_STAT(stat_decode_cop1_mul_);
            } break;

            case MIPS_SPEC_COP_MASK_DIV:
            {
                switch ( fr ) {
                case 16:
                    bundle->addInstruction(
                        new VanadisFPDivideInstruction<float>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                case 17:
                    bundle->addInstruction(
                        new VanadisFPDivideInstruction<double>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                case 20:
                    bundle->addInstruction(
                        new VanadisFPDivideInstruction<int32_t>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                case 21:
                    bundle->addInstruction(
                        new VanadisFPDivideInstruction<int64_t>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                default:
                    insert_decode_fault = true;
                    break;
                }

                MIPS_INC_DECODE_STAT(stat_decode_cop1_div_);
            } break;

            case MIPS_SPEC_COP_MASK_SUB:
            {
                switch ( fr ) {
                case 16:
                    bundle->addInstruction(
                        new VanadisFPSubInstruction<float>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                case 17:
                    bundle->addInstruction(
                        new VanadisFPSubInstruction<double>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                case 20:
                    bundle->addInstruction(
                        new VanadisFPSubInstruction<int32_t>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                case 21:
                    bundle->addInstruction(
                        new VanadisFPSubInstruction<int64_t>(ins_addr, hw_thr, options_, fpflags, fd, fs, ft));
                    break;
                default:
                    insert_decode_fault = true;
                    break;
                }

                MIPS_INC_DECODE_STAT(stat_decode_cop1_sub_);
            } break;

            case MIPS_SPEC_COP_MASK_CVTS:
            {
                switch ( fr ) {
                case 16:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<float, float>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                case 17:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<double, float>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                case 20:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<int32_t, float>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                case 21:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<int64_t, float>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                default:
                    insert_decode_fault = true;
                    break;
                }

                MIPS_INC_DECODE_STAT(stat_decode_cop1_cvts_);
            } break;

            case MIPS_SPEC_COP_MASK_CVTD:
            {
                switch ( fr ) {
                case 16:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<float, double>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                case 17:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<double, double>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                case 20:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<int32_t, double>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                case 21:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<int64_t, double>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                default:
                    insert_decode_fault = true;
                    break;
                }

                MIPS_INC_DECODE_STAT(stat_decode_cop1_cvtd_);
            } break;

            case MIPS_SPEC_COP_MASK_CVTW:
            {
                switch ( fr ) {
                case 16:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<float, int32_t>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                case 17:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<double, int32_t>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                case 20:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<int32_t, int32_t>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                case 21:
                    bundle->addInstruction(
                        new VanadisFPConvertInstruction<int64_t, int32_t>(ins_addr, hw_thr, options_, fpflags, fd, fs));
                    break;
                default:
                    insert_decode_fault = true;
                    break;
                }

                MIPS_INC_DECODE_STAT(stat_decode_cop1_cvtw_);
            } break;

            case MIPS_SPEC_COP_MASK_CMP_ULT:
            case MIPS_SPEC_COP_MASK_CMP_LT:
            case MIPS_SPEC_COP_MASK_CMP_LTE:
            case MIPS_SPEC_COP_MASK_CMP_EQ:
            {
                // if neither are true, then we have a good decode, otherwise a
                // problem. register 31 is where condition codes and rounding modes are kept
                switch ( fr ) {
                case 16:
                {
                    switch ( next_ins & 0xF ) {
                    case 0x2:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_eq_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, float>(
                            ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    case 0xC:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_lt_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LT, float>(
                            ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    case 0xE:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_lte_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LTE, float>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    default:
                        insert_decode_fault = true;
                        break;
                    }
                } break;
                case 17:
                {
                    switch ( next_ins & 0xF ) {
                    case 0x2:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_eq_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, double>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    case 0xC:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_lt_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LT, double>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    case 0xE:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_lte_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LTE, double>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    case 0x5:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_ult_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_ULT, double>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    default:
                        insert_decode_fault = true;
                        break;
                    }
                } break;
                case 20:
                {
                    switch ( next_ins & 0xF ) {
                    case 0x2:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_eq_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, int32_t>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    case 0xC:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_lt_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LT, int32_t>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    case 0xE:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_lte_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LTE, int32_t>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    default:
                        insert_decode_fault = true;
                        break;
                    }
                } break;
                case 21:
                {
                    switch ( next_ins & 0xF ) {
                    case 0x2:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_eq_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, int64_t>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    case 0xC:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_lt_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_EQ, int64_t>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    case 0xE:
                        MIPS_INC_DECODE_STAT(stat_decode_cop1_lte_);
                        bundle->addInstruction(new VanadisMIPSFPSetRegCompareInstruction<REG_COMPARE_LTE, int64_t>(
                                ins_addr, hw_thr, options_, MIPS_FP_STATUS_REG, fs, ft));
                        break;
                    default:
                        insert_decode_fault = true;
                        break;
                    }
                } break;

                default:
                    insert_decode_fault = true;
                    break;
                }
            } break;
            default:
                insert_decode_fault = true;
                break;
            }
        }
    } break;

    case MIPS_SPEC_OP_SPECIAL3:
    {
        //output_->verbose(CALL_INFO, 16, 0, "[decoder, partial: special3], further decode required...\n");

        switch ( next_ins & 0x3F ) {
        case MIPS_SPEC_OP_MASK_RDHWR:
        {
            const uint16_t target_reg = rt;
            //const uint16_t req_type   = rd;
            //output_->verbose(CALL_INFO, 16, 0, "[decode/RDHWR] target: %" PRIu16 " type: %" PRIu16 "\n", target_reg, req_type);

            switch ( rd ) {
            case 29:
            {
                auto thread_call = std::bind(&VanadisMIPSDecoder::getThreadLocalStoragePointer, this);
                bundle->addInstruction(new VanadisSetRegisterByCallInstruction<int32_t>(ins_addr, hw_thr, options_, target_reg, thread_call));
            } break;
            default:
                insert_decode_fault = true;
                break;
            }
        } break;
        default:
            insert_decode_fault = true;
            break;
        }
    } break;

    default:
        insert_decode_fault = true;
        break;
    }

    if ( insert_decode_fault ) {
        bundle->addInstruction(new VanadisInstructionDecodeFault(ins_addr, hw_thr, options_));
        stat_decode_fault_->addData(1);
    }
    else {
        stat_uop_generated_->addData(bundle->getInstructionCount());
    }

    #ifdef VANADIS_BUILD_DEBUG
    for ( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
        output_->verbose(
            CALL_INFO, 16, VANADIS_DBG_DECODER_FLG, "-> [%3" PRIu32 "]: %s\n", i, bundle->getInstructionByIndex(i)->getInstCode());
    }
    #endif

    // Mark the end of a micro-op group so we can count real instructions and not just micro-ops
    /* Unused
    if ( bundle->getInstructionCount() > 0 ) {
        bundle->getInstructionByIndex(bundle->getInstructionCount() - 1)->markEndOfMicroOpGroup();
    }*/
}
