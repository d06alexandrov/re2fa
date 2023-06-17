bin_PROGRAMS = re2fa

re2fa_SOURCES = re2fa.c

re2fa_CPPFLAGS = \
	-I$(top_srcdir)/lib

re2fa_LDADD = \
	$(top_builddir)/lib/librefa.la \
	$(PTHREAD_LIBS) \
	$(ARGP_LIBS)
