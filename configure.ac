dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(boblight, 2.0)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CXX
AM_PROG_CC_STDC
AC_HEADER_STDC

AM_PROG_LIBTOOL

AC_CHECK_LIB(pthread,[main])
AC_CHECK_LIB(rt,[main])
AC_CHECK_LIB(m,[main])
AC_CHECK_LIB(dl,[main])
AC_CHECK_LIB(X11,[main],,AC_MSG_ERROR([Missing a required library]))
AC_CHECK_LIB(GL,[main],,AC_MSG_ERROR([Missing a required library]))
AC_CHECK_LIB(Xrender,[main],,AC_MSG_ERROR([Missing a required library]))
AC_CHECK_LIB(avcodec,[main],,AC_MSG_ERROR([Missing a required library]))
AC_CHECK_LIB(avformat,[main],,AC_MSG_ERROR([Missing a required library]))
AC_CHECK_LIB(swscale,[main],,AC_MSG_ERROR([Missing a required library]))

AC_CHECK_HEADERS([X11/Xlib.h X11/Xutil.h GL/glx.h],,AC_MSG_ERROR([Missing a required header]))
AC_CHECK_HEADERS([libavcodec/avcodec.h libavformat/avformat.h libswscale/swscale.h],,AC_MSG_ERROR([Missing a required header]))

AC_OUTPUT([
Makefile
src/Makefile
])
