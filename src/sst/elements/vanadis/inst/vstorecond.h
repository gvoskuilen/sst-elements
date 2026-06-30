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

#ifndef _H_VANADIS_STORE_CONDITIONAL
#define _H_VANADIS_STORE_CONDITIONAL

#include "inst/vstore.h"

namespace SST {
namespace Vanadis {

class VanadisStoreConditionalInstruction : public virtual VanadisStoreInstruction
{
public:
    VanadisStoreConditionalInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t mem_addr_reg,
        const int64_t offset, const uint16_t value_reg, const uint16_t cond_result_reg, const uint16_t store_width,
        VanadisStoreRegisterType reg_type) :
        VanadisInstruction(
            addr, hw_thr, isa_opts,
            reg_type == STORE_INT_REGISTER ? 2 : 1, 1,
            reg_type == STORE_INT_REGISTER ? 2 : 1, 1,
            reg_type == STORE_FP_REGISTER ? 1 : 0, 0,
            reg_type == STORE_FP_REGISTER ? 1 : 0, 0),
        VanadisStoreInstruction(
            addr, hw_thr, isa_opts, mem_addr_reg, offset, value_reg, store_width, MEM_TRANSACTION_LLSC_STORE, reg_type),
            value_success(1), value_failure(0)
    {
        isa_int_regs_out[0] = cond_result_reg;
        isa_int_regs_out_mask_ = (1ULL << cond_result_reg);
    }

    VanadisStoreConditionalInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t mem_addr_reg,
        const int64_t offset, const uint16_t value_reg, const uint16_t cond_result_reg, const uint16_t store_width,
        VanadisStoreRegisterType reg_type, int64_t success_value, int64_t failure_value) :
        VanadisInstruction(
            addr, hw_thr, isa_opts,
            reg_type == STORE_INT_REGISTER ? 2 : 1, 1,
            reg_type == STORE_INT_REGISTER ? 2 : 1, 1,
            reg_type == STORE_FP_REGISTER ? 1 : 0, 0,
            reg_type == STORE_FP_REGISTER ? 1 : 0, 0),
        VanadisStoreInstruction(
            addr, hw_thr, isa_opts, mem_addr_reg, offset, value_reg, store_width, MEM_TRANSACTION_LLSC_STORE, reg_type),
            value_success(success_value), value_failure(failure_value)
    {
        isa_int_regs_out[0] = cond_result_reg;
        isa_int_regs_out_mask_ = (1ULL << cond_result_reg);
    }

    VanadisStoreConditionalInstruction(const VanadisStoreConditionalInstruction& copy_me) :
        VanadisInstruction(copy_me),VanadisStoreInstruction(copy_me), value_success(copy_me.value_success),
        value_failure(copy_me.value_failure)
        {
            ;
        }

    VanadisStoreConditionalInstruction* clone() {
        return new VanadisStoreConditionalInstruction(*this);
    }

    int64_t getResultSuccess() const { return value_success; }
    int64_t getResultFailure() const { return value_failure; }

protected:
    const int64_t value_success;
    const int64_t value_failure;

};

} // namespace Vanadis
} // namespace SST

#endif
