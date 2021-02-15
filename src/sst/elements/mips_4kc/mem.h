/* SPIM S20 MIPS simulator.
   Macros for accessing memory.
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



/* A note on directions:  "Bottom" of memory is the direction of
   decreasing addresses.  "Top" is the direction of increasing addresses.*/

#ifndef _MEM_H
#define  _MEM_H

/* Type of contents of a memory word. */

//typedef long mem_word;
typedef int32_t mem_word;






#define TEXT_BOT ((mem_addr) 0x400000)



/* Amount to grow text segment when we run out of space for instructions. */

#define TEXT_CHUNK_SIZE	4096


/* The data segment and boundaries. */


#define BYTE_TYPE char

/* Non-ANSI C compilers do not like signed chars.  You can change it to
   'char' if the compiler will treat chars as signed values... */

#if ((defined (sun) || defined (hpux)) && !defined(__STDC__))
/* Sun and HP cc compilers: */
#undef BYTE_TYPE
#define BYTE_TYPE char
#endif


//#define DATA_BOT ((mem_addr) 0x10000000)



/* Exclusive, but include 4K at top of stack. */

//#define STACK_TOP ((mem_addr) 0x80000000)




#define K_TEXT_BOT ((mem_addr) 0x80000000)





#define K_DATA_BOT ((mem_addr) 0x90000000)



/* Memory-mapped IO area. */

#define MM_IO_BOT ((mem_addr) 0xffff0000)

#define MM_IO_TOP ((mem_addr) 0xffffffff)


#define RECV_CTRL_ADDR ((mem_addr) 0xffff0000)

#define RECV_READY 0x1
#define RECV_INT_ENABLE 0x2

#define RECV_INT_MASK 0x100

#define RECV_BUFFER_ADDR ((mem_addr) 0xffff0004)


#define TRANS_CTRL_ADDR ((mem_addr) 0xffff0008)

#define TRANS_READY 0x1
#define TRANS_INT_ENABLE 0x2

#define TRANS_INT_MASK 0x200

#define TRANS_BUFFER_ADDR ((mem_addr) 0xffff000c)



/* You would think that a compiler could perform CSE on the arguments to
   these macros.  However, complex expressions break some compilers, so
   do the CSE ourselves. */

#define READ_MEM_INST(LOC, ADDR)					   \
{mem_addr _addr_ = (mem_addr) (ADDR);				   \
   if (_addr_ >= TEXT_BOT && _addr_ < text_top && !(_addr_ & 0x3))	   \
     LOC = text_seg [(_addr_ - TEXT_BOT) >> 2];				   \
   else if (_addr_ >= K_TEXT_BOT && _addr_ < k_text_top && !(_addr_ & 0x3))\
     LOC = k_text_seg [(_addr_ - K_TEXT_BOT) >> 2];			   \
   else LOC = bad_text_read (_addr_);}


#define READ_MEM_BYTE(LOC, ADDR)					   \
{mem_addr _addr_ = (mem_addr) (ADDR);				   \
   if (_addr_ >= DATA_BOT && _addr_ < data_top)				   \
    LOC = data_seg_b [_addr_ - DATA_BOT];				   \
   else if (_addr_ >= stack_bot && _addr_ < STACK_TOP)			   \
     LOC = stack_seg_b [_addr_ - stack_bot];				   \
   else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top)		   \
    LOC = k_data_seg_b [_addr_ - K_DATA_BOT];				   \
   else									   \
     LOC = bad_mem_read (_addr_, 0, (mem_word *)&LOC);}


#define READ_MEM_HALF(LOC, ADDR)					   \
{mem_addr _addr_ = (mem_addr) (ADDR);				   \
   if (_addr_ >= DATA_BOT && _addr_ < data_top && !(_addr_ & 0x1))	   \
     LOC = data_seg_h [(_addr_ - DATA_BOT) >> 1];			   \
  else if (_addr_ >= stack_bot && _addr_ < STACK_TOP && !(_addr_ & 0x1))   \
    LOC = stack_seg_h [(_addr_ - stack_bot) >> 1];			   \
   else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top && !(_addr_ & 0x1))\
     LOC = k_data_seg_h [(_addr_ - K_DATA_BOT) >> 1];			   \
  else									   \
    LOC = bad_mem_read (_addr_, 0x1, (mem_word *)&LOC);}


