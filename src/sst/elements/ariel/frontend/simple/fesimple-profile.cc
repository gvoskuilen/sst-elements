// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

//#include <malloc.h>
#include <execinfo.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "pin.H"
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <map>
#include <stack>
#include <ctime>
#include <bitset>
#include <set>
#include <sst_config.h>

#include "vdsoparser.h"
#include "shadowstack.h"

#ifdef HAVE_CUDA
#include "host_defines.h"
#include "builtin_types.h"
#endif

#ifdef HAVE_LIBZ

#include "zlib.h"
#define BT_PRINTF(fmt, args...) gzprintf(btfiles[thr], fmt, ##args);

#else

#define BT_PRINTF(fmt, args...) fprintf(btfiles[thr], fmt, ##args);

#endif

//This must be defined before inclusion of intttypes.h
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "ariel_shmem.h"
#include "ariel_inst_class.h"

#undef __STDC_FORMAT_MACROS


using namespace SST::ArielComponent;

KNOB<UINT32> PerformWriteTrace(KNOB_MODE_WRITEONCE, "pintool",
    "w", "0", "Perform write tracing (i.e copy values directly into SST memory operations) (0 = disabled, 1 = enabled)");
KNOB<UINT32> TrapFunctionProfile(KNOB_MODE_WRITEONCE, "pintool",
    "t", "0", "Function profiling level (0 = disabled, 1 = enabled)");
KNOB<string> SSTNamedPipe(KNOB_MODE_WRITEONCE, "pintool",
    "p", "", "Named pipe to connect to SST simulator");
KNOB<UINT64> MaxInstructions(KNOB_MODE_WRITEONCE, "pintool",
    "i", "10000000000", "Maximum number of instructions to run");
KNOB<UINT32> SSTVerbosity(KNOB_MODE_WRITEONCE, "pintool",
    "v", "0", "SST verbosity level");
KNOB<UINT32> MaxCoreCount(KNOB_MODE_WRITEONCE, "pintool",
    "c", "1", "Maximum core count to use for data pipes.");
KNOB<UINT32> CoreStartIndex(KNOB_MODE_WRITEONCE, "pintool",
    "b", "0", "Which core to start counting from.");
KNOB<UINT32> StartupMode(KNOB_MODE_WRITEONCE, "pintool",
    "s", "1", "Mode for configuring profile behavior, 1 = start enabled, 0 = start disabled, 2 = attempt auto detect");
KNOB<UINT32> InterceptMemAllocations(KNOB_MODE_WRITEONCE, "pintool",
    "m", "1", "Should intercept multi-level memory allocations, mallocs, and frees, 1 = start enabled, 0 = start disabled");
KNOB<string> UseMallocMap(KNOB_MODE_WRITEONCE, "pintool",
    "u", "", "Should intercept ariel_malloc_flag() and interpret using a malloc map: specify filename or leave blank for disabled");
KNOB<UINT32> KeepMallocStackTrace(KNOB_MODE_WRITEONCE, "pintool",
    "k", "1", "Should keep shadow stack and dump on malloc calls. 1 = enabled, 0 = disabled");
KNOB<UINT32> DefaultMemoryPool(KNOB_MODE_WRITEONCE, "pintool",
    "d", "0", "Default SST Memory Pool");
KNOB<string> SSTNamedPipe2(KNOB_MODE_WRITEONCE, "pintool",
    "g", "", "Named pipe to connect to SST simulator");
KNOB<string> SSTNamedPipe3(KNOB_MODE_WRITEONCE, "pintool",
    "x", "", "Named pipe to connect to SST simulator");
KNOB<UINT32> InstrumentInstructions(KNOB_MODE_WRITEONCE, "pintool",
    "E", "1", "Enable instruction instrumentation");

#define ARIEL_MAX(a,b) \
   ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
   
#define ARIEL_MIN(a,b) \
   ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })

