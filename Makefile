CC := gcc
DEBUGGING_FLAGS := -std=c99 -g -rdynamic -O0 -Werror -Wno-missing-field-initializers -Wno-sign-compare -Wno-missing-braces
RELEASE_FLAGS ?= -std=c99 -O3 -DNDEBUG -Werror -Wall -Wextra -Wno-missing-field-initializers -Wno-sign-compare -Wno-missing-braces
CFLAGS ?= $(RELEASE_FLAGS)
SRCS := config.c util.c navboard.c xutil.c
BIN := navboard
BOARDS ?= $(wildcard boards/*.c)
BOARDS_OBJ ?= $(BOARDS:.c=.o)

LDFLAGS := -lX11 -lxcb -lxcb-ewmh -lxcb-icccm -lxcb-xtest -lX11-xcb -lXft -lm


all: $(BIN)

install: $(BIN)
	install -m 0755 -Dt "$(DESTDIR)/usr/bin/" $(BIN)
	install -m 0755 -D $(BIN)-local.sh "$(DESTDIR)/usr/bin/$(BIN)-local"
	install -m 0755 -Dt "$(DESTDIR)/usr/include/navboard/" *.h

uninstall:
	rm -f "$(DESTDIR)/usr/bin/$(BIN)"
	rm -f "$(DESTDIR)/usr/bin/$(BIN)-local"

navboard: $(SRCS:.c=.o) $(BOARDS_OBJ)
	$(CC) $(CFLAGS) $^   -o $@ $(LDFLAGS) -lscutest

navboard-test: navboard_unit.c $(SRCS) $(BOARDS_OBJ)
	$(CC) $(DEBUGGING_FLAGS) $^ -o $@ $(LDFLAGS) -lscutest

CFLAGS := $(DEBUGGING_FLAGS)
test: navboard-test
	xvfb-run -w 1 -a ./$^

clean:
	rm -f *.{o,a} *-test $(BIN)
	rm -f boards/*.{o,a} *-test

.PHONY: clean install uninstall install-headers

.DELETE_ON_ERROR:
