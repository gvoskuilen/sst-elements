dnl -*- Autoconf -*-

AC_DEFUN([SST_mips_4kc_CONFIG], [
  sst_check_mips_4kc="yes"

  libelf_happy="yes"

  # Use LIBELF
  SST_CHECK_LIBELF([libelf_happy="yes"],[libelf_happy="no"],[AC_MSG_ERROR([libelf library could not be found])])

  AS_IF( [test "$libelf_happy"="yes"], [sst_check_mips_4kc="yes"], [sst_check_mips_4kc="no"] )

  AS_IF([test "$sst_check_mips_4kc"="yes"], [$1], [$2])
])