/* Function profiling - enabled by TrapFunctionProfile option */
typedef struct {
    int64_t insExecuted;
} ArielFunctionRecord;

std::map<std::string, ArielFunctionRecord*> funcProfile;

/* Basic parameters */
UINT32 core_count;
UINT32 min_core;
ArielTunnel *tunnel = NULL;

/* Ariel enable flag */
UINT32 instrument_instructions;
bool enable_output;
// Beta - delay start by a set number of instructions
bool countInst;
UINT64 * enable_at;

/* Memory allocation and pools */
UINT32 default_pool;
std::vector<void*> allocated_list;
PIN_LOCK mainLock;
UINT64* lastMallocSize;
UINT64* lastMallocLoc;
UINT32 overridePool;
bool shouldOverride;

/* CUDA */
#ifdef HAVE_CUDA
GpuReturnTunnel *tunnelR = NULL;
GpuDataTunnel *tunnelD = NULL;
#endif

/* Support for ARIEL_MALLOC_FLAG */
// Map each location ID to the set of repeats that should go to fast mem
set<int64_t> fastmemlocs;
// Flag whether to send next intercept malloc to fast memory or not
struct mallocFlagInfo {
    bool valid;
    int count;
    int level;
    int id;
    mallocFlagInfo(bool a, int b, int c, int d) : valid(a), count(b), level(c), id(d) {}
};
std::vector<mallocFlagInfo> toFast;

/* Intercept memory values flag */

/* For gettimeofday/get_clocktime overrides: */
struct timeval offset_tv;
#if !defined(__APPLE__)
struct timespec offset_tp_mono;
struct timespec offset_tp_real;
#endif

/* MemSieve support */
// Per-thread malloc file -> we don't have to lock the file this way
// Compress it if possible
#ifdef HAVE_LIBZ
std::vector<gzFile> btfiles;
#else
std::vector<FILE*> btfiles;
#endif

UINT64 mallocIndex;
FILE * rtnNameMap;
PIN_LOCK mallocIndexLock;

// Temporary
uint64_t* rcount;
uint64_t* wcount;
uint64_t* rwcount;
uint64_t* ncount;

/***********************************************/
/************* Function definitions ************/
/***********************************************/

VOID Fini(INT32 code, VOID* v)
{
    if(SSTVerbosity.Value() > 0) {
        std::cout << "SSTARIEL: Execution completed, shutting down." << std::endl;
    }
    
    THREADID thr = PIN_ThreadId();

    std::cout << "ariel_fini. Cleaning up child " << thr << " " << thr + min_core << std::endl;
    ArielCommand ac;
    ac.command = ARIEL_PERFORM_EXIT;
    ac.instPtr = (uint64_t) 0;

    bool done = tunnel->cleanUpChild();
    if (done) {
        printf("Ariel cleaning up last child\n");
        tunnel->writeMessage(0, ac);
        delete tunnel;
#ifdef HAVE_CUDA
        delete tunnelR;
        delete tunnelD;
#endif

    } // Else just clean up local state

    free(ncount);
    free(rwcount);
    free(rcount);
    free(wcount);

}


void mapped_ariel_enable(); // Forward declare
void mapped_ariel_start_instcount() {
    countInst = true;
}

void mapped_ariel_end_instcount() { }

void mapped_ariel_enable_at(uint64_t delay) {
    THREADID thr = PIN_ThreadId();

    /*fprintf(stderr, "ARIEL: Scheduling ariel_enable() in %" PRIu64 " instructions\n", delay);
    if (delay == 0)
        mapped_ariel_enable();
    else
        enable_at[thr] = delay;
    */
}

void decEnableAt(THREADID thr) {
    if (enable_at[thr] = 0)
        return;
    enable_at[thr]--;
    if (enable_at[thr] == 0)
        mapped_ariel_enable();
}

