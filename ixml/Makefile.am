# $Id: Makefile.am,v 1.4 2006/03/27 21:27:31 r3mi Exp $
#
# "Makefile.am" for "libupnp/ixml"
#
# (C) Copyright 2005 Rémi Turboult <r3mi@users.sourceforge.net>
#

AM_CPPFLAGS		= -I$(srcdir)/inc -I$(srcdir)/src/inc \
                          -I$(top_srcdir)/upnp/inc
AM_CFLAGS		= 

LDADD			= libixml.la

if ENABLE_DEBUG
    AM_CPPFLAGS		+= -DDEBUG
else
    AM_CPPFLAGS		+= -DNDEBUG
endif

if ENABLE_SCRIPTSUPPORT
    AM_CPPFLAGS		+= -DIXML_HAVE_SCRIPTSUPPORT
endif

lib_LTLIBRARIES		= libixml.la

libixml_la_LDFLAGS	= -version-info $(LT_VERSION_IXML) \
			  -export-symbols-regex '^ixml.*'

if WIN32
libixml_la_LDFLAGS += -no-undefined
endif

libixml_la_SOURCES	= \
			src/attr.c \
			src/document.c \
			src/element.c \
			src/inc/ixmlmembuf.h \
			src/inc/ixmlparser.h \
			src/ixml.c \
			src/ixmldebug.c \
			src/ixmlparser.c \
			src/ixmlmembuf.c \
			src/namedNodeMap.c \
			src/node.c \
			src/nodeList.c

upnpincludedir		= $(includedir)/upnp

upnpinclude_HEADERS	= \
			inc/ixml.h \
			inc/ixmldebug.h \
			inc/posix_overwrites.h

check_PROGRAMS          = \
			test_document \
			poc_gh_506 \
			poc_gh_517

TESTS                   = \
			test/test_document.sh \
			poc_gh_506 \
			poc_gh_517

test_document_SOURCES 	= test/test_document.c

poc_gh_506_SOURCES      = test/poc_gh_506.c

poc_gh_517_SOURCES      = test/poc_gh_517.cpp

EXTRA_DIST		= \
			test/test_document.sh \
			test/testdata \
			test/poc_gh_249.c \
			test/poc_ghsa_hcx4.c \
			test/poc_ghsa_hcx4_utf8.c \
			test/poc_ghsa_j247_clonenode.c \
			test/poc_ghsa_j247_getbytagname.c \
			test/poc_ghsa_j247_getbytagnamens.c \
			test/poc_ghsa_j247_isancestor.c \
			test/poc_ghsa_j247_print.c \
			test/poc_ghsa_q54q.c \
			test/add_subdirectory.cmake.in \
			test/add_subdirectory_exclude.cmake.in \
			test/find_package_no_components.cmake.in \
			test/find_package_test.cmake.in \
			test/find_package_wrong_components.cmake.in

dist-hook:
	rm -rf `find $(distdir)/test/testdata/ -type f \! -name '*ml' -print`

clean-local:
	@if [ -d bin ] ; then  rm -rf bin ; fi

