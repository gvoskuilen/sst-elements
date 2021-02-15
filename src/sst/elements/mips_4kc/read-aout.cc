/* SPIM S20 MIPS simulator.
   Code to read a MIPS a.out file.
   Copyright (C) 1990-1994 by James Larus (larus@cs.wisc.edu).
   ALL RIGHTS RESERVED.

   SPIM is distributed under the following conditions:

     You may make copies of SPIM for your own use and modify those copies.

     All copies of SPIM must retain my name and copyright notice.

     You may not sell SPIM or distributed SPIM in conjunction with a
     commerical product or service without the expressed written consent of
     James Larus.

   THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE. */


/* $Header: /var/home/larus/Software/larus/SPIM/RCS/read-aout.c,v 3.18 1994/02/01 20:41:25 larus Exp $
*/


#include <sst_config.h>
#include <sst_element_config.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_LIBELF_H
#include <gelf.h>
#elif (HAVE_LIBELF_LIBELF_H == 1)
// Macports installs in an odd place
#include <libelf/gelf.h>
#else
#error "No libelf headers found"
#endif

#include "mips_4kc.h"

using namespace SST;
using namespace SST::MIPS4KCComponent;

/*#include "spim.h"
#include "spim-utils.h"
#include "inst.h"
#include "mem.h"
#include "data.h"
#include "read-aout.h"
#include "sym-tbl.h"*/


/* Exported Variables: */

/* Imported Variables: */



/* Read a MIPS executable file.  Return zero if successful and
   non-zero otherwise. */