VOID copy(void* dest, const void* input, UINT32 length)
{
    for(UINT32 i = 0; i < length; ++i) {
        ((char*) dest)[i] = ((char*) input)[i];
    }
}

VOID WriteInstructionReadWrite(THREADID thr, ADDRINT* readAddr, UINT32 readSize,
            ADDRINT* writeAddr, UINT32 writeSize, ADDRINT ip, UINT32 instClass,
            UINT32 simdOpWidth )
{
    if (thr < core_count) {
        rwcount[thr]++;
    }
}

VOID WriteInstructionReadOnly(THREADID thr, ADDRINT* readAddr, UINT32 readSize, ADDRINT ip,
            UINT32 instClass, UINT32 simdOpWidth)
{
    if (thr < core_count) {
        rcount[thr]++;
    }
}

VOID WriteNoOp(THREADID thr, ADDRINT ip)
{
    if (thr < core_count) {
        ncount[thr]++;
    }
}

VOID WriteInstructionWriteOnly(THREADID thr, ADDRINT* writeAddr, UINT32 writeSize, ADDRINT ip,
            UINT32 instClass, UINT32 simdOpWidth)
{
    if (thr < core_count) {
        wcount[thr]++;
    }

}

VOID IncrementFunctionRecord(VOID* funcRecord)
{
    ArielFunctionRecord* arielFuncRec = (ArielFunctionRecord*) funcRecord;

    __asm__ __volatile__(
        "lock incq %0"
        : /* no output registers */
        : "m" (arielFuncRec->insExecuted)
        : "memory"
    );
}

VOID InstrumentInstruction(INS ins, VOID *v)
{
    UINT32 simdOpWidth     = 1;
    UINT32 instClass       = ARIEL_INST_UNKNOWN;
    UINT32 maxSIMDRegWidth = 1;

    std::string instCode = INS_Mnemonic(ins);

    for(UINT32 i = 0; i < INS_MaxNumRRegs(ins); i++) {
        if( REG_is_xmm(INS_RegR(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 2);
        } else if ( REG_is_ymm(INS_RegR(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 4);
        } else if ( REG_is_zmm(INS_RegR(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 8);
        }
    }

    for(UINT32 i = 0; i < INS_MaxNumWRegs(ins); i++) {
        if( REG_is_xmm(INS_RegW(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 2);
        } else if ( REG_is_ymm(INS_RegW(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 4);
        } else if ( REG_is_zmm(INS_RegW(ins, i)) ) {
                maxSIMDRegWidth = ARIEL_MAX(maxSIMDRegWidth, 8);
        }
    }

    if( instCode.size() > 1 ) {
        std::string prefix = "";

        if( instCode.size() > 2 ) {
            prefix = instCode.substr(0, 3);
        }

        std::string suffix = instCode.substr(instCode.size() - 2);

        if("MOV" == prefix || "mov" == prefix) {
            // Do not found MOV as an FP instruction?
            simdOpWidth = 1;
        } else {
            if( (suffix == "PD") || (suffix == "pd") ) {
                simdOpWidth = maxSIMDRegWidth;
                instClass = ARIEL_INST_DP_FP;
            } else if( (suffix == "PS") || (suffix == "ps") ) {
                simdOpWidth = maxSIMDRegWidth * 2;
                instClass = ARIEL_INST_SP_FP;
            } else if( (suffix == "SD") || (suffix == "sd") ) {
                simdOpWidth = 1;
                instClass = ARIEL_INST_DP_FP;
            } else if ( (suffix == "SS") || (suffix == "ss") ) {
                simdOpWidth = 1;
                instClass = ARIEL_INST_SP_FP;
            } else {
                simdOpWidth = 1;
            }
        }
    }

    if( INS_IsMemoryRead(ins) && INS_IsMemoryWrite(ins) ) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
                WriteInstructionReadWrite,
                IARG_THREAD_ID,
                IARG_MEMORYREAD_EA, IARG_UINT32, INS_MemoryReadSize(ins),
                IARG_MEMORYWRITE_EA, IARG_UINT32, INS_MemoryWriteSize(ins),
                IARG_INST_PTR,
                IARG_UINT32, instClass,
                IARG_UINT32, simdOpWidth,
                IARG_END);
    } else if( INS_IsMemoryRead(ins) ) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
                WriteInstructionReadOnly,
                IARG_THREAD_ID,
                IARG_MEMORYREAD_EA, IARG_UINT32, INS_MemoryReadSize(ins),
                IARG_INST_PTR,
                IARG_UINT32, instClass,
                IARG_UINT32, simdOpWidth,
                IARG_END);
    } else if( INS_IsMemoryWrite(ins) ) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
                WriteInstructionWriteOnly,
                IARG_THREAD_ID,
                IARG_MEMORYWRITE_EA, IARG_UINT32, INS_MemoryWriteSize(ins),
                IARG_INST_PTR,
                IARG_UINT32, instClass,
                IARG_UINT32, simdOpWidth,
                IARG_END);
    } else {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)
                WriteNoOp,
                IARG_THREAD_ID,
                IARG_INST_PTR,
                IARG_END);
    }

    vdsoInstrument(ins);
}

