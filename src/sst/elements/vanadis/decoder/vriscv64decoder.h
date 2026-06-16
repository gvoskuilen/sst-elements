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

#ifndef _H_VANADIS_RISCV64_DECODER
#define _H_VANADIS_RISCV64_DECODER

#include "decoder/vdecoder.h"
#include "os/vriscvcpuos.h"

#include <cstdint>
#include <cstring>

#define VANADIS_RISCV_OPCODE_MASK 0x7F
#define VANADIS_RISCV_RD_MASK     0xF80
#define VANADIS_RISCV_RS1_MASK    0xF8000
#define VANADIS_RISCV_RS2_MASK    0x1F00000
#define VANADIS_RISCV_FUNC3_MASK  0x7000
#define VANADIS_RISCV_FUNC7_MASK  0xFE000000
#define VANADIS_RISCV_IMM12_MASK  0xFFF00000
#define VANADIS_RISCV_IMM7_MASK   0xFE000000
#define VANADIS_RISCV_IMM5_MASK   0xF80
#define VANADIS_RISCV_IMM20_MASK  0xFFFFF000

#define VANADIS_RISCV_SIGN12_MASK       0x800
#define VANADIS_RISCV_SIGN12_UPPER_1_32 0xFFFFF000
#define VANADIS_RISCV_SIGN12_UPPER_1_64 0xFFFFFFFFFFFFF000LL

#define AMO_W 0x2
#define AMO_D 0x3

#define LR 0x2
#define SC 0x3
#define AMO_SWAP 0x1
#define AMO_ADD  0x0
#define AMO_XOR  0x4
#define AMO_AND  0xc
#define AMO_OR   0x8
#define AMO_MIN  0x10
#define AMO_MAX  0x14
#define AMO_MINU 0x18
#define AMO_MAXU 0x1c


namespace SST {
namespace Vanadis {

class VanadisRISCV64Decoder : public VanadisDecoder
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        VanadisRISCV64Decoder,
    #ifdef VANADIS_BUILD_DEBUG
        "vanadisdbg",
    #else
        "vanadis",
    #endif
        "VanadisRISCV64Decoder",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Implements a RISCV64-compatible decoder for Vanadis CPU processing.",
        SST::Vanadis::VanadisDecoder)

    SST_ELI_DOCUMENT_PARAMS(
      {"halt_on_decode_fault",
		"Fatal error if a decode fault occurs, used for debugging and not recommmended default is 0 (false)", "0"},
      { "entry_point", "Starting instruction pointer; if not specified (set to 0), "
                      "falls back to the core's ELF reader to discover", "0"})

    VanadisRISCV64Decoder(ComponentId_t id, Params& params, SST::Output* output);

    ~VanadisRISCV64Decoder();

    const char*                  getISAName() const override { return "RISCV64"; }
    uint16_t                     countISAIntReg() const override { return options->countISAIntRegisters(); }
    uint16_t                     countISAFPReg() const override { return options->countISAFPRegisters(); }
    const VanadisDecoderOptions* getDecoderOptions() const override { return options; }
    VanadisFPRegisterMode        getFPRegisterMode() const override { return VANADIS_REGISTER_MODE_FP64; }

    void setStackPointer( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t start_stack_address ) override;

    void setArg1Register( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value ) override;

    virtual void setReturnRegister( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value ) override;

    virtual void setThreadPointer( VanadisISATable* isa_tbl, VanadisRegisterFile* regFile, const uint64_t value ) override;

    bool tick( uint64_t cycle ) override;

protected:
    const VanadisDecoderOptions* options;
    bool                         fatal_decode_fault;

    void decode(const uint64_t ins_address, const uint32_t ins, VanadisInstructionBundle* bundle);

    uint16_t expand_rvc_int_register(const uint16_t reg_in) const { return reg_in + 8; }

    uint16_t extract_rs2_rvc(const uint32_t ins) const { return static_cast<uint16_t>((ins & 0x1C) >> 2); }

    uint16_t extract_rs1_rvc(const uint32_t ins) const { return static_cast<uint16_t>((ins & 0x380) >> 7); }

    uint64_t extract_uimm_rvc_t1(const uint32_t ins) const
    {
        const uint64_t uimm_76 = ((ins & 0x60) << 1);
        const uint64_t uimm_53 = ((ins & 0x1C00) >> 7);

        return (uimm_76 | uimm_53);
    }

