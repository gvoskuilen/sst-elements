// Copyright 2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2020 NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/* 

Fault file specification:

<FAULT> ::= <WHERE> <WHEN> <WHAT> <EOL>

#String denoting the location of the fault. E.g. ALU, MDU, etc...
<WHERE> ::= "ALU" | "MDU" | etc...

# Specify conditions for it to occur: Either by event or by cycle
<WHEN> ::= <TRIGGER> #
# trigger is case insensitive
<TRIGGER> ::= "Cycle" | "Event"      

# bitmask of bits to flip
<WHAT> = <bitmask>

#Note: Bitmask and trigger number can be specified as a decimal (1234)
 or hex (0x4d2). Hex requires the "0x" prefix"

# Examples

# At clock cycle 37, flip the least significant bit of the ALU output
ALU Cycle 37 0x00000001  
# At the 4th time we use the MDU, flip all bits of the output
MDU Event 4 0xffffffff

 */

/* To add new fault

faults.h
   ~51 
   convenience func
faults.cc
   ~71 (parse map)
   ~114(Genf time)
   check_for_fault
   convenience func
   printStats
reg.h
   ~74
cycle-cl or as needed
 */


#include <sst_config.h>
#include <algorithm>
#include "faults.h"
#include <fstream>
#include <sst/core/rng/mersenne.h>
#include "mips_4kc.h"

using namespace SST;
using namespace SST::RNG;
using namespace SST::MIPS4KCComponent;
using namespace faultTrack;

const std::map<std::string, faultChecker_t::location_idx_t> faultChecker_t::parseMap = {
    {"RF", RF_FAULT_IDX},
    {"ID", ID_FAULT_IDX},
    {"MDU", MDU_FAULT_IDX},
    {"MEM_PRE_ADDR", MEM_PRE_ADDR_FAULT_IDX},
    {"MEM_POST", MEM_POST_FAULT_IDX},
    {"WB", WB_FAULT_IDX},
    {"ALU", ALU_FAULT_IDX},
    {"MEM_BP",MEM_BP_FAULT_IDX},
    {"CONTROL",CONTROL_FAULT_IDX},
    {"INST_ADDR",INST_ADDR_FAULT_IDX},
    {"INST_TYPE",INST_TYPE_FAULT_IDX},
    {"WB_ADDR",WB_ADDR_FAULT_IDX},
    {"MEM_PRE_DATA", MEM_PRE_DATA_FAULT_IDX},
    {"PC", PC_FAULT_IDX}
};

