#
# SYNOPSIS
#
#   AX_CXX_ENABLE_LIBRARY(VARIABLE-LABEL, HEADER-FILE, LIBRARY-FILE,
#                         [HELP-TEXT],
#                         [ACTION-IF-ENABLE], [ACTION-IF-DISBALE])
#
#
# DESCRIPTION
#
#   Provides a generic test for a given library, similar in concept to the
#   PKG_CHECK_MODULES macro used by pkg-config.
#
#   Most simplest libraries can be checked against simply through the
#   presence of a header file and a library to link to. This macro allows to
#   wrap around the test so that it doesn't have to be recreated each time.
#
#
#   If the library is find, LABEL_LIBS and WITH_LABEL are defined, and
#   ax_LABEL_enable is equal to yes or no.
#
#   Example:
#
#     AX_CXX_ENABLE_LIBRARY([GTEST], [gtest/gtest.h], [gtest],
#                           [enable tests], [], [])
#

AC_DEFUN([AX_CXX_ENABLE_LIBRARY], [
  m4_pushdef([lib_prefix], m4_toupper([$1]))
  m4_pushdef([with_arg], m4_tolower([$1]))
  m4_pushdef([description],
             [m4_default([$4], [build with ]with_arg[ support])])

  AC_ARG_VAR(lib_prefix[_LIBS], [linker flags for ]$1[ library])

  AC_ARG_WITH(with_arg,
    [AS_HELP_STRING([--with-]with_arg, description[@<:@default=auto@:>@])],
    [],
    [AS_TR_SH([with_]with_arg)=auto])

  AC_LANG_PUSH([C++])

  AS_IF([test x$AS_TR_SH([with_]with_arg) != xno], [
    AC_CHECK_HEADERS([$2],
      [AS_TR_SH([have_]with_arg)=yes],
      [AS_TR_SH([have_]with_arg)=no])
    AS_IF([test x$AS_TR_SH([have_]with_arg) != xno],
      AC_CHECK_LIB([$3], [main],
        [AS_TR_SH(lib_prefix[_LIBS])=-l$3],
        [AS_TR_SH([have_]with_arg)=no]))
    AS_IF([test x$AS_TR_SH([with_]with_arg) == xyes && test x$AS_TR_SH([have_]with_arg) == xno], 
      [AC_MSG_ERROR([failed to find library ]with_arg[ with header ]$2)])
    AS_TR_SH([with_]with_arg)=$AS_TR_SH([have_]with_arg)
  ], [AS_TR_SH([have_]with_arg)=no])

  AC_LANG_POP([C++])

  AS_IF([test x$AS_TR_SH([with_]with_arg) == xyes],
    [$5], [$6])

  AM_CONDITIONAL([WITH_][$1], [test x$AS_TR_SH([with_]with_arg) == xyes])

  AS_TR_SH([ax_]with_arg[_enable])=$AS_TR_SH([with_]with_arg)

  m4_popdef([description])
  m4_popdef([with_arg])
  m4_popdef([lib_prefix])
])