    uint64_t extract_uimm_rcv_t2(const uint32_t ins) const
    {
        const uint64_t uimm_6  = ((ins & 0x20) << 1);
        const uint64_t uimm_2  = ((ins & 0x40) >> 4);
        const uint64_t uimm_53 = ((ins & 0x1C00) >> 7);

        return (uimm_6 | uimm_53 | uimm_2);
    }

    uint64_t extract_uimm_rcv_t3(const uint32_t ins) const
    {
        const uint64_t uimm_76 = ((ins & 0x60) << 1);
        const uint64_t uimm_54 = ((ins & 0x1800) >> 6);
        const uint64_t uimm_8  = ((ins & 0x400) >> 2);

        return (uimm_76 | uimm_54 | uimm_8);
    }

    std::string getAMO_type( int type ) {
        switch( type ) {
            case AMO_W: return "W";
            case AMO_D: return "D";
            default: return "???";
        }
    }
    std::string getAMO_name( int amo_op ) {
        switch( amo_op ) {
            case AMO_ADD: return "AMOADD";
            case AMO_XOR: return "AMOXOR";
            case AMO_OR: return "AMOOR";
            case AMO_AND: return "AMOAND";
            case AMO_MIN: return "AMOMIN";
            case AMO_MAX: return "AMOMAX";
            case AMO_MINU: return "AMOMINU";
            case AMO_MAXU: return "AMPOMAXU";
            default: return "???";
        }
    }
    std::string getCSR_name( int func_code ) {
        switch( func_code ) {
            case 0x1: return "CSRRW";
            case 0x2: return "CSRRS";
            case 0x3: return "CSRRC";
            case 0x5: return "CSRRWI";
            case 0x6: return "CSRRSI";
            case 0x7: return "CSRRCI";
            default: return "Unknown instruction";
        }
    }

    uint64_t extract_nzuimm_rcv(const uint32_t ins) const
    {
        const uint64_t imm_3  = ((ins & 0x20) >> 2);
        const uint64_t imm_2  = ((ins & 0x40) >> 4);
        const uint64_t imm_96 = ((ins & 0x780) >> 1);
        const uint64_t imm_54 = ((ins & 0x1800) >> 6);

        return (imm_96 | imm_54 | imm_3 | imm_2);
    }

    uint16_t extract_rd(const uint32_t ins) const { return static_cast<uint16_t>((ins & VANADIS_RISCV_RD_MASK) >> 7); }

    uint16_t extract_rs1(const uint32_t ins) const
    {
        return static_cast<uint16_t>((ins & VANADIS_RISCV_RS1_MASK) >> 15);
    }

    uint16_t extract_rs2(const uint32_t ins) const
    {
        return static_cast<uint16_t>((ins & VANADIS_RISCV_RS2_MASK) >> 20);
    }

    uint32_t extract_func3(const uint32_t ins) const { return ((ins & VANADIS_RISCV_FUNC3_MASK) >> 12); }

    uint32_t extract_func7(const uint32_t ins) const { return ((ins & VANADIS_RISCV_FUNC7_MASK) >> 25); }

    uint32_t extract_opcode(const uint32_t ins) const { return (ins & VANADIS_RISCV_OPCODE_MASK); }

    int32_t extract_imm12(const uint32_t ins) const
    {
        const uint32_t imm12 = (ins & VANADIS_RISCV_IMM12_MASK) >> 20;
        return sign_extend12(imm12);
    }

    int64_t sign_extend12(int64_t value) const
    {
        return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? value : (value | 0xFFFFFFFFFFFFF000LL);
    }

    int64_t sign_extend12(uint64_t value) const
    {
        // If sign at bit-12 is 1, then set all values 31:20 to 1
        return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? static_cast<int64_t>(value)
                                                        : (static_cast<int64_t>(value) | 0xFFFFFFFFFFFFF000LL);
    }

    int32_t sign_extend12(int32_t value) const
    {
        // If sign at bit-12 is 1, then set all values 31:20 to 1
        return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? value : (value | 0xFFFFF000);
    }

