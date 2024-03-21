
#include "vdsoparser.h"

TimeData* vdsoPatchData;
SST::ArielComponent::ArielTunnel *vdsoTunnel = NULL;


/************** Instrumentation functions *****************************/

VOID vdsoInstrument(INS ins) {
    ADDRINT insAddr = INS_Address(ins);
    if (insAddr >= vdsoStart && insAddr < vdsoEnd) {
        if (vdsoEntryMap.find(insAddr) != vdsoEntryMap.end()) {
            VdsoFunction func = vdsoEntryMap[insAddr];
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) VdsoEntryPoint,
                    IARG_THREAD_ID, 
                    IARG_UINT32, (uint32_t)func, 
                    IARG_REG_VALUE, REG_RDI,
                    IARG_REG_VALUE, REG_RSI,
                    IARG_END);
        } else if (INS_IsCall(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) VdsoCallPoint,
                    IARG_THREAD_ID,
                    IARG_END);
        } else if (INS_IsRet(ins)) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) VdsoRetPoint,
                    IARG_THREAD_ID,
                    IARG_END);
        }
    }
}

VOID VdsoEntryPoint(THREADID thr, uint32_t func, ADDRINT arg0, ADDRINT arg1) {
    if (!vdsoPatchData[thr].level) {
        if ((VdsoFunction)func == VF_CLOCK_GETTIME) {
            vdsoPatchData[thr].tp = (struct timespec*) arg1;
        } else {
            vdsoPatchData[thr].tv = (struct timeval*) arg0;
        }
        vdsoPatchData[thr].func = (VdsoFunction)func;
        vdsoPatchData[thr].level++;
    }
}

VOID VdsoCallPoint(THREADID thr) {
    vdsoPatchData[thr].level++;
}

VOID VdsoRetPoint(THREADID thr) {
    if (vdsoPatchData[thr].level == 0) return;
    else if (vdsoPatchData[thr].level > 1) {
        vdsoPatchData[thr].level--;
        return;
    } else {
        vdsoPatchData[thr].level = 0;
        if (vdsoPatchData[thr].func == VF_CLOCK_GETTIME) {
            if (vdsoPatchData[thr].tp == NULL) return;
            if (arielenabled) {
                vdsoTunnel->getTimeNs(vdsoPatchData[thr].tp);
                vdsoPatchData[thr].tp->tv_nsec += base_tp.tv_nsec;
                vdsoPatchData[thr].tp->tv_sec += base_tp.tv_sec;
            }
        } else {
            if (vdsoPatchData[thr].tv == NULL) return;
            //struct timeval tv, tmp_tv;
            //tmp_tv = *(vdsoPatchData[thr].tv);
            if (arielenabled) {
                vdsoTunnel->getTime(vdsoPatchData[thr].tv);
            //    tmp_tv = *(vdsoPatchData[thr].tv);
                vdsoPatchData[thr].tv->tv_usec += base_tv.tv_usec;
                vdsoPatchData[thr].tv->tv_sec += base_tv.tv_sec;
            }
            //tv = *(vdsoPatchData[thr].tv);
            //printf("Replacing %" PRIu64 "/%" PRIu64 " with %" PRIu64 "/%" PRIu64 "\n",
            //        tmp_tv.tv_sec, tmp_tv.tv_usec, tv.tv_sec, tv.tv_usec);
        }
    }
}




/************** Functions for locating vDSO information ***************/
// Locate vdso and functions to intercept
void vdsoInit(UINT32 threads, SST::ArielComponent::ArielTunnel* tunnel) {
    find_vdso_section();
    if (!vdsoEnd) return; // Uh-oh we didn't find it!

    vdso_init_info();

    ADDRINT vdsoFuncAddr = (ADDRINT) vdso_find_function("LINUX_2.6", "clock_gettime");
    if (vdsoFuncAddr != 0)
        vdsoEntryMap[vdsoFuncAddr] = VF_CLOCK_GETTIME;

    vdsoFuncAddr = (ADDRINT) vdso_find_function("LINUX_2.6", "__vdso_clock_gettime");
    if (vdsoFuncAddr != 0)
        vdsoEntryMap[vdsoFuncAddr] = VF_CLOCK_GETTIME;

    vdsoFuncAddr = (ADDRINT) vdso_find_function("LINUX_2.6", "gettimeofday");
    if (vdsoFuncAddr != 0)
        vdsoEntryMap[vdsoFuncAddr] = VF_GETTIMEOFDAY;
    
    vdsoFuncAddr = (ADDRINT) vdso_find_function("LINUX_2.6", "__vdso_gettimeofday");
    if (vdsoFuncAddr != 0)
        vdsoEntryMap[vdsoFuncAddr] = VF_GETTIMEOFDAY;

    vdsoPatchData = (TimeData*) malloc(sizeof(TimeData) * threads);
    vdsoTunnel = tunnel;

    // Timer offsets
    arielenabled = false;
    base_tv.tv_sec = 0;
    base_tv.tv_usec = 0;
    base_tp.tv_sec = 0;
    base_tp.tv_nsec = 0;
}

