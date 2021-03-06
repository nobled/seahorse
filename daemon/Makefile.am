
localedir = $(datadir)/locale
pixmapsdir = $(datadir)/pixmaps/

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libcryptui \
	-I$(top_srcdir)/libegg \
	-I$(top_srcdir)/libseahorse \
	$(SEAHORSE_CFLAGS) \
	-DDATA_DIR=\""$(datadir)"\" \
	-DLOCALEDIR=\""$(localedir)"\" \
	-DPIXMAPSDIR=\""$(pixmapsdir)"\" \
	-DLIBCRYPTUI_API_SUBJECT_TO_CHANGE \
	-DAUTOSTARTDIR=\""$(sysconfdir)/xdg/autostart"\"

bin_PROGRAMS = seahorse-daemon

BUILT_SOURCES = \
	seahorse-service-bindings.h \
	seahorse-service-keyset-bindings.h \
	seahorse-service-crypto-bindings.h 

if WITH_SHARING
SHARING_SRCS = seahorse-hkp-server.c seahorse-sharing.c 
else
SHARING_SRCS = 
endif

if WITH_PGP
PGP_LDADD = $(top_builddir)/pgp/libseahorse-pgp.la
PGP_SRCS = seahorse-service-crypto.c
else
PGP_LDADD = 
PGP_SRCS = 
endif

if WITH_SSH
SSH_LDADD = $(top_builddir)/ssh/libseahorse-ssh.la
else
SSH_LDADD = 
endif

seahorse_daemon_SOURCES = \
    seahorse-daemon.c seahorse-daemon.h \
    seahorse-dbus-server.c seahorse-service.c seahorse-service.h \
    seahorse-service-keyset.c \
    $(PGP_SRCS) \
    $(SHARING_SRCS)


seahorse_daemon_LDADD = \
	$(top_builddir)/libseahorse/libseahorse.la \
	$(PGP_LDADD) \
	$(SSH_LDADD) \
	$(top_builddir)/common/libseahorse-common.la \
	$(top_builddir)/libcryptui/libcryptui.la \
	$(top_builddir)/libegg/libeggdatetime.la \
	$(top_builddir)/libegg/libeggsmclient.la \
	$(SEAHORSE_LIBS)

man_MANS = seahorse-daemon.1

EXTRA_DIST = \
    $(man_MANS) \
    seahorse-service.xml \
    seahorse-service-keyset.xml \
    seahorse-service-crypto.xml \
    seahorse-service-bindings.h \
    seahorse-service-keyset-bindings.h \
    seahorse-service-crypto-bindings.h \
    org.gnome.seahorse.service.in

CLEANFILES = *.bak \
    seahorse-daemon.desktop.in \
    org.gnome.seahorse.service

# DBUS binding files
seahorse-service-bindings.h: seahorse-service.xml
	dbus-binding-tool --prefix=seahorse_service --mode=glib-server --output=$@ $<
seahorse-service-keyset-bindings.h: seahorse-service-keyset.xml
	dbus-binding-tool --prefix=seahorse_service_keyset --mode=glib-server --output=$@ $<
seahorse-service-crypto-bindings.h: seahorse-service-crypto.xml
	dbus-binding-tool --prefix=seahorse_service_crypto --mode=glib-server --output=$@ $<

# Dbus service file
servicedir = $(datadir)/dbus-1/services/
service_in_files = org.gnome.seahorse.service.in
service_DATA = $(service_in_files:.service.in=.service)

# Rule to make the service file with bindir expanded
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@