    int32_t sign_extend12(uint32_t value) const
    {
        // If sign at bit-12 is 1, then set all values 31:20 to 1
        return (value & VANADIS_RISCV_SIGN12_MASK) == 0 ? static_cast<int32_t>(value)
                                                        : (static_cast<int32_t>(value) | 0xFFFFF000);
    }

    // Extract components for an R-type instruction
    void processR(
        const uint32_t ins, uint32_t& opcode, uint16_t& rd, uint16_t& rs1, uint16_t& rs2, uint32_t& func_code_1,
        uint32_t& func_code_2) const
    {
        opcode      = extract_opcode(ins);
        rd          = extract_rd(ins);
        rs1         = extract_rs1(ins);
        rs2         = extract_rs2(ins);
        func_code_1 = extract_func3(ins);
        func_code_2 = extract_func7(ins);
    }

    // Extract components for an I-type instruction
    template <typename T>
    void processI(const uint32_t ins, uint32_t& opcode, uint16_t& rd, uint16_t& rs1, uint32_t& func_code, T& imm) const
    {
        opcode    = extract_opcode(ins);
        rd        = extract_rd(ins);
        rs1       = extract_rs1(ins);
        func_code = extract_func3(ins);

        // This also performs sign extension which is required in the RISC-V ISA
        int32_t imm_tmp = extract_imm12(ins);
        imm             = static_cast<T>(imm_tmp);
    }

    template <typename T>
    void processS(const uint32_t ins, uint32_t& opcode, uint16_t& rs1, uint16_t& rs2, uint32_t& func_code, T& imm) const
    {
        opcode    = extract_opcode(ins);
        rs1       = extract_rs1(ins);
        rs2       = extract_rs2(ins);
        func_code = extract_func3(ins);

        const int32_t ins_i32 = static_cast<int32_t>(ins);
        imm                   = static_cast<T>(
            sign_extend12(((ins_i32 & VANADIS_RISCV_RD_MASK) >> 7) | ((ins_i32 & VANADIS_RISCV_FUNC7_MASK) >> 20)));
    }

    template <typename T>
    void processU(const uint32_t ins, uint32_t& opcode, uint16_t& rd, T& imm) const
    {
        opcode = extract_opcode(ins);
        rd     = extract_rd(ins);

        const int32_t ins_i32 = static_cast<int32_t>(ins);
        imm                   = static_cast<T>(ins_i32 & 0xFFFFF000);
    }

    template <typename T>
    void processJ(const uint32_t ins, uint32_t& opcode, uint16_t& rd, T& imm) const
    {
        opcode = extract_opcode(ins);
        rd     = extract_rd(ins);

        const uint32_t ins_i32  = static_cast<uint32_t>(ins);
        const uint32_t imm20    = (ins_i32 & 0x80000000) >> 11;
        const uint32_t imm19_12 = (ins_i32 & 0xFF000);
        const uint32_t imm_10_1 = (ins_i32 & 0x7FE00000) >> 20;
        const uint32_t imm11    = (ins_i32 & 0x100000UL) >> 9;

        int32_t imm_tmp = (imm20 | imm19_12 | imm11 | imm_10_1);
        imm_tmp         = (0 == imm20) ? imm_tmp : imm_tmp | 0xFFF00000;

        imm = static_cast<T>(imm_tmp);
    }

    template <typename T>
    void processB(const uint32_t ins, uint32_t& opcode, uint16_t& rs1, uint16_t& rs2, uint32_t& func_code, T& imm) const
    {
        opcode    = extract_opcode(ins);
        rs1       = extract_rs1(ins);
        rs2       = extract_rs2(ins);
        func_code = extract_func3(ins);

        const int32_t ins_i32 = static_cast<int32_t>(ins);

        const int32_t imm_12  = (ins_i32 & 0x80000000) >> 19;
        const int32_t imm_11  = (ins_i32 & 0x80) << 4;
        const int32_t imm_105 = (ins_i32 & 0x7E000000) >> 20;
        const int32_t imm_41  = (ins_i32 & 0xF00) >> 7;

        int32_t imm_tmp = imm_12 | imm_11 | imm_105 | imm_41;

        if ( imm_12 != 0 ) { imm_tmp |= 0xFFFFF000; }

        imm = static_cast<T>(imm_tmp);
    }
};
} // namespace Vanadis
} // namespace SST

#endif