/* Intercept ariel_enable() in application & start simulating instructions */
void mapped_ariel_enable()
{

    // Note
    // By adding clock offset calculation, this function now has visible side-effects when called more than once
    // In most cases won't matter -> ariel_enable() called once or close together in time so offsets will stabilize quickly
    // In some cases could cause a big jump in time in the middle of simulation -> ariel_enable() left in app but mode is always-on
    // So, update ariel_enable & offsets in lock & don't update if already enabled

    /* LOCK */
    THREADID thr = PIN_ThreadId();
    PIN_GetLock(&mainLock, thr);
    
    if (enable_output) {
        PIN_ReleaseLock(&mainLock);
        return;
    }
    
    // Setup timers to count start time + elapsed simulated time
    struct timeval tvsim;
    gettimeofday(&offset_tv, nullptr);
    tunnel->getTime(&tvsim);
    offset_tv.tv_sec -= tvsim.tv_sec;
    offset_tv.tv_usec -= tvsim.tv_usec;
#if ! defined(__APPLE__)
    struct timespec tpsim;
    clock_gettime(CLOCK_MONOTONIC, &offset_tp_mono);
    clock_gettime(CLOCK_REALTIME, &offset_tp_real);
    tunnel->getTimeNs(&tpsim);
    offset_tp_mono.tv_sec -= tpsim.tv_sec;
    offset_tp_mono.tv_nsec -= tpsim.tv_nsec;
    offset_tp_real.tv_sec -= tpsim.tv_sec;
    offset_tp_real.tv_nsec -= tpsim.tv_nsec;
#endif
    /* ENABLE */
    enable_output = true;
#if ! defined(__APPLE__)
    vdsoSetBaseTime(true, &offset_tv, &offset_tp_mono);
#else
    vdsoSEtBaseTime(true, &offset_tv);
#endif

    /* UNLOCK */
    PIN_ReleaseLock(&mainLock);

    fprintf(stderr, "ARIEL: Enabling memory and instruction tracing from program control at simulated Ariel cycle %" PRIu64 ".\n",
            tunnel->getCycles());
    fflush(stdout);
    fflush(stderr);
}

/* Return the current cycle count from Ariel */
uint64_t mapped_ariel_cycles()
{
    return tunnel->getCycles();
}

/* 
 * Override gettimeofday to return simulated time
 * If ariel_enable is false, returns system gettimeofday value
 * If ariel_enable is true, returns system gettimeofday when ariel was enabled + elapsed simulated time since ariel was enabled
 */
