/* SPIM S20 MIPS simulator.
   Code to create, maintain and access memory.
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


/* $Header: /var/home/larus/Software/larus/SPIM/RCS/mem.c,v 3.29 1994/07/14 19:48:32 larus Exp $
*/

#include <sst_config.h>
#include "mips_4kc.h"

using namespace SST;
using namespace SST::MIPS4KCComponent;

#if 0
/* Translate from SPIM memory address to physical address */
/* returns a pointer to the region of host memory */
mem_word* MIPS4KC::MEM_ADDRESS_PTR(mem_addr ADDR) {   
    mem_addr _addr_ = (mem_addr) (ADDR);

    if (_addr_ >= DATA_BOT && _addr_ < data_top) {
        return &data_seg[_addr_ - DATA_BOT];
    } else if (_addr_ >= stack_bot && _addr_ < STACK_TOP) {
        return &stack_seg[_addr_ - stack_bot];
    } else {      
        run_error ("Memory address out of bounds or kernel or instruction range\n");
    }
}
#endif


/* Memory is allocated in five chunks:
	text, data, stack, kernel text, and kernel data.

   The arrays are independent and have different semantics.

   text is allocated from 0x400000 up and only contains INSTRUCTIONs.
   It does not expand.

   data is allocated from 0x10000000 up.  It can be extended by the
   SBRK system call.  Programs can only read and write this segment.

   stack grows from 0x7fffefff down.  It is automatically extended.
   Programs can only read and write this segment.

   k_text is like text, except its is allocated from 0x80000000 up.

   k_data is like data, but is allocated from 0x90000000 up.

   Both kernel text and kernel data can only be accessed in kernel mode.
*/


void MIPS4KC::make_memory (long int text_size, long int data_size, long int data_limit,
	long int stack_size, long int stack_limit, long int k_text_size,
	long int k_data_size, long int k_data_limit)
{
  if (data_size <= 65536)
    data_size = 65536;
  if (text_seg == NULL)
    text_seg = (instruction **)
      xmalloc (sizeof (instruction *) * text_size / BYTES_PER_WORD);
  else
    {
      free_instructions (text_seg, (text_top - TEXT_BOT) / BYTES_PER_WORD);
      text_seg = (instruction **) realloc (text_seg, text_size);
    }
  memclr (text_seg, sizeof (instruction *) * text_size / BYTES_PER_WORD);
  text_top = TEXT_BOT + text_size;

  if (data_seg == NULL)
    data_seg =
      (mem_word *) xmalloc (sizeof (mem_word) * data_size / BYTES_PER_WORD);
  memclr (data_seg, sizeof (mem_word) * data_size / BYTES_PER_WORD);
  data_seg_b = (BYTE_TYPE *) data_seg;
  data_seg_h = (short *) data_seg;
  data_top = DATA_BOT + data_size;
  data_size_limit = data_limit;

  if (stack_seg == NULL)
    stack_seg =
      (mem_word *) xmalloc (sizeof (mem_word) * stack_size / BYTES_PER_WORD);
  memclr (stack_seg, sizeof (mem_word) * stack_size / BYTES_PER_WORD);
  stack_seg_b = (BYTE_TYPE *) stack_seg;
  stack_seg_h = (short *) stack_seg;
  stack_bot = STACK_TOP - stack_size;
  stack_size_limit = stack_limit;

  if (k_text_seg == NULL)
    k_text_seg = (instruction **)
      xmalloc (sizeof (instruction *) * k_text_size / BYTES_PER_WORD);
  else
    free_instructions (k_text_seg, (k_text_top - K_TEXT_BOT) / BYTES_PER_WORD);
  memclr (k_text_seg, sizeof (instruction *) * k_text_size / BYTES_PER_WORD);
  k_text_top = K_TEXT_BOT + k_text_size;

  if (k_data_seg == NULL)
    k_data_seg =
      (mem_word *) xmalloc (sizeof (mem_word) * k_data_size / BYTES_PER_WORD);
  memclr (k_data_seg, sizeof (mem_word) * k_data_size / BYTES_PER_WORD);
  k_data_seg_b = (BYTE_TYPE *) k_data_seg;
  k_data_seg_h = (short *) k_data_seg;
  k_data_top = K_DATA_BOT + k_data_size;
  k_data_size_limit = k_data_limit;

  text_modified = 0;
  data_modified = 0;
}


/* Free the storage used by the old instructions in memory. */

void MIPS4KC::free_instructions (instruction **inst, int n)
{
    printf("FREEING Instructions\n");

  for ( ; n > 0; n --, inst ++)
    if (*inst)
      free_inst (*inst);
}


/* Expand the data segment by adding N bytes. */

void MIPS4KC::expand_data (long int addl_bytes)
{
  long old_size = data_top - DATA_BOT;
  long new_size = old_size + addl_bytes;
  mem_word *p;

  if (addl_bytes < 0) // || (source_file && new_size > data_size_limit))
    {
      error ("Can't expand data segment by %d bytes to %d bytes\n",
	     addl_bytes, new_size);
      run_error ("Use -ldata # with # > %d\n", new_size);
    }
  data_seg = (mem_word *) realloc (data_seg, new_size);
  if (data_seg == NULL)
    fatal_error ("realloc failed in expand_data\n");
  data_seg_b = (BYTE_TYPE *) data_seg;
  data_seg_h = (short *) data_seg;
  for (p = data_seg + old_size / BYTES_PER_WORD;
       p < data_seg + new_size / BYTES_PER_WORD; )
    *p ++ = 0;
  data_top += addl_bytes;
}


/* Expand the stack segment by adding N bytes.  Can't use REALLOC
   since it copies from bottom of memory blocks and stack grows down from
   top of its block. */

void MIPS4KC::expand_stack (long int addl_bytes)
{
  long old_size = STACK_TOP - stack_bot;
  long new_size = old_size + MAX (addl_bytes, old_size);
  mem_word *new_seg;
  mem_word *po, *pn;

  if (addl_bytes < 0) // || (source_file && new_size > stack_size_limit))
    {
      error ("Can't expand stack segment by %d bytes to %d bytes\n",
	     addl_bytes, new_size);
      run_error ("Use -lstack # with # > %d\n", new_size);
    }

  new_seg = (mem_word *) xmalloc (new_size);
  po = stack_seg + (old_size / BYTES_PER_WORD - 1);
  pn = new_seg + (new_size / BYTES_PER_WORD - 1);

  for ( ; po >= stack_seg ; ) *pn -- = *po --;
  for ( ; pn >= new_seg ; ) *pn -- = 0;

  free (stack_seg);
  stack_seg = new_seg;
  stack_seg_b = (BYTE_TYPE *) stack_seg;
  stack_seg_h = (short *) stack_seg;
  stack_bot -= (new_size - old_size);
}


/* Expand the kernel data segment by adding N bytes. */

void MIPS4KC::expand_k_data (long int addl_bytes)
{
  long old_size = k_data_top - K_DATA_BOT;
  long new_size = old_size + addl_bytes;
  mem_word *p;

  if (addl_bytes < 0) // || (source_file && new_size > k_data_size_limit))
    {
      error ("Can't expand kernel data segment by %d bytes to %d bytes\n",
	     addl_bytes, new_size);
      run_error ("Use -lkdata # with # > %d\n", new_size);
    }
  k_data_seg = (mem_word *) realloc (k_data_seg, new_size);
  if (k_data_seg == NULL)
    fatal_error ("realloc failed in expand_k_data\n");
  k_data_seg_b = (BYTE_TYPE *) k_data_seg;
  k_data_seg_h = (short *) k_data_seg;
  for (p = k_data_seg + old_size / BYTES_PER_WORD;
       p < k_data_seg + new_size / BYTES_PER_WORD; )
    *p ++ = 0;
  k_data_top += addl_bytes;
}



/* Handle the infrequent and erroneous cases in the memory access macros. */
instruction * MIPS4KC::cl_bad_text_read (mem_addr addr, int *excpt)
{
  mem_word bits;

  out.output(CALL_INFO, "bad cl_text read @ %lld\n", reg_word::getNow());
  READ_MEM_WORD (bits, addr);
  return (inst_decode (bits));
}

instruction * MIPS4KC::bad_text_read (mem_addr addr)
{
  mem_word bits;

  out.output(CALL_INFO, "bad text read @ %lld\n", reg_word::getNow());
  READ_MEM_WORD (bits, addr);
  return (inst_decode (bits));
}


