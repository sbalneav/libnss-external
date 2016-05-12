AC_INIT([nss_external], [1.0], [sbalneav@ltsp.org])
AC_CONFIG_HEADER(config.h)

AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/passwd.c])

AC_PREREQ([2.59])
AC_PROG_CC
AM_PROG_CC_C_O

AM_PROG_LIBTOOL
AM_DISABLE_STATIC

AC_CHECK_HEADER([nss.h], ,
	[AC_MSG_ERROR([NSS headers missing])])

AC_CONFIG_FILES([Makefile] [src/Makefile] [man/Makefile])
AC_OUTPUT
