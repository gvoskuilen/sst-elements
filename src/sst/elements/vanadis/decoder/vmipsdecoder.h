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

#ifndef _H_VANADIS_MIPS_DECODER
#define _H_VANADIS_MIPS_DECODER

#include "decoder/vdecoder.h"

#define MIPS_REG_ZERO 0
#define MIPS_REG_LO   32
#define MIPS_REG_HI   33

#define MIPS_FP_VER_REG    32
#define MIPS_FP_STATUS_REG 33

#define MIPS_OP_MASK 0xFC000000
#define MIPS_RS_MASK 0x3E00000
#define MIPS_RT_MASK 0x1F0000
#define MIPS_RD_MASK 0xF800

#define MIPS_FD_MASK 0x7C0
#define MIPS_FS_MASK 0xF800
#define MIPS_FT_MASK 0x1F0000
#define MIPS_FR_MASK 0x3E00000

#define MIPS_ADDR_MASK    0x7FFFFFF
#define MIPS_J_ADDR_MASK  0x3FFFFFF
#define MIPS_J_UPPER_MASK 0xF0000000
#define MIPS_IMM_MASK     0xFFFF
#define MIPS_SHFT_MASK    0x7C0
#define MIPS_FUNC_MASK    0x3F

#define MIPS_SPEC_OP_SPECIAL3 0x7c000000

#define MIPS_SPECIAL_OP_MASK 0x7FF

#define MIPS_SPEC_OP_MASK_ADD  0x20
#define MIPS_SPEC_OP_MASK_ADDU 0x21
#define MIPS_SPEC_OP_MASK_AND  0x24

#define MIPS_SPEC_OP_MASK_ANDI   0x30000000
#define MIPS_SPEC_OP_MASK_ORI    0x34000000
#define MIPS_SPEC_OP_MASK_REGIMM 0x04000000
#define MIPS_SPEC_OP_MASK_BGEZAL 0x00110000
#define MIPS_SPEC_OP_MASK_BGTZ   0x1C000000
#define MIPS_SPEC_OP_MASK_LUI    0x3C000000
#define MIPS_SPEC_OP_MASK_ADDIU  0x24000000
#define MIPS_SPEC_OP_MASK_LB     0x80000000
#define MIPS_SPEC_OP_MASK_LBU    0x90000000
#define MIPS_SPEC_OP_MASK_LL     0xC0000000
#define MIPS_SPEC_OP_MASK_LW     0x8C000000
#define MIPS_SPEC_OP_MASK_LWL    0x88000000
#define MIPS_SPEC_OP_MASK_LWR    0x98000000
#define MIPS_SPEC_OP_MASK_LH     0x84000000
#define MIPS_SPEC_OP_MASK_LHU    0x94000000
#define MIPS_SPEC_OP_MASK_SB     0xA0000000
#define MIPS_SPEC_OP_MASK_SC     0xE0000000
#define MIPS_SPEC_OP_MASK_SH     0xA4000000
#define MIPS_SPEC_OP_MASK_SW     0xAC000000
#define MIPS_SPEC_OP_MASK_SWL    0xA8000000
#define MIPS_SPEC_OP_MASK_SWR    0xB8000000
#define MIPS_SPEC_OP_MASK_BEQ    0x10000000
#define MIPS_SPEC_OP_MASK_BNE    0x14000000
#define MIPS_SPEC_OP_MASK_BLEZ   0x18000000
#define MIPS_SPEC_OP_MASK_SLTI   0x28000000
#define MIPS_SPEC_OP_MASK_SLTIU  0x2C000000
#define MIPS_SPEC_OP_MASK_JAL    0x0C000000
#define MIPS_SPEC_OP_MASK_J      0x08000000
#define MIPS_SPEC_OP_MASK_COP1   0x44000000
#define MIPS_SPEC_OP_MASK_XORI   0x38000000

#define MIPS_SPEC_OP_MASK_SFP32 0xE4000000
#define MIPS_SPEC_OP_MASK_LFP32 0xC4000000

