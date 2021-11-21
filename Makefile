CC:= gcc
CFLAGS+= -std=c99 -Wall -Werror -pedantic -O3
CFLAGS+= -include stddef.h -include stdint.h
CFLAGS+= -Wl,-rpath,'$$ORIGIN/lib'
CFLAGS+= -Wl,-allow-shlib-undefined
SUBDIR:= lib

CFLAGS+= $(SUBDIR:%=-L%)
LIBS:= -lrefa
LIBS+= -pthread

VER:= version_info.h

BIN:= re2fa

all: $(SUBDIR) $(BIN)

re2fa: main.c $(VER)
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

$(VER): .FORCE
	echo -n 'const char *git_version = "'	>  version_info.h
	echo -n `git describe`			>> version_info.h
	echo '";'				>> version_info.h

.FORCE:

$(SUBDIR):
	$(MAKE) -C $@

.PHONY: $(SUBDIR)

clean:
	@rm -vf *.o $(BIN) $(VER); \
	for dir in $(SUBDIR); do \
	  $(MAKE) -C $$dir -f Makefile $@; \
	done