void MIPS4KC::bad_text_write (mem_addr addr, instruction *inst)
{
  SET_MEM_WORD (addr, ENCODING (inst));
}


mem_word MIPS4KC::bad_mem_read (mem_addr addr, int mask, mem_word *dest)
{
  mem_word tmp;

  if (addr & mask)
    RAISE_EXCEPTION (ADDRL_EXCPT, BadVAddr = addr)
  else if (addr >= TEXT_BOT && addr < text_top)
    switch (mask)
      {
      case 0x0:
	tmp = ENCODING (text_seg [(addr - TEXT_BOT) >> 2]);
#ifdef BIGENDIAN
	tmp = (unsigned)tmp >> (8 * (3 - (addr & 0x3)));
#else
	tmp = (unsigned)tmp >> (8 * (addr & 0x3));
#endif
	return (0xff & tmp);

      case 0x1:
	tmp = ENCODING (text_seg [(addr - TEXT_BOT) >> 2]);
#ifdef BIGENDIAN
	tmp = (unsigned)tmp >> (8 * (2 - (addr & 0x2)));
#else
	tmp = (unsigned)tmp >> (8 * (addr & 0x2));
#endif
	return (0xffff & tmp);

      case 0x3:
	return (ENCODING (text_seg [(addr - TEXT_BOT) >> 2]));

      default:
	run_error ("Bad mask (0x%x) in bad_mem_read\n", mask);
      }
  else if (addr > data_top
	   && addr < stack_bot
	   /* If more than 16 MB below stack, probably is bad data ref */
	   && addr > stack_bot - 16*1000*K)
    {
      /* Grow stack segment */
      expand_stack (stack_bot - addr + 4);
      *dest = 0;		/* Newly allocated memory */
      return (0);
    }
  else if (MM_IO_BOT <= addr && addr <= MM_IO_TOP)
    return (read_memory_mapped_IO (addr));
  else
    /* Address out of range */
    RAISE_EXCEPTION (DBUS_EXCPT, BadVAddr = addr)
  return (0);
}


void MIPS4KC::bad_mem_write (mem_addr addr, mem_word value, int mask)
{
  mem_word tmp;

  if (addr & mask)
    /* Unaligned address fault */
    RAISE_EXCEPTION (ADDRS_EXCPT, BadVAddr = addr)
  else if (addr >= TEXT_BOT && addr < text_top)
    switch (mask)
      {
      case 0x0:
	tmp = ENCODING (text_seg [(addr - TEXT_BOT) >> 2]);
#ifdef BIGENDIAN
	tmp = ((tmp & ~(0xff << (8 * (3 - (addr & 0x3)))))
	       | (value & 0xff) << (8 * (3 - (addr & 0x3))));
#else
	tmp = ((tmp & ~(0xff << (8 * (addr & 0x3))))
	       | (value & 0xff) << (8 * (addr & 0x3)));
#endif
	text_seg [(addr - TEXT_BOT) >> 2] = inst_decode (tmp);
	break;

      case 0x1:
	tmp = ENCODING (text_seg [(addr - TEXT_BOT) >> 2]);
#ifdef BIGENDIAN
	tmp = ((tmp & ~(0xffff << (8 * (2 - (addr & 0x2)))))
	       | (value & 0xffff) << (8 * (2 - (addr & 0x2))));
#else
	tmp = ((tmp & ~(0xffff << (8 * (addr & 0x2))))
	       | (value & 0xffff) << (8 * (addr & 0x2)));
#endif
	text_seg [(addr - TEXT_BOT) >> 2] = inst_decode (tmp);
	break;

      case 0x3:
	text_seg [(addr - TEXT_BOT) >> 2] = inst_decode (value);
	break;

      default:
	run_error ("Bad mask (0x%x) in bad_mem_read\n", mask);
      }
  else if (addr > data_top
	   && addr < stack_bot
	   /* If more than 16 MB below stack, probably is bad data ref */
	   && addr > stack_bot - 16*1000*K)
    {
      /* Grow stack segment */
      expand_stack (stack_bot - addr + 4);
      if (addr >= stack_bot)
	{
	  if (mask == 0)
	    stack_seg_b [addr - stack_bot] = value;
	  else if (mask == 1)
	    stack_seg_h [(addr - stack_bot) > 1] = value;
	  else
	    stack_seg [(addr - stack_bot) >> 2] = value;
	}
      else
	RAISE_EXCEPTION (DBUS_EXCPT, BadVAddr = addr)
    }
  else if (MM_IO_BOT <= addr && addr <= MM_IO_TOP)
    write_memory_mapped_IO (addr, value);
  else
    /* Address out of range */
    RAISE_EXCEPTION (DBUS_EXCPT, BadVAddr = addr)
}