#define MIPS_SPEC_OP_MASK_BLTZ    0x0
#define MIPS_SPEC_OP_MASK_BGEZ    0x10000
#define MIPS_SPEC_OP_MASK_BREAK   0x0D
#define MIPS_SPEC_OP_MASK_DADD    0x2C
#define MIPS_SPEC_OP_MASK_DADDU   0x2D
#define MIPS_SPEC_OP_MASK_DIV     0x1A
#define MIPS_SPEC_OP_MASK_DIVU    0x1B
#define MIPS_SPEC_OP_MASK_DDIV    0x1E
#define MIPS_SPEC_OP_MASK_DDIVU   0x1F
#define MIPS_SPEC_OP_MASK_DMULT   0x1C
#define MIPS_SPEC_OP_MASK_DMULTU  0x1D
#define MIPS_SPEC_OP_MASK_DSLL    0x38
#define MIPS_SPEC_OP_MASK_DSLL32  0x3C
#define MIPS_SPEC_OP_MASK_DSLLV   0x14
#define MIPS_SPEC_OP_MASK_DSRA    0x3B
#define MIPS_SPEC_OP_MASK_DSRA32  0x3F
#define MIPS_SPEC_OP_MASK_DSRAV   0x17
#define MIPS_SPEC_OP_MASK_DSRL    0x3A
#define MIPS_SPEC_OP_MASK_DSRL32  0x3E
#define MIPS_SPEC_OP_MASK_DSRLV   0x16
#define MIPS_SPEC_OP_MASK_DSUB    0x2E
#define MIPS_SPEC_OP_MASK_DSUBU   0x2F
#define MIPS_SPEC_OP_MASK_JALR    0x09
#define MIPS_SPEC_OP_MASK_JR      0x08
#define MIPS_SPEC_OP_MASK_MFHI    0x10
#define MIPS_SPEC_OP_MASK_MFLO    0x12
#define MIPS_SPEC_OP_MASK_MOVN    0x0B
#define MIPS_SPEC_OP_MASK_MOVZ    0x0A
#define MIPS_SPEC_OP_MASK_MTHI    0x11
#define MIPS_SPEC_OP_MASK_MTLO    0x13
#define MIPS_SPEC_OP_MASK_MULT    0x18
#define MIPS_SPEC_OP_MASK_MULTU   0x19
#define MIPS_SPEC_OP_MASK_NOR     0x27
#define MIPS_SPEC_OP_MASK_OR      0x25
#define MIPS_SPEC_OP_MASK_RDHWR   0x3B
#define MIPS_SPEC_OP_MASK_SLL     0x00
#define MIPS_SPEC_OP_MASK_SLLV    0x04
#define MIPS_SPEC_OP_MASK_SLT     0x2A
#define MIPS_SPEC_OP_MASK_SLTU    0x2B
#define MIPS_SPEC_OP_MASK_SRA     0x03
#define MIPS_SPEC_OP_MASK_SRAV    0x07
#define MIPS_SPEC_OP_MASK_SRL     0x02
#define MIPS_SPEC_OP_MASK_SRLV    0x06
#define MIPS_SPEC_OP_MASK_SUB     0x22
#define MIPS_SPEC_OP_MASK_SUBU    0x23
#define MIPS_SPEC_OP_MASK_SYSCALL 0x0C
#define MIPS_SPEC_OP_MASK_SYNC    0x0F
#define MIPS_SPEC_OP_MASK_XOR     0x26

#define MIPS_SPEC_COP_MASK_CF   0x2
#define MIPS_SPEC_COP_MASK_CT   0x6
#define MIPS_SPEC_COP_MASK_MFC  0x0
#define MIPS_SPEC_COP_MASK_MTC  0x4
#define MIPS_SPEC_COP_MASK_MOV  0x6
#define MIPS_SPEC_COP_MASK_CVTS 0x20
#define MIPS_SPEC_COP_MASK_CVTD 0x21
#define MIPS_SPEC_COP_MASK_CVTW 0x24
#define MIPS_SPEC_COP_MASK_MUL  0x2
#define MIPS_SPEC_COP_MASK_ADD  0x0
#define MIPS_SPEC_COP_MASK_SUB  0x1
#define MIPS_SPEC_COP_MASK_DIV  0x3

#define MIPS_SPEC_COP_MASK_CMP_LT  0x3C
#define MIPS_SPEC_COP_MASK_CMP_LTE 0x3E
#define MIPS_SPEC_COP_MASK_CMP_EQ  0x32
#define MIPS_SPEC_COP_MASK_CMP_ULT 0x35

#define MIPS_INC_DECODE_STAT(stat_name) (stat_name)->addData(1);

namespace SST {
namespace Vanadis {

class VanadisInstructionBundle;
class VanadisISATable;
class VanadisRegisterFile;

class VanadisMIPSDecoder : public VanadisDecoder
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        VanadisMIPSDecoder,
    #ifdef VANADIS_BUILD_DEBUG
        "vanadisdbg",
    #else
        "vanadis",
    #endif
        "VanadisMIPSDecoder",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Implements a MIPS-compatible decoder for Vanadis CPU processing.",
        SST::Vanadis::VanadisDecoder)