void faultChecker_t::init(faultTrack::location_t loc, uint64_t period,
                          string fault_file, bool _fault_by_time,
                          uint32_t _bitsToFlip,
                          uint32_t seed, Output *Out)  {
    fault_by_time = _fault_by_time;
    bitsToFlip = _bitsToFlip;
    out = Out;

    //init RNG
    if (seed != 0) {
        rng = new MersenneRNG(seed);
    } else {
        rng = new MersenneRNG();
    }

    // "Old Style" (non-file) fault injection
    faultTrack::location_t locations = loc;
    printf("Fault Injector: Inject faults at 0x%x\n", locations);
    printf("Fault Injector: Bitmask 0x%x\n", bitsToFlip);


    for (int i = NO_LOC_FAULT_IDX; i < LAST_FAULT_IDX; ++i) {
        event_count[i] = 0;
    }

    // randomly generate when different faults should occur
#define GEN_F_TIME(FAULT) \
    if (locations & FAULT) {                                            \
        faultTime[FAULT##_IDX] = rng->generateNextUInt64() % period;     \
        printf(" Will Inject %s at %lld\n", #FAULT ,faultTime[FAULT##_IDX]); \
    } else {                                                            \
        faultTime[FAULT##_IDX] = -1;                                     \
    }

    GEN_F_TIME(RF_FAULT);
    GEN_F_TIME(ID_FAULT);
    GEN_F_TIME(MDU_FAULT);
    GEN_F_TIME(MEM_PRE_ADDR_FAULT);
    GEN_F_TIME(MEM_POST_FAULT);
    GEN_F_TIME(WB_FAULT);
    GEN_F_TIME(ALU_FAULT);
    GEN_F_TIME(MEM_BP_FAULT);
    GEN_F_TIME(CONTROL_FAULT);
    GEN_F_TIME(INST_ADDR_FAULT);
    GEN_F_TIME(INST_TYPE_FAULT);
    GEN_F_TIME(WB_ADDR_FAULT);
    GEN_F_TIME(MEM_PRE_DATA_FAULT);
    GEN_F_TIME(PC_FAULT);

#undef GEN_F_TIME

    // "New Sytle" (file-based) faultsread in file of faults
    if (fault_file != "") {
        readFaultFile(fault_file);
    }
}

// take a string, return its location index or LAST_FAULT_IDX for error
faultChecker_t::location_idx_t faultChecker_t::findLocation(const string& loc) {
    const auto &iter = parseMap.find(loc);
    if (iter != parseMap.end()) {
        return iter->second;
    } else {
        return LAST_FAULT_IDX;
    }
}

// read in one line, add to appropriate 
bool faultChecker_t::readFaultFileLine(ifstream &in, const int lineNum) {

    string firstWord;
    in >> firstWord;

    if (in.good()) {
        if (firstWord.find("#") != std::string::npos) {
            // comment line
            string commentLine;
            getline(in, commentLine);
            printf("COMMENT: %s\n", commentLine.c_str());
        } else {
            // regular fault line
            location_idx_t loc = findLocation(firstWord);
            printf("Fault Loc %s (%d), ", firstWord.c_str(), loc);
            if (loc == LAST_FAULT_IDX) {
                out->fatal(CALL_INFO,-1, "Invalid Fault Location (\"%s\") Line %d\n", 
                           firstWord.c_str(), lineNum);
            }

            string trigger;
            bool cycle_trigger = 0;
            in >> trigger;

            if (!in.good()) {
                out->fatal(CALL_INFO,-1, "Fault File Error Line %d\n", lineNum);
            } else {
                std::transform(trigger.begin(), trigger.end(), trigger.begin(),
                               [](unsigned char c){ return std::tolower(c); });

                if (trigger == "cycle") {
                    printf("Cycle Trigger ");
                    cycle_trigger = 1;
                } else if (trigger == "event") {
                    printf("Event Trigger ");
                    cycle_trigger = 0;
                } else {
                    out->fatal(CALL_INFO,-1, "Invalid Fault Trigger (\"%s\") Line %d\n", 
                           trigger.c_str(), lineNum);
                }
            }

            uint64_t when;
            readNum(when, in, lineNum);

            uint32_t what;
            readNum(what, in, lineNum);

            printf("@ %llu flip: 0x%x\n", when, what);

            if (what == 0) {
                out->fatal(CALL_INFO,-1, "Cannot Specify Fault with no bits flipping. Line %d\n", 
                           lineNum);
            }

            // put in data
            if (cycle_trigger) {
                upcomingFaults_c[loc].push_back(faultFileDesc(when, what));
            } else {
                upcomingFaults_e[loc].push_back(faultFileDesc(when, what));
            }
        }

        return true; // more to do (probably)
    } else if (in.eof()) {
        return false; //we're done
    } else {
        out->fatal(CALL_INFO,-1, "fault file error. Line %d\n", lineNum);
        return false;
    }
}

void faultChecker_t::readFaultFile(string fault_file_path) {

    // open file
    ifstream f_file;
    f_file.open(fault_file_path);
    if (!f_file.is_open() || !f_file.good()) {
        out->fatal(CALL_INFO,-1, "Cannot Open fault file: %s\n", fault_file_path.c_str());
    }

    // read lines/comments, inserting into upcomingFaults_x
    bool moreToRead = true;
    int n = 0;
    while(moreToRead) {
        moreToRead = readFaultFileLine(f_file, ++n);
    }

    // close file
    f_file.close();

    // sort upcomingFaults_x
    for (int i = NO_LOC_FAULT_IDX; i < LAST_FAULT_IDX; ++i) {
        sort(upcomingFaults_c[i].begin(), upcomingFaults_c[i].end(), 
             [](const faultFileDesc& a, const faultFileDesc& b) -> bool {
                 return a.when > b.when;
             });
        sort(upcomingFaults_e[i].begin(), upcomingFaults_e[i].end(), 
             [](const faultFileDesc& a, const faultFileDesc& b) -> bool {
                 return a.when > b.when;
             });
    }
}

bool faultChecker_t::checkForNewStyleFault(location_idx_t idx, uint32_t &faultedBits) {
    bool ret = false;
    uint32_t what=0;

    // check for fault @ cycle
    if (!upcomingFaults_c[idx].empty() && 
        reg_word::getNow() == upcomingFaults_c[idx].back().when) {        
        ret = true;
        what = upcomingFaults_c[idx].back().what;
        upcomingFaults_c[idx].pop_back();
    }

    // check for fault @ event
    if (!upcomingFaults_e[idx].empty() && 
        event_count[idx] == upcomingFaults_e[idx].back().when) {
        uint32_t e_faultBits = upcomingFaults_e[idx].back().what;
        // if both a by-cycle and by-event fault occur at the same
        // time, we note it and combine the bits.
        if (ret) {
            out->verbose(CALL_INFO, 1, 0, 
                         "By-cycle and by-event fault occur simultaneously."
                         " Combining faults (%x|%x = %x)\n", what, e_faultBits, what|e_faultBits);
        }

        ret = true;
        what |= e_faultBits;
        upcomingFaults_e[idx].pop_back();
    }    

    faultedBits = what;
    return ret;
}

// should we inject?
bool faultChecker_t::checkForFault(faultTrack::location_t loc, uint32_t &faultedBits) {
    location_idx_t newLoc = LAST_FAULT_IDX; 
    // default for old-style faults is 0 - let calling function
    // randomly determine which bit to flip. Otherwise, use the global
    // bitmask.
    faultedBits = bitsToFlip; 

    // advances event counts and check for "old style" faults 
    switch (loc) {
    case RF_FAULT:
        newLoc = RF_FAULT_IDX;
        event_count[RF_FAULT_IDX]++;
        if (reg_word::getNow() == faultTime[RF_FAULT_IDX]) {return true;}
        break;
    case ID_FAULT:
        // ???
        newLoc = ID_FAULT_IDX;
        event_count[ID_FAULT_IDX]++;
        break;
    case MDU_FAULT:
        newLoc = MDU_FAULT_IDX;
        event_count[MDU_FAULT_IDX]++;
        if (fault_by_time) {
            if(reg_word::getNow() == faultTime[MDU_FAULT_IDX]) {
                return true;
            }
        } else {
            if(event_count[MDU_FAULT_IDX] == faultTime[MDU_FAULT_IDX]) {
                return true;
            }
        }
        break;
    case MEM_PRE_ADDR_FAULT:
        newLoc = MEM_PRE_ADDR_FAULT_IDX;
        // this assumes that MEM_PRE must be checked everytime since
        // we don't increment in MEM_POST
        event_count[MEM_PRE_ADDR_FAULT_IDX]++;
        if (fault_by_time) {
            if(reg_word::getNow() == faultTime[MEM_PRE_ADDR_FAULT_IDX]) {return true;}
        } else {
            if(event_count[MEM_PRE_ADDR_FAULT_IDX] == faultTime[MEM_PRE_ADDR_FAULT_IDX]) {return true;}
        }
        break;
    case MEM_PRE_DATA_FAULT:
        newLoc = MEM_PRE_DATA_FAULT_IDX;
        // this assumes that MEM_PRE must be checked everytime since
        // we don't increment in MEM_POST
        event_count[MEM_PRE_DATA_FAULT_IDX]++;
        if (fault_by_time) {
            if(reg_word::getNow() == faultTime[MEM_PRE_DATA_FAULT_IDX]) {return true;}
        } else {
            if(event_count[MEM_PRE_DATA_FAULT_IDX] == faultTime[MEM_PRE_DATA_FAULT_IDX]) {return true;}
        }
        break;
    case MEM_POST_FAULT:
        newLoc = MEM_POST_FAULT_IDX;
        event_count[MEM_POST_FAULT_IDX]++;
        if (fault_by_time) {
            if(reg_word::getNow() == faultTime[MEM_POST_FAULT_IDX]) {return true;}
        } else {
            if(event_count[MEM_PRE_ADDR_FAULT_IDX] == faultTime[MEM_POST_FAULT_IDX]) {return true;}
        }
        break;

#define STD_F_CASE(STR) \
        case STR: \
            newLoc = STR##_IDX;                                         \
            event_count[STR##_IDX]++;                                   \
            if (fault_by_time) {                                        \
                if (reg_word::getNow() == faultTime[STR##_IDX]) {return true;} \
            } else {                                                    \
                if(event_count[STR##_IDX] == faultTime[STR##_IDX]) {    \
                    return true;                                        \
                }                                                       \
            }                                                           \
            break;

        STD_F_CASE(WB_FAULT);
        STD_F_CASE(ALU_FAULT);
        STD_F_CASE(MEM_BP_FAULT);
        STD_F_CASE(CONTROL_FAULT); 
        STD_F_CASE(INST_ADDR_FAULT);
        STD_F_CASE(INST_TYPE_FAULT);
        STD_F_CASE(WB_ADDR_FAULT);
        STD_F_CASE(PC_FAULT); 

#undef STD_F_CASE

    default:
        printf("Unknown fault location\n");
    }

    // check for New Style (file-based) faults
    bool ret = checkForNewStyleFault(newLoc, faultedBits);

    // default
    return ret;
}

// handy for picking which register in register file
unsigned int faultChecker_t::getRand1_31() {
    unsigned int ret;
    do {
        ret = rng->generateNextUInt32() & 0x1f;
    } while (ret == 0);
    return ret;
}

faultDesc faultChecker_t::getFault(faultTrack::location_t loc, uint32_t &flippedBits) {
    if (flippedBits == 0) { //randomply generate single bit fault
        int bit = rng->generateNextUInt32() & 0x1f;
        uint32_t bits = (1<<bit);
        return faultDesc(loc, bits);
    } else {
        return faultDesc(loc, flippedBits);
    }
}

void faultChecker_t::checkAndInject_RF(reg_word R[32]) {
    uint32_t flippedBits = 0;
    if(checkForFault(faultTrack::RF_FAULT, flippedBits)) {
        unsigned int reg = getRand1_31();
        printf("INJECTING RF FAULT reg %d (bits %x) @ %lld\n", reg,
               flippedBits, reg_word::getNow());
        R[reg].addFault(getFault(RF_FAULT, flippedBits));
    }
}


void faultChecker_t::checkAndInject_MDU(reg_word &hi, reg_word &lo) {
    uint32_t flippedBits = 0;
    if (checkForFault(MDU_FAULT, flippedBits)) {
        uint32_t roll = rng->generateNextUInt32();  // should replace random hi/lo selection
        bool faultHi = roll & 0x1;  
        roll >>= 1;

        faultDesc theFault = getFault(MDU_FAULT, flippedBits);

        printf("INJECTING MDU FAULT reg %s:%08x @ %lld\n", 
               (faultHi) ? "hi" : "lo", theFault.bits, reg_word::getNow());
        if (faultHi) {
            hi.addFault(theFault);
        } else {
            lo.addFault(theFault);
        }
    }
}

// possibly fault data and address
// isLoad ignored for now
void faultChecker_t::checkAndInject_MEM_PRE(reg_word &addr, 
                                            reg_word &value, bool isLoad) {
    uint32_t flippedBits = 0;
    bool faultAddr = checkForFault(MEM_PRE_ADDR_FAULT, flippedBits);

    if (faultAddr) {
        faultDesc theFault = getFault(MEM_PRE_ADDR_FAULT, flippedBits);

        printf("INJECTING MEM_PRE_ADDR FAULT %08x %s @ %lld\n", 
               theFault.bits,
               (isLoad) ? "(isLoad)" : "(isStore)",
               reg_word::getNow());

        addr.addFault(theFault);
    }

    flippedBits = 0;
    bool faultData = checkForFault(MEM_PRE_DATA_FAULT, flippedBits);

    if (faultData) {
        faultDesc theFault = getFault(MEM_PRE_DATA_FAULT, flippedBits);

        printf("INJECTING MEM_PRE_DATA FAULT %08x %s @ %lld\n", 
               theFault.bits,
               (isLoad) ? "(isLoad)" : "(isStore)",
               reg_word::getNow());
        if (isLoad) {
            printf(" INJECTING fault to data on load: no effect\n");
        }
        value.addFault(theFault);
    }
}

// inject fault after MEM. Note: if inject into 'store' may have no
// value
void faultChecker_t::checkAndInject_MEM_POST(reg_word &data) {
    uint32_t flippedBits = 0;
    if(checkForFault(MEM_POST_FAULT, flippedBits)) {
        printf("INJECTING MEM_POST Fault @ %lld\n", reg_word::getNow());
        data.addFault(getFault(MEM_POST_FAULT, flippedBits));
    }
}

void faultChecker_t::checkAndInject_WB(reg_word &data) {
    uint32_t flippedBits = 0;
    if(checkForFault(WB_FAULT, flippedBits)) {
        printf("INJECTING WB Fault  @ %lld\n", reg_word::getNow());
        data.addFault(getFault(WB_FAULT, flippedBits));
    }
}

void faultChecker_t::checkAndInject_ALU(reg_word &data) {
    uint32_t flippedBits = 0;
    if(checkForFault(ALU_FAULT, flippedBits)) { 
        printf("INJECTING ALU Fault  @ %lld\n", reg_word::getNow());
        data.addFault(getFault(ALU_FAULT, flippedBits));
    } 
}

void faultChecker_t::checkAndInject_MEM_BP_FAULT(reg_word &data) {
    uint32_t flippedBits = 0;
    if(checkForFault(MEM_BP_FAULT, flippedBits)) {
        printf("INJECTING MEM_BP Fault  @ %lld\n", reg_word::getNow());
        data.addFault(getFault(MEM_BP_FAULT, flippedBits));
    }
}

void faultChecker_t::checkAndInject_CONTROL_FAULT(reg_word &data) {
    out->fatal(CALL_INFO,-1, "Control Fault\n");
}

void faultChecker_t::checkAndInject_INST_ADDR_FAULT(reg_word &data) {
    uint32_t flippedBits = 0;
    if(checkForFault(INST_ADDR_FAULT, flippedBits)) {
        printf("INJECTING INST_ADDR Fault  @ %lld\n", reg_word::getNow());
        data.addFault(getFault(INST_ADDR_FAULT, flippedBits));
    }
}

void faultChecker_t::checkAndInject_INST_TYPE_FAULT(pipe_stage *ps_ptr) {
    uint32_t flippedBits = 0;
    char buf[128];
    if(checkForFault(INST_TYPE_FAULT, flippedBits)) {
        printf("INJECTING INST_TYPE Fault  @ %lld\n", reg_word::getNow());
        MIPS4KC::print_inst_internal(buf,128,ps_ptr->inst,
                                     STAGE_PC(ps_ptr).getData());
        printf(" Changing %s ", buf);
        // get the old instruction encoding
        unsigned long encoding = ps_ptr->inst->encoding;
        // permute it
        if (flippedBits == 0) {
            int bit = rng->generateNextUInt32() & 0x1f;
            flippedBits = (1<<bit);
        }
        encoding ^= flippedBits;
        // reencode
        instruction *newInst = MIPS4KC::inst_decode(encoding);
        MIPS4KC::print_inst_internal(buf,128,newInst,
                                     STAGE_PC(ps_ptr).getData());
        newInst->faulted = 1;
        printf("-> %s\n", buf);
        // save it to pipe stage
        ps_ptr->inst = newInst;
    }
}

// take in the register file, index to be written, and the value.
// Check for faults and perform the writeback.
void faultChecker_t::checkAndInject_WB_ADDR_FAULT(reg_word (&R)[32],
                                                  const uint origIdx,
                                                  const reg_word &value){
    uint useIdx  = origIdx;
    uint32_t flippedBits = 0;
    if(checkForFault(WB_ADDR_FAULT, flippedBits)) {
        // find (wrong) destination to write to
        faultDesc f = getFault(WB_ADDR_FAULT, flippedBits);
        flippedBits &= 0x1f;
        if (flippedBits == 0) {
            // nothing was flipped, so let's pick something
            int bit = rng->generateNextUInt32() % 5;
            flippedBits = (1<<bit);
        } 
        
        useIdx ^= flippedBits;
        useIdx &= 0x1f; // ignore anything over 31


        printf("INJECTING WB_ADDR Fault (r[%u] instead of r[%u]) @ %lld\n",
               useIdx, origIdx, reg_word::getNow());
        
        if (useIdx != 0) {
            // write back to wrong place and mark as wrong
            R[useIdx].addOverwriteFault(f, value);
        }
        // mark the place that _should_ have been written back is wrong
        R[origIdx].noteNotWritten(f,value);
    } else {
        R[useIdx] = value;
    }
}

void faultChecker_t::checkAndInject_PC_FAULT(reg_word &pc) {
    uint32_t flippedBits = 0;
    if(checkForFault(PC_FAULT, flippedBits)) {
        printf("INJECTING PC Fault @ %lld\n", reg_word::getNow());
        pc.addFault(getFault(PC_FAULT, flippedBits));
    }
}

void faultChecker_t::printStats() {
    printf("Faultable Events:\n");

#define PF(STR) printf("\t%s_event : %llu\n", #STR, event_count[STR##_IDX]);
    
    PF(RF_FAULT);
    PF(ID_FAULT);
    PF(MDU_FAULT);
    PF(MEM_PRE_ADDR_FAULT);
    PF(MEM_POST_FAULT);
    PF(WB_FAULT);
    PF(ALU_FAULT);
    PF(MEM_BP_FAULT);
    PF(CONTROL_FAULT);
    PF(INST_ADDR_FAULT);
    PF(INST_TYPE_FAULT);
    PF(WB_ADDR_FAULT);
    PF(MEM_PRE_DATA_FAULT);
    PF(PC_FAULT);

#undef PF

}
