
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include "inttypes.h"

#include <vector>
#include <set>

#include "shadowstack.h"

// The shadow stack
// Stack per thread
std::vector<std::vector<StackRecord> > arielStack;

// For convienence we try to pick up func ptr -> name translation
// This structure tracks which ptrs we've attempted to translate already
// To simplify synchronization, each thread operates independently
std::vector< std::set<ADDRINT> > instPtrsList;

/* Record function call */
VOID ariel_stack_call(THREADID thr, ADDRINT stackPtr, ADDRINT target, ADDRINT ip) 
{
    // Handle longjmp
    while (arielStack[thr].size() > 0 && stackPtr >= arielStack[thr].back().getStackPtr()) {
        arielStack[thr].pop_back();
    }

    // Add new record
    arielStack[thr].push_back(StackRecord(stackPtr, target, ip));
}

/* Process function return */
VOID ariel_stack_return(THREADID thr, ADDRINT stackPtr)
{

    // Handle longjmp
    while (arielStack[thr].size() > 0 && stackPtr >= arielStack[thr].back().getStackPtr()) {
        arielStack[thr].pop_back();
    }

    arielStack[thr].pop_back();
}

VOID ariel_print_stack(UINT32 thr, BT_FILE_TYPE dest)
{
    unsigned int depth = arielStack[thr].size() - 1;
    vector<ADDRINT> newMappings; 

    for (vector<StackRecord>::reverse_iterator rit = arielStack[thr].rbegin(); rit != arielStack[thr].rend(); rit++) {
        if (instPtrsList[thr].find(rit->getInstPtr()) == instPtrsList[thr].end()) {
            newMappings.push_back(rit->getInstPtr());
            instPtrsList[thr].insert(rit->getInstPtr());
        }

        PRINTFUNC(dest, "0x%" PRIx64 ",0x%" PRIx64 ", %s", rit->getTarget(), rit->getInstPtr(), ((depth == 0) ? "\n" : ""));
        depth--;
    }

    // Generate any new mappings - only works if app was compiled with debug
    for (std::vector<ADDRINT>::iterator it = newMappings.begin(); it != newMappings.end(); it++) {
        string file;
        int line;
        PIN_LockClient();
        PIN_GetSourceLocation(*it, NULL, &line, &file);
        PIN_UnlockClient();
        PRINTFUNC(dest, "MAP: 0x%" PRIx64 ", %s:%d\n", *it, file.c_str(), line);
    }
}

/* Instrument app to pick up calls and returns */
VOID shadowStackInstrument(TRACE trace, VOID* args) {
    RTN rtn = TRACE_Rtn(trace);

    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
        INS tail = BBL_InsTail(bbl);

        if (INS_IsCall(tail)) {
            if (INS_IsDirectBranchOrCall(tail)) {
                ADDRINT target = INS_DirectBranchOrCallTargetAddress(tail);
                INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                        (AFUNPTR) ariel_stack_call,
                        IARG_THREAD_ID,
                        IARG_REG_VALUE, REG_STACK_PTR,
                        IARG_ADDRINT, target,
                        IARG_INST_PTR,
                        IARG_END);
            } else if (!RTN_Valid(rtn) || ".plt" != SEC_Name(RTN_Sec(rtn))) {
                INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                        (AFUNPTR) ariel_stack_call,
                        IARG_THREAD_ID,
                        IARG_REG_VALUE, REG_STACK_PTR,
                        IARG_BRANCH_TARGET_ADDR,
                        IARG_INST_PTR,
                        IARG_END);
            }
        }

        if (RTN_Valid(rtn) && ".plt" == SEC_Name(RTN_Sec(rtn))) {
            INS_InsertCall(tail, IPOINT_BEFORE,
                    (AFUNPTR) ariel_stack_call,
                    IARG_THREAD_ID,
                    IARG_REG_VALUE, REG_STACK_PTR,
                    IARG_BRANCH_TARGET_ADDR,
                    IARG_INST_PTR,
                    IARG_END);
        }

        if (INS_IsRet(tail)) {
            INS_InsertPredicatedCall(tail, IPOINT_BEFORE,
                    (AFUNPTR) ariel_stack_return,
                    IARG_THREAD_ID,
                    IARG_REG_VALUE, REG_STACK_PTR,
                    IARG_END);
        }
    }
}

VOID initShadowStack(UINT32 cores)
{
    arielStack.resize(cores);
    instPtrsList.resize(cores);
}