    SST_ELI_DOCUMENT_PARAMS({ "entry_point", "Starting instruction pointer; if not specified (set to 0), "
                                             "falls back to the core's ELF reader to discover", "0"})

    SST_ELI_DOCUMENT_STATISTICS(
	    VANADIS_DECODER_ELI_STATISTICS,
	    { "ins_decode_add", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_addu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_and", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_dadd", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_daddu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_ddiv", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_div", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_divu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_dmult", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_dmultu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_dsllv", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_dsrav", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_dsrlv", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_dsub", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_dsubu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_jr", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_jalr", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_mfhi", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_mflo", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_mult", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_multu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_nor", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_or", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sllv", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_slt", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sltu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_srav", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_srlv", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sub", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_subu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_syscall", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sync", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_xor", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sll", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_srl", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sra", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_bltz", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_bgezal", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_bgez", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_lui", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_lb", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_lbu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_lhu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_lh", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_lw", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_lfp32", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_ll", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_lwl", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_lwr", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sb", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sc", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sw", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sh", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sfp32", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_swr", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_swl", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_addiu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_beq", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_bgtz", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_blez", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_bne", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_slti", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_sltiu", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_andi", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_ori", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_j", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_jal", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_xori", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_rdhwr", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_mtc", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_mfc", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_cf", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_ct", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_mov", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_mul", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_div", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_sub", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_cvts", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_cvtd", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_cvtw", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_lt", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_ult", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_lte", "Count number of instructions decoded", "ins", 1 },
        { "ins_decode_cop1_eq", "Count number of instructions decoded", "ins", 1 }
	)

    VanadisMIPSDecoder(ComponentId_t id, Params& params, SST::Output* output);

    ~VanadisMIPSDecoder();

    virtual const char*                  getISAName() const { return "MIPS"; }
    virtual uint16_t                     countISAIntReg() const { return options->countISAIntRegisters(); }
    virtual uint16_t                     countISAFPReg() const { return options->countISAFPRegisters(); }
    virtual const VanadisDecoderOptions* getDecoderOptions() const { return options; }

    virtual VanadisFPRegisterMode getFPRegisterMode() const { return VANADIS_REGISTER_MODE_FP32; }

    void setStackPointer( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t start_stack_address ) override;

    void setArg1Register( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value ) override;

    void setFuncPointer( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value ) override;

    void setReturnRegister( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value ) override;

    void setSuccessRegister( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value ) override;

    bool tick( uint64_t cycle ) override;

private:

    void extract_imm(const uint32_t ins, uint32_t* imm) const { (*imm) = (ins & MIPS_IMM_MASK); }

    void extract_three_regs(const uint32_t ins, uint16_t* rt, uint16_t* rs, uint16_t* rd) const
    {
    (*rt) = (ins & MIPS_RT_MASK) >> 16;
    (*rs) = (ins & MIPS_RS_MASK) >> 21;
    (*rd) = (ins & MIPS_RD_MASK) >> 11;
    }

    void extract_fp_regs(const uint32_t ins, uint16_t* fr, uint16_t* ft, uint16_t* fs, uint16_t* fd) const
    {
    (*fr) = (ins & MIPS_FR_MASK) >> 21;
    (*ft) = (ins & MIPS_FT_MASK) >> 16;
    (*fs) = (ins & MIPS_FS_MASK) >> 11;
    (*fd) = (ins & MIPS_FD_MASK) >> 6;
    }

    void decode( const uint64_t ins_addr, const uint32_t next_ins, VanadisInstructionBundle* bundle );

    const VanadisDecoderOptions* options;

    bool haltOnDecodeZero;

    Statistic<uint64_t>* stat_decode_add;
    Statistic<uint64_t>* stat_decode_addu;
    Statistic<uint64_t>* stat_decode_and;
    Statistic<uint64_t>* stat_decode_dadd;
    Statistic<uint64_t>* stat_decode_daddu;
    Statistic<uint64_t>* stat_decode_ddiv;
    Statistic<uint64_t>* stat_decode_div;
    Statistic<uint64_t>* stat_decode_divu;
    Statistic<uint64_t>* stat_decode_dmult;
    Statistic<uint64_t>* stat_decode_dmultu;
    Statistic<uint64_t>* stat_decode_dsllv;
    Statistic<uint64_t>* stat_decode_dsrav;
    Statistic<uint64_t>* stat_decode_dsrlv;
    Statistic<uint64_t>* stat_decode_dsub;
    Statistic<uint64_t>* stat_decode_dsubu;
    Statistic<uint64_t>* stat_decode_jr;
    Statistic<uint64_t>* stat_decode_jalr;
    Statistic<uint64_t>* stat_decode_mfhi;
    Statistic<uint64_t>* stat_decode_mflo;
    Statistic<uint64_t>* stat_decode_mult;
    Statistic<uint64_t>* stat_decode_multu;
    Statistic<uint64_t>* stat_decode_nor;
    Statistic<uint64_t>* stat_decode_or;
    Statistic<uint64_t>* stat_decode_sllv;
    Statistic<uint64_t>* stat_decode_slt;
    Statistic<uint64_t>* stat_decode_sltu;
    Statistic<uint64_t>* stat_decode_srav;
    Statistic<uint64_t>* stat_decode_srlv;
    Statistic<uint64_t>* stat_decode_sub;
    Statistic<uint64_t>* stat_decode_subu;
    Statistic<uint64_t>* stat_decode_syscall;
    Statistic<uint64_t>* stat_decode_sync;
    Statistic<uint64_t>* stat_decode_xor;
    Statistic<uint64_t>* stat_decode_sll;
    Statistic<uint64_t>* stat_decode_srl;
    Statistic<uint64_t>* stat_decode_sra;
    Statistic<uint64_t>* stat_decode_bltz;
    Statistic<uint64_t>* stat_decode_bgezal;
    Statistic<uint64_t>* stat_decode_bgez;
    Statistic<uint64_t>* stat_decode_lui;
    Statistic<uint64_t>* stat_decode_lb;
    Statistic<uint64_t>* stat_decode_lbu;
    Statistic<uint64_t>* stat_decode_lhu;
    Statistic<uint64_t>* stat_decode_lh;
    Statistic<uint64_t>* stat_decode_lw;
    Statistic<uint64_t>* stat_decode_lfp32;
    Statistic<uint64_t>* stat_decode_ll;
    Statistic<uint64_t>* stat_decode_lwl;
    Statistic<uint64_t>* stat_decode_lwr;
    Statistic<uint64_t>* stat_decode_sb;
    Statistic<uint64_t>* stat_decode_sc;
    Statistic<uint64_t>* stat_decode_sw;
    Statistic<uint64_t>* stat_decode_sh;
    Statistic<uint64_t>* stat_decode_sfp32;
    Statistic<uint64_t>* stat_decode_swr;
    Statistic<uint64_t>* stat_decode_swl;
    Statistic<uint64_t>* stat_decode_addiu;
    Statistic<uint64_t>* stat_decode_beq;
    Statistic<uint64_t>* stat_decode_bgtz;
    Statistic<uint64_t>* stat_decode_blez;
    Statistic<uint64_t>* stat_decode_bne;
    Statistic<uint64_t>* stat_decode_slti;
    Statistic<uint64_t>* stat_decode_sltiu;
    Statistic<uint64_t>* stat_decode_andi;
    Statistic<uint64_t>* stat_decode_ori;
    Statistic<uint64_t>* stat_decode_j;
    Statistic<uint64_t>* stat_decode_jal;
    Statistic<uint64_t>* stat_decode_xori;
    Statistic<uint64_t>* stat_decode_rdhwr;
    Statistic<uint64_t>* stat_decode_cop1_mtc;
    Statistic<uint64_t>* stat_decode_cop1_mfc;
    Statistic<uint64_t>* stat_decode_cop1_cf;
    Statistic<uint64_t>* stat_decode_cop1_ct;
    Statistic<uint64_t>* stat_decode_cop1_mov;
    Statistic<uint64_t>* stat_decode_cop1_mul;
    Statistic<uint64_t>* stat_decode_cop1_div;
    Statistic<uint64_t>* stat_decode_cop1_sub;
    Statistic<uint64_t>* stat_decode_cop1_cvts;
    Statistic<uint64_t>* stat_decode_cop1_cvtd;
    Statistic<uint64_t>* stat_decode_cop1_cvtw;
    Statistic<uint64_t>* stat_decode_cop1_lt;
    Statistic<uint64_t>* stat_decode_cop1_ult;
    Statistic<uint64_t>* stat_decode_cop1_lte;
    Statistic<uint64_t>* stat_decode_cop1_eq;
};

} // namespace Vanadis
} // namespace SST

#endif
