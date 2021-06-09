DEBUGGING_FLAGS := -std=c99 -g -rdynamic -O0 -Werror -Wno-missing-field-initializers -Wno-sign-compare
RELEASE_FLAGS ?= -std=c99 -O3 -DNDEBUG -Werror -Wno-missing-field-initializers -Wno-sign-compare -Wno-missing-braces
ifndef DEBUG
CFLAGS ?= $(RELEASE_FLAGS)
else
CFLAGS ?= $(DEBUGGING_FLAGS)
endif
SRCS := config.c util.c navboard.c xutil.c functions.c
CFLAGS += -fPIC
BIN := navboard
BOARDS ?= $(wildcard boards/*.c)
BOARDS_OBJ ?= $(BOARDS:.c=.o)

LDFLAGS := -lX11 -lxcb -lxcb-ewmh -lxcb-icccm -lxcb-xtest -ldtext

all: $(BIN) libnavboard.so

install: $(BIN) libnavboard.so
	install -m 0755 -Dt "$(DESTDIR)/usr/lib/" libnavboard.so
	install -m 0755 -Dt "$(DESTDIR)/usr/bin/" $(BIN)
	install -m 0755 -D $(BIN)-local.sh "$(DESTDIR)/usr/bin/$(BIN)-local"
	install -m 0755 -Dt "$(DESTDIR)/usr/include/navboard/" *.h

uninstall:
	rm -f "$(DESTDIR)/usr/bin/$(BIN)"
	rm -f "$(DESTDIR)/usr/bin/$(BIN)-local"
	rm -f "$(DESTDIR)/usr/lib/libnavboard.so"

libnavboard.so: $(SRCS:.c=.o) $(BOARDS_OBJ)
	${CC} ${CFLAGS} -fPIC -shared -o $@ $^ ${CFLAGS} ${LDFLAGS}

navboard: $(SRCS:.c=.o) $(BOARDS_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

navboard-test: $(wildcard Tests/*_unit.c) $(SRCS) $(BOARDS_OBJ)
	$(CC) $(DEBUGGING_FLAGS) $^ -o $@ $(LDFLAGS) -lscutest

test: CFLAGS := $(DEBUGGING_FLAGS)
test: navboard-test
	xvfb-run -w 1 -a ./$^

clean:
	find . -name "*.o" -exec rm {} +
	rm -f *-test $(BIN)

.PHONY: clean install uninstall install-headers

.DELETE_ON_ERROR:
