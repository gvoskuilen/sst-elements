

AC_DEFUN([SST_CHECK_LIBELF], [
  AC_ARG_WITH([libelf],
    [AS_HELP_STRING([--with-libelf@<:@=DIR@:>@],
      [Use libelf package installed in optionally specified DIR])])

  sst_check_libelf_happy="yes"
  #AS_IF([test "$with_libelf" = "no"], [sst_check_libelf_happy="no"])

  CPPFLAGS_saved="$CPPFLAGS"
  LDFLAGS_saved="$LDFLAGS"
  LIBS_saved="$LIBS"

  AS_IF([test ! -z "$with_libelf" -a "$with_libelf" != "yes"],
    [LIBELF_CPPFLAGS="-I$with_libelf/include"
     CPPFLAGS="$LIBELF_CPPFLAGS $CPPFLAGS"
     LIBELF_LDFLAGS="-L$with_libelf/lib"
     LIBELF_LIBDIR="$with_libelf/lib"
     LDFLAGS="$LIBELF_LDFLAGS $LDFLAGS"],
    [LIBELF_CPPFLAGS=
     LIBELF_LDFLAGS=
     LIBELF_LIBDIR=])

  AC_LANG_PUSH(C++)
  AC_CHECK_HEADERS([libelf.h], [], [sst_check_libelf_happy="no"])
  AC_CHECK_HEADERS([libelf/libelf.h], [], [sst_check_libelf_happy="no"])
  AC_CHECK_LIB([elf], [elf_version],
    [LIBELF_LIB="-lelf"], [sst_check_libelf_happy="no"])
  AC_LANG_POP(C++)

  CPPFLAGS="$CPPFLAGS_saved"
  LDFLAGS="$LDFLAGS_saved"
  LIBS="$LIBS_saved"

  AC_SUBST([LIBELF_CPPFLAGS])
  AC_SUBST([LIBELF_LDFLAGS])
  AC_SUBST([LIBELF_LIB])
  AC_SUBST([LIBELF_LIBDIR])
  AM_CONDITIONAL([HAVE_LIBELF], [test "$sst_check_libelf_happy" = "yes"])
  AS_IF([test "$sst_check_libelf_happy" = "yes"],
        [AC_DEFINE([HAVE_LIBELF], [1], [Set to 1 if LIBELF was found])])
  AC_DEFINE_UNQUOTED([LIBELF_LIBDIR], ["$LIBELF_LIBDIR"], [Path to LIBELF library])

  AS_IF([test "$sst_check_libelf_happy" = "yes"], [$1], [$2])
])
