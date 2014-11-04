bin_PROGRAMS = nks-scan unnks
if BUILD_NKS_LS_LIBS
bin_PROGRAMS += nks-ls-libs
endif

include_HEADERS = nks.h
lib_LTLIBRARIES = libnks.la

EXTRA_DIST = lib_data.c libnks.sym
DISTCLEANFILES = *~

AM_CPPFLAGS = -include config.h
AM_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(GLIB_CFLAGS)

LIBS_SRC =
LIBS_LD_FLAGS =

if OS_DARWIN
LIBS_SRC += libs-darwin.c
LIBS_LD_FLAGS += -framework CoreFoundation
endif

if OS_WINDOWS
LIBS_SRC += libs-windows.c
endif

LT_CURRENT = 0
LT_REVISION = 0
LT_AGE = 0

libnks_la_SOURCES = \
	$(LIBS_SRC) \
	config.h \
	gen_key.c \
	gen_key.h \
	keys.c \
	keys.h \
	libs.c \
	libs.h \
	nks.c \
	nks.h \
	nks_io.c \
	nks_io.h \
	util.c \
	util.h
libnks_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
                    -no-undefined \
                    -export-symbols $(srcdir)/libnks.sym \
                    $(LIBS_LD_FLAGS)
libnks_la_LIBADD = $(GLIB_LIBS) $(GCRYPT_LIBS)

unnks_SOURCES = \
	config.h \
	unnks.c \
	util.c \
	util.h
unnks_CFLAGS = $(AM_CFLAGS)
unnks_LDADD = libnks.la

nks_scan_SOURCES = \
	config.h \
	nks-scan.c \
	util.c \
	util.h
nks_scan_CFLAGS = $(AM_CFLAGS)
nks_scan_LDADD = libnks.la

if BUILD_NKS_LS_LIBS
nks_ls_libs_SOURCES = \
	config.h \
	nks-ls-libs.c \
	util.c \
	util.h
nks_ls_libs_CFLAGS = $(AM_CFLAGS)
nks_ls_libs_LDFLAGS = $(LIBS_LD_FLAGS)
nks_ls_libs_LDADD = libnks.la
endif

lib_data.c: $(top_srcdir)/libs.conf
	$(top_srcdir)/mklibdata < $(top_srcdir)/libs.conf > lib_data.c

all: lib_data.c
