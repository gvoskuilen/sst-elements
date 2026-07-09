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

#ifndef _H_VANADIS_REG_FILE
#define _H_VANADIS_REG_FILE


#include "decoder/visaopts.h"
#include "inst/fpregmode.h"

#include <cstring>
#include <sst/core/output.h>
#include <sst/core/sst_types.h>

namespace SST {
namespace Vanadis {

class VanadisRegisterFile
{

public:
    VanadisRegisterFile(
        const uint32_t thr, const VanadisDecoderOptions* decoder_ots, const uint16_t int_regs, const uint16_t fp_regs,
        const VanadisFPRegisterMode fp_rmode, SST::Output* logger) :
        hw_thread_(thr),
        count_int_regs_(int_regs),
        count_fp_regs_(fp_regs),
        decoder_opts_(decoder_ots),
		int_reg_width_(8),
		fp_reg_width_( (fp_rmode == VANADIS_REGISTER_MODE_FP32) ? 4 : 8 )
    {
        // Int registers are always 64-bits

        int_reg_storage_ = new char[int_reg_width_ * count_int_regs_];
        fp_reg_storage_ = new char[fp_reg_width_ * count_fp_regs_];

        reset();

        output_ = logger;
    }

    ~VanadisRegisterFile()
    {
        delete[] int_reg_storage_;
        delete[] fp_reg_storage_;
    }

    void reset()
    {
        std::memset(int_reg_storage_, 0, (int_reg_width_ * count_int_regs_));
        std::memset(fp_reg_storage_, 0, (fp_reg_width_ * count_fp_regs_));
    }

    const VanadisDecoderOptions* getDecoderOptions() const { return decoder_opts_; }

    uint32_t getIntRegWidth() const {
        return int_reg_width_;
    }

    uint32_t getFPRegWidth() const {
        return fp_reg_width_;
    }

    void copyFromRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len, bool is_fp) {
        if(is_fp) {
            copyFromFPRegister(reg, offset, values, len);
        } else {
            copyFromIntRegister(reg, offset, values, len);
        }
    }

    void copyFromFPRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len) {
        assert(reg < count_fp_regs_);
        assert((offset + len) <= fp_reg_width_);
        int index = get_reg_index(reg,1);
        uint8_t* reg_ptr = (uint8_t*) &fp_reg_storage_[index];

        for(auto i = 0; i < len; ++i) {
            values[i] = reg_ptr[offset + i];
        }
    }