int mapped_gettimeofday(struct timeval *tp, void *tzp)
{
    // Return 'real' time if simulation not enabled
    if (!enable_output) {
       return gettimeofday(tp, NULL);
    }

    if ( tp == NULL ) { errno = EINVAL ; return -1; }
    tunnel->getTime(tp);
    tp->tv_sec += offset_tv.tv_sec;
    tp->tv_usec += offset_tv.tv_usec;
    return 0;
}

/*
 * Override clock_gettime to return simulated time
 * If ariel_enable is false, returns actual clock_gettime value
 * If ariel_enable is true, returns elapsed simulated time since ariel was enabled + clock_gettime(CLOCK_MONOTONIC) from
 * when ariel was enabled
 */
#if ! defined(__APPLE__)
int mapped_clockgettime(clockid_t clock, struct timespec *tp)
{
    if (!enable_output)
        return clock_gettime(clock, tp);

    if (tp == NULL) { errno = EINVAL; return -1; }
    tunnel->getTimeNs(tp);

    // Only offset these two clocks -> TODO the others
    if (clock == CLOCK_MONOTONIC) {
        tp->tv_sec += offset_tp_mono.tv_sec;
        tp->tv_nsec += offset_tp_mono.tv_nsec;
    } else if (clock == CLOCK_REALTIME) {
        tp->tv_sec += offset_tp_real.tv_sec;
        tp->tv_nsec += offset_tp_real.tv_nsec;
    }
    
    return 0;
}
#endif


void mapped_ariel_region_flag(char const * name, int id) {
    UINT32 thr = (UINT32) PIN_ThreadId();
    struct timeval tp, tv;
    mapped_gettimeofday(&tv, NULL);
    //fprintf(stdout, "ariel_region_flag. ArielCore: %" PRIu32 ", Id: %d, Section: %s, Sim cycles: %" PRIu64 ", Sim gettimeofday: %" PRIu64 "s/%" PRIu64 "us\n",
    //        thr, id, name, tunnel->getCycles(), tv.tv_sec, tv.tv_usec);
    fprintf(stdout, "ariel_region_flag. ArielCore: %" PRIu32 ", Id: %d, Section: %s, Sim cycles: %" PRIu64 ", Sim gettimeofday: %" PRIu64 "s/%" PRIu64 "us\n",
            thr, id, name, tunnel->getCycles(), tv.tv_sec, tv.tv_usec);
    for (int i = 0; i < core_count; i++) {
        fprintf(stdout, "ariel_region_flag. ArielCore: %" PRIu32 ", Id: %d, Section: %s, Instruction_counts(%d): %" PRIu64 ", %" PRIu64 ", %" PRIu64 ", %" PRIu64 "\n",
            thr, id, name, i, rcount[i], wcount[i], rwcount[i], ncount[i]);
        rcount[i] = 0;
        wcount[i] = 0;
        rwcount[i] = 0;
        ncount[i] = 0;
    }
    fflush(stdout);
}