int MIPS4KC::read_aout_file (const char *file_name)
{
    // load instructions, pre-decoded 
    
    if (elf_version(EV_CURRENT) == EV_NONE) {
        out.fatal(CALL_INFO,-1, "ELF library initialization failed: %s\n", 
                  elf_errmsg(-1));
    }

    int fd;
    if ((fd = open(file_name, O_RDONLY, 0)) < 0) {
        out.fatal(CALL_INFO,-1, "ELF file open failed: x%sx\n", file_name);
    }

    Elf *e;
    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
        out.fatal(CALL_INFO,-1,"elf_begin() failed: %s.",elf_errmsg(-1));
    }

    if (elf_kind(e) != ELF_K_ELF) {
        out.fatal(CALL_INFO,-1, "file is not an ELF object.");
    }

    Elf32_Ehdr *ehdr;
    if ((ehdr = elf32_getehdr(e)) == NULL) {
        out.fatal(CALL_INFO,-1, "getehdr() failed: %s.",elf_errmsg(-1));
        if (ehdr->e_machine != EM_MIPS) {
            out.fatal(CALL_INFO,-1, "Executable is not MIPS\n");
        }
        if (ehdr->e_type != ET_EXEC) {
            out.fatal(CALL_INFO,-1, "File is not Executable\n");
        }
        if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
            out.fatal(CALL_INFO,-1, "Exdecutable is not 32-bit\n");
        }
        if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
            out.fatal(CALL_INFO,-1, "Exdecutable is LSB Endianness\n");
        }
    }

    size_t shstrndx;
    if (elf_getshdrstrndx(e, &shstrndx) != 0) 
        out.fatal(CALL_INFO,-1, "elf getshdrstrndx() failed:␣%s.", elf_errmsg(-1));

    Elf_Scn *scn=NULL;
    GElf_Shdr shdr;
    mem_addr lowData = 0xffffffff;
    bool lowDataSet = 0;
    while ((scn = elf_nextscn(e, scn)) != NULL) {
        if (gelf_getshdr(scn, &shdr) != &shdr) 
            out.fatal(CALL_INFO,-1 , "getshdr() failed:␣%s.", elf_errmsg(-1));

        char *name;
        if ((name = elf_strptr(e, shstrndx , shdr.sh_name)) == NULL)
            out.fatal(CALL_INFO,-1 , "elf_strptr() failed:␣%s.", 
                      elf_errmsg(-1));

        if (outputLevel > 0) {
            printf("Section %4lu %s Size=%lu Flags= %s%s%s Addr=%lx\n", 
                   elf_ndxscn(scn), name, shdr.sh_size,
                   (shdr.sh_flags & SHF_WRITE) ? "Write " : "",
                   (shdr.sh_flags & SHF_ALLOC) ? "Alloc " : "",
                   (shdr.sh_flags & SHF_EXECINSTR) ? "Exec " : "",
                   shdr.sh_addr);
        }

                    
        // if .text section, set the program_starting_address
        // NOTE: this assumes built with -nostartfiles
        if (strncmp(name,".text",5) == 0) {
            if (outputLevel > 0) {
                printf("Detected INIT section %lx\n", shdr.sh_addr);
            }
            program_starting_address = shdr.sh_addr;
        }

        // if reginfo section, get the $gp
        if (strncmp(name,".reginfo",8) == 0) {
            if (outputLevel > 0) {
                printf(" Detected .reginfo section %lx\n", shdr.sh_addr);
            }
            Elf_Data *data = NULL;
            while((data = elf_getdata(scn, data)) != NULL) {
                if (outputLevel > 0) {
                    printf(" .reginfo Data: align %lu, off %llu, size %lu\n", 
                           data->d_align, data->d_off, data->d_size);
                }
                assert(data->d_size == 24);
                uint32_t* wp = (uint32_t*)(data->d_buf) + 5;
                R[REG_GP] = *wp; // set $gp
                if (outputLevel > 0) {
                    printf(" Setting $gp to 0x%x\n", R[REG_GP].getData());
                }
            }
        }
        
        // load executable sections
        if (shdr.sh_flags & SHF_EXECINSTR) {
            mem_addr addr = shdr.sh_addr;
            Elf_Data *data = NULL;
            while((data = elf_getdata(scn, data)) != NULL) {
                size_t n=0;
                if (outputLevel > 0) {
                    printf(" Text Data: align %lu, off %llu, size %lu\n", 
                           data->d_align, data->d_off, data->d_size);
                }
                while (n < shdr.sh_size) {
                    //pointer to instruction
                    uint32_t* wp = (uint32_t*) data->d_buf + (n>>2);
                    // make instruction
                    instruction *add_inst = inst_decode(*wp); 
                    store_instruction(add_inst, addr); // store
                    addr += 4; // advance
                    n += 4;
                }
                if (outputLevel > 0) {
                    printf(" Last text %x address\n", addr-4);
                }
                last_text_addr = addr-4;
            }
        } else if (shdr.sh_flags & SHF_ALLOC) {
            if ((strncmp(name,".MIPS.abiflags",14) == 0) ||
                (strncmp(name,".eh_frame",9) == 0)) {
                continue; // ignore these sections
            }

            // if this is the first data section
            mem_addr addr = shdr.sh_addr;
            if (addr < lowData) {
                if (lowDataSet) {
                    // we probable make some unsafe assumptions about
                    // the layout of the executable
                    out.fatal(CALL_INFO,-1 , 
                              "start of data segment already set\n"); 
                }
                if (outputLevel > 0) {
                    printf("setting start of data section to %x\n", addr);
                }
                //R[REG_GP] = addr+32752; // set $gp
                DATA_BOT = addr; // set bottom of the text 
                data_top = DATA_BOT + initial_data_size;
                lowData = addr;
                lowDataSet = 1;
            }

            // load data
            Elf_Data *data = NULL;
            uint8_t zero = 0;
            while((data = elf_getdata(scn, data)) != NULL) {
                size_t n=0;
                if (outputLevel > 0) {
                    printf(" Data Data: align %lu, off %llu, size %lu\n", 
                           data->d_align, data->d_off, data->d_size);
                }
                while (n < shdr.sh_size) {
                    //pointer to data
                    uint8_t* wp;
                    if (data->d_buf) {
                        // load from the file
                        wp = (uint8_t*) data->d_buf + n;
                    } else {
                        // load zeros (e.g. the .bss segment)
                        wp = &zero;
                    }
                    store_byte(*wp, addr); // store
                    image[addr] = *wp; // record for later storage
                    if (outputLevel > 0) {
                        printf("store data %x to %x\n", *wp, addr);
                    }
                    addr += 1; // advance
                    n += 1;
                }
            }
        }
    }

    elf_end(e);
    close(fd);

#if 0
    //text_begins_at_point(0x1000); //(header.text_start);
    //store_instruction(inst_decode (getw (fd)));
    instruction *add_inst = inst_decode(0x0221820); // ADD r3, r1, r2
    store_instruction(add_inst);

     add_inst = inst_decode(0x0221820); // ADD r3, r1, r2
    store_instruction(add_inst);

     add_inst = inst_decode(0x0221820); // ADD r3, r1, r2
    store_instruction(add_inst);

     add_inst = inst_decode(0x0221820); // ADD r3, r1, r2
    store_instruction(add_inst);

     add_inst = inst_decode(0x0221820); // ADD r3, r1, r2
    store_instruction(add_inst);

    add_inst = inst_decode(0x0221820); // ADD r3, r1, r2
    store_instruction(add_inst);

    printf("Reading...\n");
    print_inst(TEXT_BOT);
    program_starting_address = TEXT_BOT; //header.entry;
#endif

    // TODO load data
    //data_begins_at_point (header.data_start);
    //program_break = header.bss_start + header.bsize;
    
    
    return (0);
}
