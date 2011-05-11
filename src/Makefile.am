## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = 

AM_CFLAGS =\
	 -Wall \
	 -g

bin_PROGRAMS = boblightd \
	boblight-X11 \
	boblight-constant
        
if HAVE_FFMPEG
bin_PROGRAMS += boblight-v4l
endif

boblight_v4l_SOURCES = \
	clients/boblight-v4l/boblight-v4l.cpp \
	clients/boblight-v4l/flagmanager-v4l.cpp \
	util/misc.cpp \
	clients/flagmanager.cpp \
	clients/boblight-v4l/videograbber.cpp

boblight_v4l_LDFLAGS = -lavutil -lavcodec -lavformat -lswscale -lavdevice -lX11

boblight_constant_SOURCES = \
	clients/boblight-constant/boblight-constant.cpp \
	clients/flagmanager.cpp \
	clients/boblight-constant/flagmanager-constant.cpp \
	util/misc.cpp

boblight_X11_SOURCES = \
	clients/boblight-X11/boblight-X11.cpp \
	clients/boblight-X11/vblanksignal.cpp \
	util/timer.cpp \
	util/clock.cpp \
	util/condition.cpp \
	util/mutex.cpp \
	util/thread.cpp \
	util/misc.cpp \
	clients/boblight-X11/grabber-base.cpp \
	clients/boblight-X11/grabber-xgetimage.cpp \
	clients/boblight-X11/grabber-xrender.cpp \
	clients/boblight-X11/flagmanager-X11.cpp \
	clients/flagmanager.cpp

boblight_X11_LDFLAGS = -lX11 -lXrender -lGL

boblightd_SOURCES = \
	util/clock.cpp \
	util/condition.cpp \
	util/log.cpp \
	util/misc.cpp \
	util/mutex.cpp \
	util/thread.cpp \
	util/timer.cpp \
	util/serialport.cpp \
	main.cpp \
	util/tcpsocket.cpp \
	util/messagequeue.cpp \
	client.cpp \
	light.cpp \
	configuration.cpp \
	device/device.cpp \
	device/devicepopen.cpp \
	device/deviceltbl.cpp \
	device/devicers232.cpp \
	device/devicedioder.cpp \
        util/signaltimer.cpp

boblightd_LDFLAGS =

boblightd_LDADD = 

if HAVE_PORTAUDIO
boblightd_SOURCES += device/devicesound.cpp
boblightd_LDFLAGS += -lportaudio
endif

lib_LTLIBRARIES = \
	libboblight.la

libboblight_la_SOURCES = \
	lib/boblight-client.cpp \
	lib/boblight.cpp \
	util/clock.cpp \
	util/messagequeue.cpp \
	util/misc.cpp \
	util/tcpsocket.cpp

libboblight_la_CPPFLAGS = $(AM_CPPFLAGS)

