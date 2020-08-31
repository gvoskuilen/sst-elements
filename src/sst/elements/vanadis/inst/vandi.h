
#ifndef _H_VANADIS_ANDI
#define _H_VANADIS_ANDI

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisAndImmInstruction : public VanadisInstruction {
public:
	VanadisAndImmInstruction(
		const uint64_t id,
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src_1,
		const int64_t immediate) :
		VanadisInstruction(id, addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_out[0] = dest;

		imm_value = immediate;
	}

	VanadisAndImmInstruction* clone() {
		return new VanadisAndImmInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		return "ANDI";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "ANDI    %5" PRIu16 " <- %5" PRIu16 " + imm=%" PRId64 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %" PRId64 ")",
			isa_int_regs_out[0], isa_int_regs_in[0], imm_value,
			phys_int_regs_out[0], phys_int_regs_in[0], imm_value );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) ANDI phys: out=%" PRIu16 " in=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16 " / in=%" PRIu16 "\n",
			(void*) getInstructionAddress(), phys_int_regs_out[0],
			phys_int_regs_in[0], imm_value,
			isa_int_regs_out[0], isa_int_regs_in[0] );

		int64_t* dest  = (int64_t*) regFile->getIntReg( phys_int_regs_out[0] );
		int64_t* src_1 = (int64_t*) regFile->getIntReg( phys_int_regs_in[0]  );

		regFile->setIntReg( phys_int_regs_out[0], (*src_1) & imm_value );

		markExecuted();
	}

private:
	int64_t imm_value;

};

}
}

#endif