void find_vdso_section() {
    char buf[129];
    buf[128] = '\0';
    FILE *fp = fopen("/proc/self/maps", "r");

    vdsoStart = 0x0;
    vdsoEnd = 0x0;

    if (fp) {
        while (fgets(buf, 128, fp)) {
            if (strstr(buf, "vdso")) {
                char * dash = strchr(buf, '-');
                if (dash) {
                    *dash = '\0';
                    vdsoStart = strtoul(buf, nullptr, 16);
                    vdsoEnd = strtoul(dash+1, nullptr, 16);
                }
            }
        }
    }
}

// Initialize vdso_info struct
void vdso_init_info() {
    size_t i;
    bool found_vaddr = false;
    vdso_info.valid = false;
    vdso_info.load_addr = vdsoStart;

    Elf64_Ehdr *hdr = (Elf64_Ehdr*)vdsoStart;
    Elf64_Phdr *pt = (Elf64_Phdr*)(vdso_info.load_addr + hdr->e_phoff);
    Elf64_Dyn *dyn = 0;

    for (i = 0; i < hdr->e_phnum; i++) {
        if (pt[i].p_type == PT_LOAD && !found_vaddr) {
            found_vaddr = true;
            vdso_info.load_offset = vdsoStart + (uintptr_t)pt[i].p_offset - (uintptr_t)pt[i].p_vaddr;
        } else if (pt[i].p_type == PT_DYNAMIC) {
            dyn = (Elf64_Dyn*)(vdsoStart + pt[i].p_offset);
        }
    }

    if (!found_vaddr || !dyn)
        return; // Uh-oh

    Elf64_Word *hash = 0;
    vdso_info.symstrings = 0;
    vdso_info.symtab = 0;
    vdso_info.versym = 0;
    vdso_info.verdef = 0;
    for (i = 0; dyn[i].d_tag != DT_NULL; i++) {
        switch (dyn[i].d_tag) {
            case DT_STRTAB:
                vdso_info.symstrings = (const char *) ((uintptr_t)dyn[i].d_un.d_ptr + vdso_info.load_offset);
                break;
            case DT_SYMTAB:
                vdso_info.symtab = (Elf64_Sym *) ((uintptr_t)dyn[i].d_un.d_ptr + vdso_info.load_offset);
                break;
            case DT_HASH:
                hash = (Elf64_Word*) ((uintptr_t)dyn[i].d_un.d_ptr + vdso_info.load_offset);
                break;
            case DT_VERSYM:
                vdso_info.versym = (Elf64_Versym*) ((uintptr_t)dyn[i].d_un.d_ptr + vdso_info.load_offset);
                break;
            case DT_VERDEF:
                vdso_info.verdef = (Elf64_Verdef*)((uintptr_t)dyn[i].d_un.d_ptr + vdso_info.load_offset);
                break;
        }
    }

    if (!vdso_info.symstrings || !vdso_info.symtab || !hash) return; // Uh-oh

    if (!vdso_info.verdef)
        vdso_info.versym = 0;

    vdso_info.nbucket = hash[0];
    vdso_info.nchain = hash[1];
    vdso_info.bucket = &hash[2];
    vdso_info.chain = &hash[vdso_info.nbucket + 2];

    vdso_info.valid = true;
}


/************** Functions for locating the calls we want to intercept ***************/

// Locate function 'name' with version 'version'
void *vdso_find_function(const char *version, const char *name) {
    unsigned long ver_hash;
    if (!vdso_info.valid) return 0;

    ver_hash = elf_hash(version);
    Elf64_Word chain = vdso_info.bucket[elf_hash(name) % vdso_info.nbucket];

    for (; chain != STN_UNDEF; chain = vdso_info.chain[chain]) {
        Elf64_Sym *sym = &vdso_info.symtab[chain];

        if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC)
            continue;

        if (ELF64_ST_BIND(sym->st_info) != STB_GLOBAL && ELF64_ST_BIND(sym->st_info) != STB_WEAK)
            continue;

        if (sym->st_shndx == SHN_UNDEF)
            continue;
        
        if (strcmp(name, vdso_info.symstrings + sym->st_name))
            continue;

        if (vdso_info.versym && !vdso_match_version(vdso_info.versym[chain], version, ver_hash))
            continue;

        return (void*) (vdso_info.load_offset + sym->st_value);
    }
    return 0;
}

// Check the version
bool vdso_match_version(Elf64_Versym ver, const char *name, Elf64_Word hash) {
    ver &= 0x7fff;
    Elf64_Verdef *def = vdso_info.verdef;
    while (true) {
        if ((def->vd_flags & VER_FLG_BASE) == 0 && (def->vd_ndx & 0x7fff) == ver) 
            break;

        if (def->vd_next == 0)
            return false;
        def = (Elf64_Verdef*)((char*)def + def->vd_next);
    }

    Elf64_Verdaux *aux = (Elf64_Verdaux*)((char*)def + def->vd_aux);
    return def->vd_hash == hash && !strcmp(name, vdso_info.symstrings + aux->vda_name);
}

// Helper for Elf
unsigned long elf_hash(const char *name) {
    unsigned long h = 0;
    while (*name) {
        h = (h << 4) + *name++;
        unsigned long g = h & 0xf0000000;
        if (g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}  

/* Timer related setup */
#if ! defined(__APPLE__)
void vdsoSetBaseTime(bool enabled, timeval * tv, timespec * tp) {
    base_tp = *tp;
#else
void vdsoSetBaseTime(bool enabled, timeval * tv) {
#endif
    base_tv = *tv;
    arielenabled = enabled;
}
