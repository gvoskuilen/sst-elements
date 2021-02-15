/* SPIM S20 MIPS Cycle Level simulator.
   Definitions for the SPIM S20 Cycle Level Simulator (SPIM-CL).
   Copyright (C) 1991-1992 by Anne Rogers (amr@cs.princeton.edu) and
   Scott Rosenberg (scottr@cs.princeton.edu)
   ALL RIGHTS RESERVED.

   SPIM-CL is distributed under the following conditions:

     You may make copies of SPIM-CL for your own use and modify those copies.

     All copies of SPIM-CL must retain our names and copyright notice.

     You may not sell SPIM-CL or distributed SPIM-CL in conjunction with a
     commerical product or service without the expressed written consent of
     Anne Rogers.

   THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.
*/

#include <sst_config.h>
#include "mips_4kc.h"

using namespace SST;
using namespace SST::MIPS4KCComponent;

void MIPS4KC::CL_READ_MEM_INST(instruction* &LOC, const reg_word &ADDR,
                                   mem_addr &PADDR, 
                                   int &EXPT) {
    mem_addr _addr_ = (mem_addr) (ADDR).getData(); 
    unsigned int tmp = tlb_vat(ADDR.getData(), 0, 1, &PADDR);
    if (_addr_ >= TEXT_BOT && _addr_ < text_top && !(_addr_ & 0x3))
        LOC = text_seg [(_addr_ - TEXT_BOT) >> 2];                 
    else if (_addr_ >= K_TEXT_BOT && _addr_ < k_text_top && !(_addr_ & 0x3)) 
        LOC = k_text_seg [(_addr_ - K_TEXT_BOT) >> 2];
    else {
        CL_RAISE_EXCEPTION(tmp, 0, EXPT);
    }
}

void MIPS4KC::CL_READ_MEM(reg_word &LOC, const reg_word &ADDR,
                          mem_addr &PADDR, int &EXPT, 
                          const memReq *req, size_t sz) {
    const mem_addr &_addr_ = ADDR.getData();
    unsigned int tmp = tlb_vat(ADDR.getData(), 0, 1, &PADDR);
    tmp = DBUS_EXCPT; // exception to use if needed

    if (sz == 1) {
        if (_addr_ >= DATA_BOT && _addr_ < data_top)
            LOC = data_seg_b [_addr_ - DATA_BOT];
        else if (_addr_ >= stack_bot && _addr_ < STACK_TOP)
            LOC = stack_seg_b [_addr_ - stack_bot];
        else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top)
            LOC = k_data_seg_b [_addr_ - K_DATA_BOT];
        else CL_RAISE_EXCEPTION(tmp, 0, EXPT);
    } else if (sz == 2) {
        if (_addr_ >= DATA_BOT && _addr_ < data_top)
            LOC = data_seg_h [(_addr_ - DATA_BOT)>>1];
        else if (_addr_ >= stack_bot && _addr_ < STACK_TOP)
            LOC = stack_seg_h [(_addr_ - stack_bot)>>1];
        else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top)
            LOC = k_data_seg_h [(_addr_ - K_DATA_BOT)>>1];
        else CL_RAISE_EXCEPTION(tmp, 0, EXPT);
    } else if (sz == 4) {
        if (_addr_ >= DATA_BOT && _addr_ < data_top)
            LOC = data_seg [(_addr_ - DATA_BOT)>>2];
        else if (_addr_ >= stack_bot && _addr_ < STACK_TOP)
            LOC = stack_seg [(_addr_ - stack_bot)>>2];
        else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top)
            LOC = k_data_seg [(_addr_ - K_DATA_BOT)>>2];
        else {
            CL_RAISE_EXCEPTION(tmp, 0, EXPT);
        }
    }

    assert(sz == req->data.size());
    assert(sz <= 4);

    int32_t data = 0;
    for (int i = sz-1; i >= 0; --i) {
        data <<= 8;
        data |= req->data[i];
    }
    if ((outputLevel > 0) && (LOC.getData() != data)) {
        // seems to be caused by unaligned memory. not sure if error
        static int c = 0;
        c++;
        if (c < 100) {
            printf("READ: %08x %08x sz%lu ?\n", LOC.getData(), data, sz); 
        }
    }
    //assert(LOC.getData() == data); 

    // if read is from 'kernel' memory, we use the incoming data since
    // it may be comgin from a different processor (or UART)
    if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top) {
        LOC = data;
        // should we check this data and record faults?
    } else {    
        // note incoming faulted data and add in the data
        LOC.checkReadForFaults(ADDR,data,sz);
    }
}

void MIPS4KC::CL_SET_MEM(const reg_word &ADDR, mem_addr &PADDR, 
                         const reg_word &VALUE, int &EXPT, 
                         const memReq *req, size_t sz) {
    const mem_addr _addr_ = (mem_addr) (ADDR.getData());
    unsigned int tmp = tlb_vat(ADDR.getData(), 0, 0, &PADDR); 
    tmp = DBUS_EXCPT; // exception to use if needed
    
    data_modified = 1; //needed?

    if (sz == 1) {
        if (_addr_ >= DATA_BOT && _addr_ < data_top)
            data_seg_b [_addr_ - DATA_BOT] = (unsigned char) (VALUE.getData());
        else if (_addr_ >= stack_bot && _addr_ < STACK_TOP)
            stack_seg_b [_addr_ - stack_bot] = (unsigned char) (VALUE.getData()); 
        else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top)  
            k_data_seg_b [_addr_ - K_DATA_BOT] = (unsigned char) (VALUE.getData());
        else CL_RAISE_EXCEPTION(tmp, 0, EXPT);
    } else if (sz == 2) {
        if (_addr_ >= DATA_BOT && _addr_ < data_top)
            data_seg_h [(_addr_ - DATA_BOT)>>1] = (unsigned short) (VALUE.getData());
        else if (_addr_ >= stack_bot && _addr_ < STACK_TOP)
            stack_seg_h [(_addr_ - stack_bot)>>1] = (unsigned short) (VALUE.getData()); 
        else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top)  
            k_data_seg_h [(_addr_ - K_DATA_BOT)>>1] = (unsigned short) (VALUE.getData());
        else CL_RAISE_EXCEPTION(tmp, 0, EXPT);
    } else if (sz == 4) {
        if (_addr_ >= DATA_BOT && _addr_ < data_top)
            data_seg [(_addr_ - DATA_BOT)>>2] = (VALUE.getData());
        else if (_addr_ >= stack_bot && _addr_ < STACK_TOP) {
            stack_seg [(_addr_ - stack_bot)>>2] = (VALUE.getData()); 
        } else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top)  
            k_data_seg [(_addr_ - K_DATA_BOT)>>2] = (VALUE.getData());
        else {
            printf("store cl_except %x %x %x\n", _addr_, K_DATA_BOT, k_data_top);
            CL_RAISE_EXCEPTION(tmp, 0, EXPT);
        }
    }
    
    assert(sz == req->data.size());

    int32_t in = 0;
    for (int i = sz-1; i >= 0; --i) {
        in <<= 8;
        in |= req->data[i];
    }
    assert( VALUE.getData() == in);
    //printf("WRITE: %08x %08x\n", VALUE.getData(), in); 
 }