#define READ_MEM_WORD(LOC, ADDR)					   \
{mem_addr _addr_ = (mem_addr) (ADDR);				   \
   if (_addr_ >= DATA_BOT && _addr_ < data_top && !(_addr_ & 0x3))	   \
     LOC = data_seg [(_addr_ - DATA_BOT) >> 2];				   \
  else if (_addr_ >= stack_bot && _addr_ < STACK_TOP && !(_addr_ & 0x3))   \
    LOC = stack_seg [(_addr_ - stack_bot) >> 2];			   \
   else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top && !(_addr_ & 0x3))\
     LOC = k_data_seg [(_addr_ - K_DATA_BOT) >> 2];			   \
  else									   \
    LOC = bad_mem_read (_addr_, 0x3, (mem_word *)&LOC);}


#define SET_MEM_INST(ADDR, INST)					   \
{mem_addr _addr_ = (mem_addr) (ADDR);				   \
   text_modified = 1;							   \
   if (_addr_ >= TEXT_BOT && _addr_ < text_top && !(_addr_ & 0x3))	   \
     text_seg [(_addr_ - TEXT_BOT) >> 2] = INST;			   \
   else if (_addr_ >= K_TEXT_BOT && _addr_ < k_text_top && !(_addr_ & 0x3))\
     k_text_seg [(_addr_ - K_TEXT_BOT) >> 2] = INST;			   \
   else bad_text_write (_addr_, INST);}


#define SET_MEM_BYTE(ADDR, VALUE)					   \
{mem_addr _addr_ = (mem_addr) (ADDR);				   \
   data_modified = 1;							   \
   if (_addr_ >= DATA_BOT && _addr_ < data_top)				   \
     data_seg_b [_addr_ - DATA_BOT] = (unsigned char) (VALUE);		   \
   else if (_addr_ >= stack_bot && _addr_ < STACK_TOP)			   \
     stack_seg_b [_addr_ - stack_bot] = (unsigned char) (VALUE);	   \
   else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top)		   \
     k_data_seg_b [_addr_ - K_DATA_BOT] = (unsigned char) (VALUE);	   \
   else bad_mem_write (_addr_, VALUE, 0);}


#define SET_MEM_HALF(ADDR, VALUE)					   \
{mem_addr _addr_ = (mem_addr) (ADDR);				   \
   data_modified = 1;							   \
   if (_addr_ >= DATA_BOT && _addr_ < data_top && !(_addr_ & 0x1))	   \
     data_seg_h [(_addr_ - DATA_BOT) >> 1] = (unsigned short) (VALUE);	   \
   else if (_addr_ >= stack_bot && _addr_ < STACK_TOP && !(_addr_ & 0x1))  \
     stack_seg_h [(_addr_ - stack_bot) >> 1] = (unsigned short) (VALUE);   \
   else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top && !(_addr_ & 0x1))\
     k_data_seg_h [(_addr_ - K_DATA_BOT) >> 1] = (unsigned short) (VALUE); \
   else bad_mem_write (_addr_, VALUE, 0x1);}


#define SET_MEM_WORD(ADDR, VALUE)					   \
{mem_addr _addr_ = (mem_addr) (ADDR);				   \
   data_modified = 1;							   \
   if (_addr_ >= DATA_BOT && _addr_ < data_top && !(_addr_ & 0x3))	   \
     data_seg [(_addr_ - DATA_BOT) >> 2] = (mem_word) (VALUE);		   \
   else if (_addr_ >= stack_bot && _addr_ < STACK_TOP && !(_addr_ & 0x3))  \
     stack_seg [(_addr_ - stack_bot) >> 2] = (mem_word) (VALUE);	   \
   else if (_addr_ >= K_DATA_BOT && _addr_ < k_data_top && !(_addr_ & 0x3))\
     k_data_seg [(_addr_ - K_DATA_BOT) >> 2] = (mem_word) (VALUE);	   \
   else bad_mem_write (_addr_, VALUE, 0x3);}




/* Exported functions: */



#endif //  _MEM_H