/* Handle infrequent and erroneous cases in cycle-level SPIM memory
   access macros. */

void MIPS4KC::cl_bad_text_write (mem_addr addr, instruction *inst, int *excpt)
{
  SET_MEM_WORD (addr, ENCODING (inst));
}


mem_word MIPS4KC::cl_bad_mem_read (mem_addr addr, int mask, mem_word *dest, int *excpt)
{
  if (addr & mask){
    CL_RAISE_EXCEPTION (ADDRL_EXCPT, 0, *excpt);
    BadVAddr = addr;
  }
  else if (addr >= TEXT_BOT && addr < text_top)
    {
      if (mask != 0x3)
	run_error ("SPIM restriction: Can only read text segment by words\n");
      else
	return (ENCODING (text_seg [(addr - TEXT_BOT) >> 2]));
    }
  else if (addr > data_top
	   && addr < stack_bot
	   /* If more than 16 MB below stack, probably is bad data ref */
	   && addr > stack_bot - 16*1000*K)
    {
      /* Grow stack segment */
      expand_stack (stack_bot - addr + 4);
      *dest = 0;				 /* Newly allocated memory */
    }
  else {
    /* Address out of range */
    CL_RAISE_EXCEPTION (DBUS_EXCPT, 0, *excpt);
    BadVAddr = addr;
  }
  return (0);

}


void MIPS4KC::cl_bad_mem_write (mem_addr addr, mem_word value, int mask, int *excpt)
{
  if (addr & mask) {
    /* Unaligned address fault */
    CL_RAISE_EXCEPTION (ADDRS_EXCPT, 0, *excpt);
    BadVAddr = addr;
  }
  else if (addr >= TEXT_BOT && addr < text_top)
    {
      if (mask != 0x3)
	run_error ("SPIM restriction: Can only write text segment by words\n");
      else
	text_seg [(addr - TEXT_BOT) >> 2] = inst_decode (value);
    }
  else if (addr > data_top
	   && addr < stack_bot
	   /* If more than 16 MB below stack, probably is bad data ref */
	   && addr > stack_bot - 16*1000*K)
    {
      /* Grow stack segment */
      expand_stack (stack_bot - addr + 4);
      if (addr >= stack_bot)
	{
	  if (mask == 0)
	    stack_seg_b [addr - stack_bot] = value;
	  else if (mask == 1)
	    stack_seg_h [addr - stack_bot] = value;
	  else
	    stack_seg [addr - stack_bot] = value;
	}
      else {
	CL_RAISE_EXCEPTION (DBUS_EXCPT, 0, *excpt);
        BadVAddr = addr;
      }
    }
  else {
    /* Address out of range */
    CL_RAISE_EXCEPTION (DBUS_EXCPT, 0, *excpt);
    BadVAddr = addr;
    }
}







/* Every IO_INTERVAL time steps, check if input is available and output
   is possible.  If so, update the control registers and buffers. */

