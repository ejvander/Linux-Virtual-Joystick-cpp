dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(Linux-Virtual-Joystick-cpp, 1.0)

AC_CHECK_HEADERS

AC_CHECK_FUNCS(getcwd gettimeofday memset strstr strtol)
AC_C_INLINE 
AC_FUNC_ERROR_AT_LINE 
AC_FUNC_MALLOC
AC_HEADER_STDBOOL 
AC_PROG_CC
AC_TYPE_SIZE_T 


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE(-Wall -Werror foreign)

AC_PROG_CXX

#AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES(Makefile src/Makefile)

AC_OUTPUT
