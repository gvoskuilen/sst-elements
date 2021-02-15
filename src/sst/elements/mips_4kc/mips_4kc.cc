// Copyright 2019,2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2019,2020 NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "mips_4kc.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/elements/memHierarchy/memEvent.h>

using namespace SST;
using namespace SST::MIPS4KCComponent;

Cycle_t reg_word::now = 0;
uint64_t reg_word::faultStats[faultTrack::LAST_FAULT_STATUS] = {};
map<int32_t, memFaultDesc> reg_word::memFaults;
map<int32_t, uint8_t> reg_word::origMem;

int MIPS4KC::sorted_name_table = 0;
int MIPS4KC::sorted_i_opcode_table = 0; 
int MIPS4KC::sorted_a_opcode_table = 0;

MIPS4KC::MIPS4KC(ComponentId_t id, Params& params) :
    Component(id), 
    break_inst(NULL), program_break(0), 
    breakpoint_reinsert(0), text_seg(0), text_modified(0), text_top(0),
    data_seg(0), data_modified(0), data_seg_h(0), data_seg_b(0), 
    data_top(0), DATA_BOT(0), cycle_level(0), cycle_running(0)
{
    outputLevel = params.find<uint32_t>("verbose", 0);
    out.init("MIPS4KC:@p:@l: ", outputLevel, 0, Output::STDOUT);

    // tell the simulator not to end without us
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<MIPS4KC>(this, &MIPS4KC::clockTic);
    clockTC = registerClock(clockFreq, clockHandler);

    // init memory
    memory = loadUserSubComponent<Interfaces::SimpleMem>("memory", ComponentInfo::SHARE_NONE, clockTC, new Interfaces::SimpleMem::Handler<MIPS4KC>(this, &MIPS4KC::handleEvent));
    if (!memory) {
        params.insert("port", "mem_link");
        memory = loadAnonymousSubComponent<Interfaces::SimpleMem>("memHierarchy.memInterface", "memory", 0,
                ComponentInfo::SHARE_PORTS, params, clockTC, new Interfaces::SimpleMem::Handler<MIPS4KC>(this, &MIPS4KC::handleEvent));
    }
    if (!memory)
        out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.memInterface subcomponent\n");

    // set timeout
    timeout = params.find<int64_t>("timeout", -1);

    // set stack 
    STACK_TOP = params.find<uint32_t>("stack_top", 0x80000000);

    // processor number
    proc_num = params.find<uint32_t>("proc_num", 0);

    // set executable file
    string execFile = params.find<std::string>("execFile");
    if (execFile.empty()) {
        out.fatal(CALL_INFO, -1, "No executabel file specified");
    } 

    // init fault injection
    uint32_t fault_locations = params.find<uint32_t>("fault_locations", 0);
    uint64_t fault_period = params.find<uint64_t>("fault_period", 100);
    uint64_t fault_bits = params.find<uint32_t>("fault_bits", 0);
    uint32_t fault_rng_seed = params.find<uint32_t>("fault_rng_seed", 0);
    bool fault_by_time = params.find<bool>("fault_by_time", 0);
    string fault_file = params.find<string>("fault_file", "");
    faultChecker.init((faultTrack::location_t)fault_locations, fault_period,
                      fault_file, fault_by_time, fault_bits, fault_rng_seed,
                      &out);

    // SPIM-CL config
    pipe_out =  stdout;
    console_out =  stdout;
    message_out =  stdout;   
    console_in = 0;
    mapped_io = 0;
    cycle_level = 1;
    bare_machine = 1;
    quiet = 1;
    /*tlb_on = 0;
    icache_on = 0;
    dcache_on = 0;*/

    program_starting_address = 0;
    initial_text_size = TEXT_SIZE;
    initial_data_size = DATA_SIZE;
    initial_data_limit = DATA_LIMIT;
    initial_stack_size = STACK_SIZE;
    initial_stack_limit = STACK_LIMIT;
    initial_k_text_size = K_TEXT_SIZE;
    initial_k_data_size = K_DATA_SIZE;
    initial_k_data_limit = K_DATA_LIMIT;

    text_seg = 0;
    data_seg = 0;
    data_seg_h = 0;
    data_seg_b = 0;
    stack_seg = 0;
    stack_seg_b = 0;
    k_text_seg = 0;
    k_data_seg_h = 0;
    k_data_seg_b = 0;
    k_data_top = 0;

    initialize_world(0);
    cl_initialize_world(1);
    read_aout_file(execFile.c_str());
    PC = program_starting_address;
}

MIPS4KC::MIPS4KC() : Component(-1)
{
	// for serialization only
}


void MIPS4KC::init(unsigned int phase) {  
    // init memory
    memory->init(phase);
    
    // Everything below we only do once
    if (phase != 0) {
        return;
    }

    // send the program image out to memory
    for(auto i = image.begin(); i != image.end(); ++i) {
        memReq *req = new memReq(memReq::Write, i->first, 1);
        req->data.resize(1);
        req->data[0] = i->second;
        memory->sendInitData(req);
        // record for the origMem - the known good data
        reg_word::initOrigMem(i->first, i->second);
    }
    image.clear();
}

// handle incoming memory
void MIPS4KC::handleEvent(memReq *req)
{
    //printf("got event %llx\n", req->id);

    std::map<uint64_t, PIPE_STAGE>::iterator i = requestsOut.find(req->id);
    if (i == requestsOut.end()) {
	out.fatal(CALL_INFO, -1, "Request ID (%" PRIx64 ") not found in outstanding requests!\n", req->id);
    } else {
        // handle event
        requestsIn.insert(std::make_pair(i->second,req));
        // clean up
        requestsOut.erase(i);
    }
}

bool MIPS4KC::clockTic( Cycle_t c)
{
    bool isFalling = (c & 0x1);
    Cycle_t pipeCycle = c >> 1;
    reg_word::setNow(pipeCycle);  // for fault record keeping
    if (outputLevel > 0) {
        printf("CYCLE %llu: %llu.%u\n", c, pipeCycle, isFalling);
    } else {
        if ((pipeCycle & 0x1fff) == 1 && isFalling) {
            //printf("CYCLE %llu: %llu.%u\n", c, pipeCycle, isFalling);
            out.output(CALL_INFO,"CYCLE %llu: %llu.%u\n", c, pipeCycle,
                       isFalling);
        }
    }

    if ((timeout != -1) && (c > timeout)) {
        printf("Timeout Reached.\n");
        primaryComponentOKToEndSim();
        return true;
    }
    
    if (isFalling) {
        if (cl_run_falling (PC, outputLevel)) {
            primaryComponentOKToEndSim();
            return true; // stop
        }
    } else {
        cl_run_rising(); // issue memory requests
    }

    // return false so we keep going
    return false;
}


void MIPS4KC::finish() {
#warning should make SST stats
    reg_word::printStats();
    faultChecker.printStats();
}
