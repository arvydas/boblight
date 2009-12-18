## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DREVISION="\"`svnversion || echo unknown`\""

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = boblightd \
	boblight-X11\
	boblight-constant\
	boblight-v4l

boblight_v4l_SOURCES = \
	clients/boblight-v4l/boblight-v4l.cpp\
	clients/boblight-v4l/flagmanager-v4l.cpp \
	clients/boblight-v4l/flagmanager-v4l.h \
	util/misc.cpp \
	util/misc.h \
	clients/flagmanager.cpp \
	clients/flagmanager.h \
	clients/boblight-v4l/videograbber.cpp \
	clients/boblight-v4l/videograbber.h 

boblight_constant_SOURCES = \
	clients/boblight-constant/boblight-constant.cpp\
	clients/flagmanager.cpp \
	clients/flagmanager.h \
	clients/boblight-constant/flagmanager-constant.cpp \
	clients/boblight-constant/flagmanager-constant.h \
	util/misc.cpp \
	util/misc.h

boblight_X11_SOURCES = \
	clients/boblight-X11/boblight-X11.cpp\
	clients/boblight-X11/vblanksignal.cpp \
	clients/boblight-X11/vblanksignal.h \
	util/timer.cpp \
	util/clock.cpp \
	util/condition.cpp \
	util/mutex.cpp \
	util/thread.cpp \
	util/misc.cpp \
	util/misc.h \
	clients/boblight-X11/grabber-base.cpp \
	clients/boblight-X11/grabber-base.h \
	clients/boblight-X11/grabber-xgetimage.cpp \
	clients/boblight-X11/grabber-xgetimage.h \
	clients/boblight-X11/grabber-xrender.cpp \
	clients/boblight-X11/grabber-xrender.h \
	clients/boblight-X11/flagmanager-X11.cpp \
	clients/boblight-X11/flagmanager-X11.h \
	clients/flagmanager.cpp \
	clients/flagmanager.h

boblightd_SOURCES = \
	util/clock.cpp    \
	util/clock.h    \
	util/condition.cpp    \
	util/condition.h    \
	util/lock.h    \
	util/log.cpp    \
	util/log.h    \
	util/misc.cpp    \
	util/misc.h    \
	util/mutex.cpp    \
	util/mutex.h    \
	util/thread.cpp    \
	util/thread.h    \
	util/timer.cpp    \
	util/timer.h    \
	util/serialport.cpp    \
	util/serialport.h    \
	main.cpp    \
	util/tcpsocket.cpp    \
	util/tcpsocket.h    \
	util/messagequeue.cpp    \
	util/messagequeue.h    \
	client.cpp    \
	client.h    \
	connectionhandler.cpp    \
	connectionhandler.h    \
	light.cpp    \
	light.h    \
	util/baudrate.h    \
	configuration.h    \
	configuration.cpp    \
	device/device.cpp    \
	device/device.h    \
	device/devicepopen.cpp    \
	device/devicepopen.h    \
	device/deviceltbl.cpp    \
	device/deviceltbl.h    \
	device/devicesound.cpp    \
	device/devicesound.h    \
	device/devicers232.cpp    \
	device/devicers232.h    \
	protocolversion.h    \
        util/signaltimer.h \
        util/signaltimer.cpp

boblightd_LDFLAGS = 

boblightd_LDADD = 

lib_LTLIBRARIES = \
	libboblight.la

libboblight_la_SOURCES = \
	lib/boblight-client.cpp\
	lib/boblight-client.h \
	lib/boblight.cpp \
	lib/boblight.h \
	util/clock.cpp \
	util/clock.h \
	util/messagequeue.cpp \
	util/messagequeue.h \
	util/misc.cpp \
	util/misc.h \
	util/tcpsocket.cpp \
	util/tcpsocket.h \
	lib/boblight-functions.h \
	lib/options.h

libboblight_la_CPPFLAGS = $(AM_CPPFLAGS)

