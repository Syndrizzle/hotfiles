bin_PROGRAMS = \
	xfwm4-settings  \
	xfwm4-tweaks-settings  \
	xfwm4-workspace-settings

xfwm4_workspace_settings_SOURCES = \
	workspace-settings.c \
	xfwm4-workspace-dialog_ui.h \
	workspace-resource.c \
	workspace-resource.h

xfwm4_workspace_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	$(LIBWNCK_CFLAGS) \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE

xfwm4_workspace_settings_LDADD = \
	$(top_builddir)/common/libxfwm-common.la \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCONF_LIBS) \
	$(LIBWNCK_LIBS)

xfwm4_settings_SOURCES = \
	xfwm4-settings.c \
	xfwm4-settings.h \
	xfwm4-dialog_ui.h \
	range-debouncer.c \
	range-debouncer.h

xfwm4_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4KBD_PRIVATE_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"

xfwm4_settings_LDADD = \
	$(top_builddir)/common/libxfwm-common.la \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCE4KBD_PRIVATE_LIBS) \
	$(LIBXFCONF_LIBS)

xfwm4_tweaks_settings_SOURCES = \
	tweaks-settings.c \
	xfwm4-tweaks-dialog_ui.h \
	range-debouncer.c \
	range-debouncer.h

xfwm4_tweaks_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCONF_CFLAGS) \
	-DDATADIR=\"$(datadir)\" \
	-DSRCDIR=\"$(top_srcdir)\" \
	-DLOCALEDIR=\"$(localedir)\"

xfwm4_tweaks_settings_LDADD = \
	$(top_builddir)/common/libxfwm-common.la \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCONF_LIBS)

AM_CPPFLAGS = \
	-I${top_srcdir} \
	$(PLATFORM_CPPFLAGS)

if MAINTAINER_MODE

BUILT_SOURCES = \
	xfwm4-dialog_ui.h \
	xfwm4-tweaks-dialog_ui.h \
	xfwm4-workspace-dialog_ui.h \
	workspace-resource.h \
	workspace-resource.c

xfwm4-workspace-dialog_ui.h: xfwm4-workspace-dialog.glade
	$(AM_V_GEN) $(XDT_CSOURCE) --static --strip-comments --strip-content --name=workspace_dialog_ui $< >$@

xfwm4-dialog_ui.h: xfwm4-dialog.glade
	$(AM_V_GEN) $(XDT_CSOURCE) --static --strip-comments --strip-content --name=xfwm4_dialog_ui $< >$@

xfwm4-tweaks-dialog_ui.h: xfwm4-tweaks-dialog.glade
	$(AM_V_GEN) $(XDT_CSOURCE) --static --strip-comments --strip-content --name=tweaks_dialog_ui $< >$@

workspace-resource.h:
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) $(srcdir)/workspace.gresource.xml --target=$@ --generate-header

workspace-resource.c:
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) $(srcdir)/workspace.gresource.xml --target=$@ --generate-source

endif

desktopdir = $(datadir)/applications
desktop_in_files = \
	xfce-wm-settings.desktop.in \
	xfce-wmtweaks-settings.desktop.in \
	xfce-workspaces-settings.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_files)  \
	xfwm4-workspace-dialog.glade  \
	xfwm4-tweaks-dialog.glade  \
	xfwm4-dialog.glade \
	workspace.gresource.xml \
	monitor-icon.svg

DISTCLEANFILES = \
	$(desktop_DATA)
