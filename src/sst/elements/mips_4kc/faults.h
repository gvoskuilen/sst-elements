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

#ifndef _FAULTS_H
#define _FAULTS_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>

using namespace std;

#include "spim.h"
#include "reg.h"
#include "sst/core/rng/sstrng.h"
#include <fstream>

namespace SST {
namespace MIPS4KCComponent {

    // forward declaration
    struct pipe_stage;
    
    // Object to check if we should inject a fault
    class faultChecker_t {
        typedef enum {
            NO_LOC_FAULT_IDX,
            RF_FAULT_IDX,
            ID_FAULT_IDX,
            MDU_FAULT_IDX,
            MEM_PRE_ADDR_FAULT_IDX,
            MEM_POST_FAULT_IDX,
            WB_FAULT_IDX,
            ALU_FAULT_IDX,
            MEM_BP_FAULT_IDX,
            CONTROL_FAULT_IDX,
            INST_ADDR_FAULT_IDX,
            INST_TYPE_FAULT_IDX,
            WB_ADDR_FAULT_IDX,
            MEM_PRE_DATA_FAULT_IDX,
            PC_FAULT_IDX, 
            LAST_FAULT_IDX
        } location_idx_t; 
        static const std::map<std::string, location_idx_t> parseMap;


        RNG::SSTRandom*  rng;

        struct faultFileDesc {
            uint64_t when;
            uint32_t what; // which bits to flip
            faultFileDesc(int64_t _when, uint32_t _what) : 
                when(_when), what(_what) {;}
        };

        // vector of faults for each location by cycle
        vector<faultFileDesc> upcomingFaults_c[LAST_FAULT_IDX];
        // vector of faults for each location by event
        vector<faultFileDesc> upcomingFaults_e[LAST_FAULT_IDX];

        // when the next fault at that location should be injeted (Old
        // Style)
        int64_t faultTime[LAST_FAULT_IDX]; 
        // count of events for a given location
        int64_t event_count[LAST_FAULT_IDX];

        location_idx_t findLocation(const string& loc);

        // read a decimal or ("0x" prefixed) hex number in
        template <class T>
        void readNum (T &num, ifstream &in, int lineNum) {
            string numStr;
            in >> numStr; 

            if (!in.good()) {
                out->fatal(CALL_INFO,-1, 
                           "Fault File Error Line %d\n", lineNum);
            } else {
                num = std::stol(numStr,nullptr,0);
            }
        }

        bool readFaultFileLine(ifstream &, const int);
        void readFaultFile(string);
        // should we inject? (faultedBits returns bits to flip or 0
        // for random)
        bool checkForFault(faultTrack::location_t, uint32_t &faultedBits);
        // should we inject? (new style)
        bool checkForNewStyleFault(location_idx_t, uint32_t &faultedBits);
        unsigned int getRand1_31(); // generate # from 1 to 31

        // bits To Flip: If using the _old_ style (command line param)
        // mechanism for specificing faults, this specifies which bits
        // will be flipped.
        uint32_t bitsToFlip;

        // if true, all (old style) faults are injected at a the
        // specified time, not the specified event count.
        bool fault_by_time;
        
        // output
        Output *out;
    public:
        faultChecker_t() {rng=0;}
        void init(faultTrack::location_t loc, uint64_t period, 
                  string fault_file, bool _fault_by_time, uint32_t bitsToFlip,
                  uint32_t seed, Output *Out);

        // convenience functions
        faultDesc getFault(faultTrack::location_t, uint32_t &flippedBits);
        void checkAndInject_RF(reg_word R[32]);
        void checkAndInject_MDU(reg_word &hi, reg_word &lo);
        // checks both data and addr faults
        void checkAndInject_MEM_PRE(reg_word &addr, reg_word &value, bool isLoad);
        void checkAndInject_MEM_POST(reg_word &data);
        void checkAndInject_WB(reg_word &data);
        void checkAndInject_ALU(reg_word &data);

        void checkAndInject_MEM_BP_FAULT(reg_word &data);
        void checkAndInject_CONTROL_FAULT(reg_word &data);
        void checkAndInject_INST_ADDR_FAULT(reg_word &data);
        void checkAndInject_INST_TYPE_FAULT(pipe_stage *ps);
        void checkAndInject_WB_ADDR_FAULT(reg_word (&R)[32], uint idx, const reg_word &value);
        void checkAndInject_PC_FAULT(reg_word &data);

        void printStats();
    };
    
};
};


#endif //_FAULTS_H
