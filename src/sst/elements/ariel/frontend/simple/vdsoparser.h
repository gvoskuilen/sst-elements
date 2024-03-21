
#ifndef _VDSO_PARSER_H_
#define _VDSO_PARSER_H_
#include <stdlib.h>
#include "pin.H"
#include <fstream>
#include <elf.h>
#include <ctime>
#include <map>

#include "ariel_shmem.h"

/**** Functions we're searching for ****/
enum VdsoFunction { VF_CLOCK_GETTIME, VF_GETTIMEOFDAY};

/**** Data structures ****/
static struct vdso_info {               // Holds parsed info from vdso section
    bool valid;
    /* Load info */
    uintptr_t load_addr;
    uintptr_t load_offset;
    /* Symbol table */
    Elf64_Sym *symtab;
    const char *symstrings;
    Elf64_Word *bucket, *chain;
    Elf64_Word nbucket, nchain;
    /* Version table */
    Elf64_Versym *versym;
    Elf64_Verdef *verdef;
} vdso_info;

static std::map<ADDRINT, VdsoFunction> vdsoEntryMap;    // Maps function address to info

struct TimeData {              // Struct holding call/return info for intercepted time functions
    union {
        struct timespec *tp;
        struct timeval *tv;
    };
    VdsoFunction func;
    uint32_t level;
};


static uintptr_t vdsoStart;
static uintptr_t vdsoEnd;

static struct timeval base_tv;
static struct timespec base_tp;
static bool arielenabled;

/**** Find and parse vdso info ****/
void vdsoInit(UINT32 threads, SST::ArielComponent::ArielTunnel * tunnel);               // Lookup functions in the vdso section 
void find_vdso_section();                                                               // Find the vDSO section
void vdso_init_info();                                                                  // Fill the vdso_info struct
void* vdso_find_function(const char *version, const char *name);                        // Locate function with this name and version
static unsigned long elf_hash(const char *name);                                        // Helper for parsing elf
static bool vdso_match_version(Elf64_Versym ver, const char *name, Elf64_Word hash);    // Check function version


/**** Instrument intercepted functions ****/
void vdsoInstrument(INS ins);    // Instrument instruction

#if ! defined(__APPLE__)
void vdsoSetBaseTime(bool arielenabled, timeval * tv, timespec * tp);
#else
void vdsoSetBaseTime(bool arielenabled, timeval * tv);
#endif

VOID VdsoCallPoint(THREADID thr);
VOID VdsoEntryPoint(THREADID thr, uint32_t func, ADDRINT arg0, ADDRINT arg1);
VOID VdsoRetPoint(THREADID thr);

#endif
