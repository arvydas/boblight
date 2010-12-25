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

dnl all use this
AC_CHECK_LIB(pthread,[main])
AC_CHECK_LIB(rt,[main])
AC_CHECK_LIB(m,[main])
AC_CHECK_LIB(dl,[main])

dnl boblightd
AC_CHECK_LIB(portaudio,[main],AC_DEFINE([HAVE_LIBPORTAUDIO],      [1], [ ]),AC_MSG_ERROR([Missing a required library]))

dnl boblight-X11
AC_CHECK_LIB(X11      ,[main],AC_DEFINE([HAVE_LIBX11],      [1], [ ]),AC_MSG_ERROR([Missing a required library]))
AC_CHECK_LIB(Xrender  ,[main],AC_DEFINE([HAVE_LIBXRENDER],  [1], [ ]),AC_MSG_ERROR([Missing a required library]))

dnl boblight-X11 and boblight-opengl
AC_CHECK_LIB(GL       ,[main],AC_DEFINE([HAVE_LIBGL],       [1], [ ]),AC_MSG_ERROR([Missing a required library]))

dnl boblight-v4l
AC_CHECK_LIB(avutil   ,[main],AC_DEFINE([HAVE_LIBAVUTIL],   [1], [ ]),AC_MSG_ERROR([Missing a required library]))
AC_CHECK_LIB(avcodec  ,[main],AC_DEFINE([HAVE_LIBAVCODEC],  [1], [ ]),AC_MSG_ERROR([Missing a required library]))
AC_CHECK_LIB(avformat ,[main],AC_DEFINE([HAVE_LIBAVFORMAT], [1], [ ]),AC_MSG_ERROR([Missing a required library]), [-lavcodec -lavutil])
AC_CHECK_LIB(swscale  ,[main],AC_DEFINE([HAVE_LIBSWSCALE],  [1], [ ]),AC_MSG_ERROR([Missing a required library]), [-lavutil])
AC_CHECK_LIB(avdevice ,[main],AC_DEFINE([HAVE_LIBAVDEVICE], [1], [ ]),AC_MSG_ERROR([Missing a required library]), [-lavcodec -lavutil -lavformat])

AC_CHECK_HEADERS([X11/Xlib.h X11/Xutil.h GL/glx.h],,AC_MSG_ERROR([Missing a required header]))
AC_CHECK_HEADERS([X11/extensions/Xrender.h X11/extensions/XShm.h],,AC_MSG_ERROR([Missing a required header]), [#include <X11/Xlib.h>])
AC_CHECK_HEADERS([libavcodec/avcodec.h libavformat/avformat.h libswscale/swscale.h],,AC_MSG_ERROR([Missing a required header]))
AC_CHECK_HEADERS([portaudio.h],,AC_MSG_ERROR([Missing a required header]))
AC_CHECK_TYPE([PaStreamCallbackTimeInfo], , AC_MSG_ERROR([boblight requires portaudio v19 headers!]), [[#include <portaudio.h>]])

AC_OUTPUT([
Makefile
src/Makefile
])
