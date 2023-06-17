AC_INIT([libREFA], [0.1], [d06alexandrov@gmail.com])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([-Wall foreign])
AM_PROG_AR
LT_INIT

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC

AC_CHECK_HEADERS([stdint.h] [stddef.h] [argp.h], [],
  AC_MSG_FAILURE([required headers are missing])
)

# Using zlib for automaton files compression
AC_ARG_WITH([zlib],
  [AS_HELP_STRING([--with-zlib],
    [support compressed automaton files @<:@default=check@:>@])],
  [],
  [with_zlib=check]
)

AS_IF(
  [test "x$with_zlib" != xno],
  [AC_CHECK_LIB([z], [main],
    [AC_DEFINE([USE_ZLIB], [], [Use zlib library]) [with_zlib=yes]],
    [if test "x$with_zlib" != xcheck; then
      AC_MSG_FAILURE([--with-zlib was given, but test for zlib failed])
     fi
    ]
  )]
)

AM_CONDITIONAL([USE_ZLIB], [test "x$with_zlib" = xyes])

AC_SEARCH_LIBS([argp_parse],
  [argp],
  [test "$ac_cv_search_argp_parse" = "none required" || ARGP_LIBS=$ac_cv_search_argp_parse],
  [AC_MSG_ERROR([unable to find the argp_parse() function])])
AC_SUBST([ARGP_LIBS])

AX_PTHREAD([], AC_MSG_FAILURE([test for pthread failed]))

AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset strrchr])

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 tools/Makefile])
AC_OUTPUT