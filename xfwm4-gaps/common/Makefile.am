AM_CPPFLAGS = \
	-I${top_srcdir}

noinst_LTLIBRARIES = \
	libxfwm-common.la

libxfwm_common_la_SOURCES = \
	xfwm-common.c \
	xfwm-common.h

libxfwm_common_la_CFLAGS = \
	$(GTK_CFLAGS) \
	$(PLATFORM_CFLAGS)

libxfwm_common_la_LIBADD = \
	$(GTK_LIBS)

libxfwm_common_la_LDFLAGS = \
	-no-undefined \
	$(PLATFORM_LDFLAGS)
