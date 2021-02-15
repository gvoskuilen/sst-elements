/* SPIM S20 MIPS simulator.
   Definitions for the SPIM S20.
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


/* $Header: /var/home/larus/Software/larus/SPIM/RCS/spim.h,v 3.23 1994/07/15 17:53:46 larus Exp $
*/

#ifndef _SPIM_H
#define _SPIM_H

#ifndef NULL
#define NULL 0
#endif


#define streq(s1, s2) !strcmp(s1, s2)


/* Round V to next greatest B boundary */

#define ROUND(V, B) (((int) V + (B-1)) & ~(B-1))


/* Sign-extend a short to a long */

#define SIGN_EX(X) ((X) & 0x8000 ? (X) | 0xffff0000 : (X))


#define MIN(A, B) ((A) < (B) ? (A) : (B))

#define MAX(A, B) ((A) > (B) ? (A) : (B))


#define K 1024


/* Useful and pervasive declarations: */

#ifdef NO_MEM_FUNCTIONS
#define memcpy(T, F, S) bcopy((void*)F, (void*)T, S)
#define memclr(B, S) bzero(B, S)
#define memcmp(S1, S2, N) bcmp(S1, S2, N)
#else
#include <memory.h>
#define memclr(B, S) memset((void*)B, 0, S)
#endif

#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#define QSORT_FUNC int(*)(const void *, const void *)
#else
double atof ();
int atoi ();
int free ();
char *malloc ();
int strcmp ();
char *strcpy ();
char *strncpy ();
#define QSORT_FUNC int(*)()
#endif



/* Type of a memory address. */

//typedef unsigned long mem_addr;
typedef uint32_t mem_addr;


#define BYTES_PER_WORD 4	/* On the MIPS processor */


/* Sizes of memory segments. */

/* Initial size of text segment. */

#ifndef TEXT_SIZE
#define TEXT_SIZE	256*K	/* 1/4 MB */
#endif

/* Initial size of k_text segment. */

#ifndef K_TEXT_SIZE
#define K_TEXT_SIZE	64*K	/* 64 KB */
#endif

/* The data segment must be larger than 64K since we immediate grab
   64K for the small data segment pointed to by $gp. The data segment is
   expanded by an sbrk system call. */

/* Initial size of data segment. */

#ifndef DATA_SIZE
#define DATA_SIZE	256*K	/* 1/4 MB */
#endif

/* Maximum size of data segment. */

#ifndef DATA_LIMIT
#define DATA_LIMIT	1000*K	/* 1 MB */
#endif

/* Initial size of k_data segment. */

#ifndef K_DATA_SIZE
#define K_DATA_SIZE	64*K	/* 64 KB */
#endif

/* Maximum size of k_data segment. */

#ifndef K_DATA_LIMIT
#define K_DATA_LIMIT	1000*K	/* 1 MB */
#endif

/* The stack grows down automatically. */

/* Initial size of stack segment. */

#ifndef STACK_SIZE
#define STACK_SIZE	64*K	/* 64 KB */
#endif

/* Maximum size of stack segment. */

#ifndef STACK_LIMIT
#define STACK_LIMIT	256*K	/* 1 MB */
#endif


/* Name of the function to invoke at start up */

#define DEFAULT_RUN_LOCATION "__start"


/* Default number of instructions to execute. */

#define DEFAULT_RUN_STEPS 2147483647


/* Address to branch to when exception occurs */

#define EXCEPTION_ADDR 0x80000080


/* Maximum size of object stored in the small data segment pointed to by
   $gp */

#define SMALL_DATA_SEG_MAX_SIZE 8

#ifndef DIRECT_MAPPED
#define DIRECT_MAPPED 0
#define TWO_WAY_SET 1
#endif


/* Interval (in instructions) at which memory-mapped IO registers are
   checked and updated.*/

#define IO_INTERVAL 100


/* Number of instructions that a character remains in receiver buffer
   if another character is available. (Should be multiple of IO_INTERVAL.) */

#define RECV_LATENCY (10*IO_INTERVAL)


/* Number of instructions that it takes to write a character. (Should
   be multiple of IO_INTERVAL.)*/

#define TRANS_LATENCY (IO_INTERVAL)



/* Triple containing a string and two integers.	 Used in tables
   mapping from a name to values. */

typedef struct strint
{
    char *name;
    int value1;
    int value2;
    strint(const char *n, int v1, int v2) : value1(v1), value2(v2) {
        const char *end = (char*)memchr(n, 0, 100);
        int sz = end ? (size_t)(end - n) : -1;
        //printf("spimX %s sz:%d %d %d\n", n, sz, v1, v2);
        name = (char*)malloc(sz+1);
        strncpy(name, n, sz+1);
    }
} inst_info;



/* Exported functions (from spim.c or xspim.c): */

#if 0
int console_input_available (void);
void control_c_seen (int);

void fatal_error (const char *fmt, ...);
char get_console_char (void);
void put_console_char (char c);
void read_input (char *str, int n);

void write_output (FILE*, const char *fmt, ...);
#endif

/* Exported variables: */

//extern int bare_machine;	/* Simulate bare instruction set */

//extern int source_file;		/* Program is source, not binary */




#endif // _SPIM_H
