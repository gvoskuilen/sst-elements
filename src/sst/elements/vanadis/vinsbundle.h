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

#ifndef _H_VANADIS_INST_BUNDLE
#define _H_VANADIS_INST_BUNDLE

#include <cinttypes>
#include <cstdint>
#include <vector>

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionBundle {

public:
    VanadisInstructionBundle(const uint64_t addr) : ins_addr_(addr), pc_inc_(4) { inst_bundle_.reserve(1); }

    ~VanadisInstructionBundle() { clear(); }

    void clear() {
        for (VanadisInstruction* next_ins : inst_bundle_) {
            delete next_ins;
        }

        inst_bundle_.clear();
    }

    uint32_t getInstructionCount() const { return inst_bundle_.size(); }

    void addInstruction(VanadisInstruction* ins) {
        inst_bundle_.push_back(ins);
    }

    VanadisInstruction* getInstructionByIndex(const uint32_t index) {
        return inst_bundle_[index];
    }

    uint64_t getInstructionAddress() const { return ins_addr_; }
    uint64_t pcIncrement() const { return pc_inc_; }
    void setPCIncrement(uint64_t new_pc_inc) { pc_inc_ = new_pc_inc; }

private:
    const uint64_t ins_addr_;
    uint64_t pc_inc_;
    std::vector<VanadisInstruction*> inst_bundle_;
};

} // namespace Vanadis
} // namespace SST

#endif