void MIPS4KC::check_memory_mapped_IO (void)
{
  static long mm_io_initialized = 0;

  if (!mm_io_initialized)
    {
      recv_control = RECV_READY;
      trans_control = TRANS_READY;
      mm_io_initialized = 1;
    }

  // AFR if (console_input_available ())
  if (0) 
    {
      recv_buffer_filled -= IO_INTERVAL;
      if (recv_buffer_filled <= 0)
	{
            //recv_buffer = get_console_char ();
	  recv_control |= RECV_READY;
	  recv_buffer_filled = RECV_LATENCY;
	  if ((recv_control & RECV_INT_ENABLE)
	      && INTERRUPTS_ON.getData()
	      && (Status_Reg & RECV_INT_MASK).getData())
	    RAISE_EXCEPTION (INT_EXCPT, Cause |= RECV_INT_MASK);
	}
    }
  else if (recv_buffer_filled <= 0)
    recv_control &= ~RECV_READY;

  if (trans_buffer_filled > 0)
    {
      trans_buffer_filled -= IO_INTERVAL;
      if (trans_buffer_filled <= 0)
	{
            // AFR put_console_char (trans_buffer);
	  trans_control |= TRANS_READY;
	  trans_buffer_filled = 0;
	  if ((trans_control & TRANS_INT_ENABLE)
	      && INTERRUPTS_ON.getData()
	      && (Status_Reg & TRANS_INT_MASK).getData())
	    RAISE_EXCEPTION (INT_EXCPT, Cause |= TRANS_INT_MASK)
	}
    }
}


/* Invoked on a write in the memory-mapped IO area. */

void MIPS4KC::write_memory_mapped_IO (mem_addr addr, mem_word value)
{
  switch (addr)
    {
    case TRANS_CTRL_ADDR:
      trans_control = ((trans_control & ~TRANS_INT_ENABLE)
		       | (value & TRANS_INT_ENABLE));

      if ((trans_control & TRANS_READY)
	  && (trans_control & TRANS_INT_ENABLE)
	  && INTERRUPTS_ON.getData()
	  && (Status_Reg & TRANS_INT_MASK).getData())
	/* Raise an interrupt immediately on enabling a ready xmitter */
	RAISE_EXCEPTION (INT_EXCPT, Cause |= TRANS_INT_MASK)
      break;

    case TRANS_BUFFER_ADDR:
      if (trans_control & TRANS_READY) /* Ignore if not ready */
	{
	  trans_buffer = value & 0xff;
	  trans_control &= ~TRANS_READY;
	  trans_buffer_filled = TRANS_LATENCY;
	}
      break;

    case RECV_CTRL_ADDR:
      recv_control = ((recv_control & ~RECV_INT_ENABLE)
		      | (value & RECV_INT_ENABLE));
      break;

    case RECV_BUFFER_ADDR:
      break;

    default:
      run_error ("Write to unused memory-mapped IO address (0x%x)\n",
		 addr);
    }
}


/* Invoked on a read in the memory-mapped IO area. */

mem_word MIPS4KC::read_memory_mapped_IO (mem_addr addr)
{
  switch (addr)
    {
    case TRANS_CTRL_ADDR:
      return (trans_control);

    case TRANS_BUFFER_ADDR:
      return (trans_buffer & 0xff);

    case RECV_CTRL_ADDR:
      return (recv_control);

    case RECV_BUFFER_ADDR:
      recv_control &= ~RECV_READY;
      recv_buffer_filled = 0;
      return (recv_buffer & 0xff);

    default:
      run_error ("Read from unused memory-mapped IO address (0x%x)\n",
		 addr);
      return (0);
    }
}



/* Misc. routines */

void MIPS4KC::print_mem (mem_addr addr)
{
  long value;

  if (TEXT_BOT <= addr && addr < text_top)
    print_inst (addr);
  else if (DATA_BOT <= addr && addr < data_top)
    {
      READ_MEM_WORD (value, addr);
      write_output (message_out, "Data seg @ 0x%08x (%d) = 0x%08x (%d)\n",
		    addr, addr, value, value);
    }
  else if (stack_bot <= addr && addr < STACK_TOP)
    {
      READ_MEM_WORD (value, addr);
      write_output (message_out, "Stack seg @ 0x%08x (%d) = 0x%08x (%d)\n",
		    addr, addr, value, value);
    }
  else if (K_TEXT_BOT <= addr && addr < k_text_top)
    print_inst (addr);
  else if (K_DATA_BOT <= addr && addr < k_data_top)
    {
      READ_MEM_WORD (value, addr);
      write_output (message_out,
		    "Kernel Data seg @ 0x%08x (%d) = 0x%08x (%d)\n",
		    addr, addr, value, value);
    }
  else
    error ("Address 0x%08x (%d) to print_mem is out of bounds\n", addr, addr);
}
