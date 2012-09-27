#-------------------------------------------------------------------------
#
# Makefile for contrib/pgxc_clean
#
# Portions Copyright (c) 2011-2012 Postgres-XC Development Group
#
# $PostgreSQL$
#
#-------------------------------------------------------------------------

PGFILEDESC = "pgxc_clean_gtm - unregister node from GTM on the fly"
PGAPPICON = win32

PROGRAM= gtm_clean
OBJS= gtm_clean.o mcxt.o

#Include GTM objects
gtm_builddir = $(top_builddir)/src/gtm
EX_OBJS = $(gtm_builddir)/common/assert.o \
	  $(gtm_builddir)/client/libgtmclient.a \
	  $(gtm_builddir)/common/gtm_serialize.o

PG_CPPFLAGS  = -DFRONTEND -DDLSUFFIX=\"$(DLSUFFIX)\" -I$(srcdir) -I$(libpq_srcdir)
PG_LIBS = $(libpq_pgport) $(PTHREAD_LIBS) $(EX_OBJS)

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pgxc_clean_gtm
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