VOID InstrumentRoutine(RTN rtn, VOID* args)
{
    if (RTN_Name(rtn) == "gettimeofday" || RTN_Name(rtn) == "_gettimeofday") {
        fprintf(stderr,"Identified routine: gettimeofday, replacing with Ariel equivalent...\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_gettimeofday);
        fprintf(stderr,"Replacement complete.\n");
        return;
#if ! defined(__APPLE__)
    } else if (RTN_Name(rtn) == "clock_gettime" || RTN_Name(rtn) == "_clock_gettime" ||
        RTN_Name(rtn) == "__clock_gettime") {
        fprintf(stderr,"Identified routine: clock_gettime, replacing with Ariel equivalent...\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_clockgettime);
        fprintf(stderr,"Replacement complete.\n");
        return;
#endif
    } else if (RTN_Name(rtn) == "ariel_region_flag" || RTN_Name(rtn) == "_ariel_region_flag") {
        fprintf(stderr, "Identified routine: ariel_region_flag, replacing with Ariel equivalent..\n");
        RTN_Replace(rtn, (AFUNPTR) mapped_ariel_region_flag);
        fprintf(stderr, "Replacement complete\n");
    }
}

/*(===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR( "This Pintool collects statistics for instructions.\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    // Load the symbols ready for us to mangle functions.
    //PIN_InitSymbolsAlt(IFUNC_SYMBOLS);
    PIN_InitSymbols();
    PIN_AddFiniFunction(Fini, 0);

    PIN_InitLock(&mainLock);
    PIN_InitLock(&mallocIndexLock);

    if(SSTVerbosity.Value() > 0) {
        std::cout << "SSTARIEL: Loading Ariel Tool to connect to SST on pipe: " <<
            SSTNamedPipe.Value() << " max instruction count: " <<
            MaxInstructions.Value() <<
            " max core count: " << MaxCoreCount.Value() << std::endl;
    }

    core_count = MaxCoreCount.Value();
    min_core = CoreStartIndex.Value();
    instrument_instructions = InstrumentInstructions.Value();

    rcount = (UINT64*) malloc(sizeof(UINT64) * core_count);
    wcount = (UINT64*) malloc(sizeof(UINT64) * core_count);
    rwcount = (UINT64*) malloc(sizeof(UINT64) * core_count);
    ncount = (UINT64*) malloc(sizeof(UINT64) * core_count);

    tunnel = new ArielTunnel(SSTNamedPipe.Value());
#ifdef HAVE_CUDA
    tunnelR = new GpuReturnTunnel(SSTNamedPipe2.Value());
    tunnelD = new GpuDataTunnel(SSTNamedPipe3.Value());
#endif
    
    lastMallocSize = (UINT64*) malloc(sizeof(UINT64) * core_count);
    lastMallocLoc = (UINT64*) malloc(sizeof(UINT64) * core_count);
    mallocIndex = 0;

    fprintf(stderr, "ARIEL-SST PIN tool activating with %" PRIu32 " threads\n", core_count);
    fflush(stdout);

    sleep(1);

    default_pool = DefaultMemoryPool.Value();
    fprintf(stderr, "ARIEL: Default memory pool set to %" PRIu32 "\n", default_pool);

    if(StartupMode.Value() == 1) {
        fprintf(stderr, "ARIEL: Tool is configured to begin with profiling immediately.\n");
        enable_output = true;
    } else if (StartupMode.Value() == 0) {
        fprintf(stderr, "ARIEL: Tool is configured to suspend profiling until program control\n");
        enable_output = false;
    } else if (StartupMode.Value() == 2) {
        fprintf(stderr, "ARIEL: Tool is configured to attempt auto detect of profiling\n");
        fprintf(stderr, "ARIEL: Initial mode will be to enable profiling unless ariel_enable function is located\n");
        enable_output = true;
    }

    enable_at = (UINT64*) malloc(sizeof(UINT64) * core_count);
    for (int i = 0; i < core_count; i++)
        enable_at[i] = 0;

    /* If not using ariel_enable, then gettimeofday/clock_gettime always return simulated time */
    offset_tv.tv_sec = 0;
    offset_tv.tv_usec = 0;
#if ! defined(__APPLE__)
    offset_tp_mono.tv_sec = 0;
    offset_tp_mono.tv_nsec = 0;
    offset_tp_real.tv_sec = 0;
    offset_tp_real.tv_nsec = 0;
#endif

    vdsoInit(core_count, tunnel);

    if(instrument_instructions){
        INS_AddInstrumentFunction(InstrumentInstruction, 0);
    }

    RTN_AddInstrumentFunction(InstrumentRoutine, 0);

#if ! defined(__APPLE__)
    vdsoSetBaseTime(enable_output, &offset_tv, &offset_tp_mono);
#else
    vdsoSetBaseTime(enable_output, &offset_tv);
#endif

    fprintf(stderr, "ARIEL: Starting program.\n");
    fflush(stdout);
    PIN_StartProgram();

    return 0;
}

