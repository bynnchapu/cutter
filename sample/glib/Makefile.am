AM_CPPFLAGS = 			\
	-I$(top_builddir)	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/cutter	\
	-I$(top_srcdir)/gcutter

AM_CFLAGS = $(CUTTER_CFLAGS)

EXTRA_DIST =			\
	run-test.sh		\
	iochannel-test-infile

if MAINTAINER_MODE
TESTS = run-test.sh
endif
TESTS_ENVIRONMENT = NO_MAKE=yes

noinst_LTLIBRARIES = 			\
		test_array.la		\
		test_base64.la		\
		test_iochannel.la	\
		test_keyfile.la		\
		test_utf8_pointer.la	\
		test_utf8_validate.la	\
		test_uri.la

AM_LDFLAGS =			\
	-rpath `pwd`/.libs	\
	-module			\
	-avoid-version		\
	-no-undefined

LIBS =						\
	$(top_builddir)/cutter/libcutter.la	\
	$(GLIB_LIBS)

test_array_la_SOURCES = array.c
test_base64_la_SOURCES = base64.c
test_iochannel_la_SOURCES = iochannel.c
test_keyfile_la_SOURCES = keyfile.c
test_uri_la_SOURCES = uri.c
test_utf8_pointer_la_SOURCES = utf8-pointer.c
test_utf8_validate_la_SOURCES = utf8-validate.c