    void copyFromIntRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len) {
        assert(reg < count_int_regs_);
        assert((offset + len) <= int_reg_width_);
        int index = get_reg_index(reg, 0);
        uint8_t* reg_ptr = (uint8_t*) &int_reg_storage_[index];

        for(auto i = 0; i < len; ++i) {
            values[i] = reg_ptr[offset + i];
        }

    }

    void copyToIntRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len) {
        assert((offset + len) <= int_reg_width_);
        assert(reg < count_int_regs_);
        int index = get_reg_index(reg, 0);
        uint8_t* reg_ptr = (uint8_t*) &int_reg_storage_[index];
        for(auto i = 0; i < len; ++i) {
            reg_ptr[offset + i] = values[i];
        }
    }

    void copyToFPRegister(uint16_t reg, uint32_t offset, uint8_t* values, uint32_t len) {
        assert((offset + len) <= fp_reg_width_);
        assert(reg < count_fp_regs_);
        int index = get_reg_index(reg, 1);
        uint8_t* reg_ptr = (uint8_t*) &fp_reg_storage_[index];
        for(auto i = 0; i < len; ++i) {
            reg_ptr[offset + i] = values[i];
        }
    }

    template <typename T>
    T getIntReg(const uint16_t reg)
    {
        assert(reg < count_int_regs_);
        assert(sizeof(T) <= int_reg_width_);

        if ( reg != decoder_opts_->getRegisterIgnoreWrites() )
        {
            int index = get_reg_index(reg, 0);
            char* reg_start = &int_reg_storage_[index];
            T*    reg_start_T = (T*)reg_start;
            return *(reg_start_T);
        }
        else {
            return T();
        }
    }

    template <typename T>
    T getFPReg(const uint16_t reg)
    {
        assert(reg < count_fp_regs_);
        assert(sizeof(T) <= fp_reg_width_);
        int index = get_reg_index(reg, 1);
        char* reg_start   = &fp_reg_storage_[index];
        T*    reg_start_T = (T*)reg_start;
        return *(reg_start_T);
    }

    template <typename T>
    void setIntReg(const uint16_t reg, const T val, const bool sign_extend = true)
    {
        assert(reg < count_int_regs_);

        if ( LIKELY(reg != decoder_opts_->getRegisterIgnoreWrites()) ) {
            int index = get_reg_index(reg, 0);
            T*    reg_ptr_t = (T*)(&int_reg_storage_[index]);
            char* reg_ptr_c = (char*)(reg_ptr_t);

            reg_ptr_t[0] = val;

            // if we need to sign extend, check if the most-significant bit is a 1, if yes then
            // fill with 0xFF, otherwise fill with 0x00
            std::memset(
                &reg_ptr_c[sizeof(T)],
                sign_extend ? ((val & (static_cast<T>(1) << (sizeof(T) * 8 - 1))) == 0) ? 0x00 : 0xFF : 0x00,
                int_reg_width_ - sizeof(T));
        }
    }

    template <typename T>
    void setFPReg(const uint16_t reg, const T val)
    {
        assert(reg < count_fp_regs_);
        assert(sizeof(T) <= fp_reg_width_);

        uint8_t* val_ptr = (uint8_t*) &val;
        int index = get_reg_index(reg, 1);
        for(auto i = 0; i < sizeof(T); ++i) {
            fp_reg_storage_[index + i] = val_ptr[i];
        }

        // Pad with extra zeros if needed
        for(auto i = sizeof(T); i < fp_reg_width_; ++i) {
            fp_reg_storage_[index + i] = 0;
        }
    }


    uint32_t getHWThread() const { return hw_thread_; }
    uint16_t getThreadCount() const { return thread_count_; }
    void setThreadCount(uint16_t threads) { thread_count_ = threads; }
    uint16_t countIntRegs() const { return count_int_regs_; }
    uint16_t countFPRegs() const { return count_fp_regs_; }

    void print(SST::Output* output, int level = 8 )
    {
        output->verbose(CALL_INFO, level, 0, "Integer Registers:\n");

        for ( uint16_t i = 0; i < count_int_regs_; ++i ) {
            printRegister(output, true, i, level);
        }

        output->verbose(CALL_INFO, level, 0, "Floating Point Registers:\n");

        for ( uint16_t i = 0; i < count_fp_regs_; ++i ) {
            printRegister(output, false, i, level);
        }
    }

private:
    char* getIntReg(const uint16_t reg)
    {
        assert(reg < count_int_regs_);
        int index = get_reg_index(reg, 0);
        return int_reg_storage_ + (index);
    }

    char* getFPReg(const uint16_t reg)
    {
        assert(reg < count_fp_regs_);
        int index = get_reg_index(reg, 1);
        return fp_reg_storage_ + (index);
    }

    void printRegister(SST::Output* output, bool isInt, uint16_t reg, int level = 8)
    {
        char* ptr = NULL;

        if ( isInt ) { ptr = getIntReg(reg); }
        else {
            ptr = getFPReg(reg);
        }

        char* val_string = new char[65];
        val_string[64]   = '\0';
        int index        = 0;

        const long long int v = ((long long int*)ptr)[0];

        for( auto i = 0; i < 64; ++i) {
            val_string[i] = '0';
        }

        for ( unsigned long long int i = 1L << (isInt ? ((int_reg_width_ * 8) - 1) : ((fp_reg_width_ * 8) - 1)); i > 0; i = i / 2 ) {
            val_string[index++] = (v & i) ? '1' : '0';
        }

        output->verbose(CALL_INFO, level, 0, "R[%5" PRIu16 "]: %s\n", reg, val_string);
        delete[] val_string;
    }

    int get_reg_index(uint16_t reg, bool is_fp)
    {
        int index = 0;
        if(is_fp) {
            index = fp_reg_width_ * reg;
        } else {
            index = int_reg_width_ * reg;
        }
        return index;

    }
    const uint32_t               hw_thread_;
    const uint16_t               count_int_regs_;
    const uint16_t               count_fp_regs_;
    const VanadisDecoderOptions* decoder_opts_;

    // Actual storage for register contents
    char* int_reg_storage_;
    char* fp_reg_storage_;

    const uint32_t        fp_reg_width_;  // Number of bytes per fp reg
    const uint32_t        int_reg_width_; // Number of bytes per int reg

    SST::Output* output_;
    uint16_t thread_count_;

};

} // namespace Vanadis
} // namespace SST

#endif
