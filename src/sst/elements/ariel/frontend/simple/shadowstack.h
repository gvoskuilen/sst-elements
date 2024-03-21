#ifndef ARIEL_FESIMPLE_SHADOW_STACK
#define ARIEL_FESIMPLE_SHADOW_STACK
  
#include <stdlib.h>
#include <stdio.h>
#include "pin.H"
#include <sst_config.h>

#ifdef HAVE_LIBZ
#include "zlib.h"
#define BT_FILE_TYPE gzFile
#define PRINTFUNC gzprintf
#else
#define BT_FILE_TYPE FILE*
#define PRINTFUNC fprintf
#endif


/****************************************************************************/
/****************************************************************************/
/* This code manages a shadow stack of the running Pinned binary            */
/* Currently used by the 'sieve' infrastructure to track malloc locations   */
/****************************************************************************/
/****************************************************************************/


/* Record of each function call */
class StackRecord {
    private:
        ADDRINT stackPtr;
        ADDRINT target;
        ADDRINT instPtr;
    public:
        StackRecord(ADDRINT sp, ADDRINT targ, ADDRINT ip) : stackPtr(sp), target(targ), instPtr(ip) {}
        ADDRINT getStackPtr() const { return stackPtr; }
        ADDRINT getTarget() { return target; }
        ADDRINT getInstPtr() { return instPtr; }
};

/* Instrumentation functions */
VOID ariel_stack_call(THREADID thr, ADDRINT stackPtr, ADDRINT target, ADDRINT ip);
VOID ariel_stack_return(THREADID thr, ADDRINT stackPtr);

/* Output function */
VOID ariel_print_stack(UINT32 thr, BT_FILE_TYPE dest);

/* Insert instrumentation */
VOID shadowStackInstrument(TRACE trace, VOID* args);

/* Initialize data structures */
VOID initShadowStack(UINT32 cores);


#endif
