## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src 

boblightddocdir = ${prefix}/doc/boblightd
boblightddoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


EXTRA_DIST = $(boblightddoc_DATA) \
	$(libboblight_HEADERS)

libboblight_HEADERS = \
	src/lib/boblight.h\
	src/lib/boblight-functions.h

libboblightdir = \
	$(prefix)/include/libboblight

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(boblightddocdir)
