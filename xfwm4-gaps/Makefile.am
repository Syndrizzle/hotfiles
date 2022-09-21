@SET_MAKE@

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

EXTRA_DIST =								\
	example.gtkrc-2.0						\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	COMPOSITOR							\
	README.md

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	po/.intltool-merge-cache.lock

SUBDIRS = 								\
	defaults 							\
	helper-dialog 							\
	icons 								\
	po 								\
	common 								\
	settings-dialogs						\
	src 								\
	themes

distclean-local:
	rm -rf *.cache

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog
