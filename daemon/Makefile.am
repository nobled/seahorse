
gladedir = $(datadir)/seahorse/glade/
localedir = $(datadir)/locale
pixmapsdir = $(datadir)/pixmaps/

INCLUDES = -I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/libseahorse \
	-I$(top_srcdir)/agent \
	$(SEAHORSE_CFLAGS) \
	-DDATA_DIR=\""$(datadir)"\" \
	-DSEAHORSE_GLADEDIR=\""$(gladedir)"\" \
	-DLOCALEDIR=\""$(localedir)"\" \
	-DPIXMAPSDIR=\""$(pixmapsdir)"\"

bin_PROGRAMS = seahorse-daemon

if WITH_AGENT
AGENT_LIB = $(top_builddir)/agent/libseahorseagent.a
else
AGENT_LIB = 
endif

if WITH_SHARING
SHARING_SRCS = seahorse-hkp-server.c seahorse-sharing.c 
else
SHARING_SRCS = 
endif

seahorse_daemon_SOURCES = \
        seahorse-daemon.c \
        seahorse-daemon.h \
        $(SHARING_SRCS)

seahorse_daemon_LDADD = \
    $(AGENT_LIB) \
	$(top_builddir)/libseahorse/libseahorse-internal.la \
	$(SEAHORSE_LIBS)
                         
glade_DATA = 

EXTRA_DIST = \
	$(glade_DATA) 

CLEANFILES = *.gladep* *.bak 

install-exec-hook:
	chmod u+s $(bindir)/seahorse-daemon