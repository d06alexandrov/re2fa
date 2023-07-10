lib_LTLIBRARIES = librefa.la

librefa_la_SOURCES = \
	dfa.c \
	dfa.h \
	dfastat.h \
	dfa_to_nfa.c \
	dfa_to_nfa.h \
	Makefile.am \
	nfa.c \
	nfa.h \
	nfa_to_dfa.c \
	nfa_to_dfa.h \
	parser.c \
	parser.h \
	parser_inner.c \
	parser_inner.h \
	refa.h \
	tree_to_nfa.c \
	tree_to_nfa.h

librefa_la_LDFLAGS = -version-info 0:0:0

if USE_ZLIB
librefa_la_LIBADD = -lz
endif
