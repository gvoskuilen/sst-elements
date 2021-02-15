# -*- Makefile -*-
#
#

AM_CPPFLAGS = \
	$(MPI_CPPFLAGS) \
	-I$(top_srcdir)/src

compdir = $(pkglibdir)
comp_LTLIBRARIES = libmips_4kc.la
libmips_4kc_la_SOURCES = \
	mips_4kc.cc \
	mips_4kc.h \
	cl-cycle.cc \
	cl-cycle.h \
	cl-cache.cc \
	cl-except.h \
	cl-except.cc \
	cl-mem.cc \
	spim-utils.cc \
	data.cc \
	inst.cc \
	mem.cc \
	read-aout.cc \
	reg.h \
	faults.h \
	faults.cc \
	mips-syscall.cc 


EXTRA_DIST = \
	README \
	test.py


libmips_4kc_la_LDFLAGS = -module -avoid-version

# add lib elf
libmips_4kc_la_LDFLAGS += $(LIBELF_LDFLAGS)
libmips_4kc_la_LIBADD = $(LIBELF_LIB)
AM_CPPFLAGS += $(LIBELF_CPPFLAGS)

##########################################################################
##########################################################################
##########################################################################


