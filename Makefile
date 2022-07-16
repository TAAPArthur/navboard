CFLAGS ?= -std=c99
DEBUGGING_FLAGS := -g -rdynamic -O0
RELEASE_FLAGS := -O3 -DNDEBUG
ifndef DEBUG
CFLAGS += $(RELEASE_FLAGS)
else
CFLAGS += $(DEBUGGING_FLAGS)
endif

# Dtext flags
DTEXT_CFLAGS = -I /usr/include/freetype2
DTEXT_LDFLAGS = -lxcb-render -lxcb-render-util -lfreetype

CFLAGS += -fPIC -Wall -Werror -Wno-missing-field-initializers -Wno-missing-braces $(DTEXT_CFLAGS)
SRCS := config.c util.c navboard.c xutil.c functions.c
BIN := navboard
BOARDS ?= $(wildcard boards/*.c)
BOARDS_OBJ ?= $(BOARDS:.c=.o)

LDFLAGS := -lX11 -lxcb -lxcb-ewmh -lxcb-icccm -lxcb-xtest $(DTEXT_LDFLAGS)


all: $(BIN) libnavboard.so

install: $(BIN) libnavboard.so
	install -m 0755 -Dt "$(DESTDIR)/usr/lib/" libnavboard.so
	install -m 0755 -D $(BIN).sh "$(DESTDIR)/usr/bin/$(BIN)"
	install -m 0755 -D $(BIN) "$(DESTDIR)/usr/libexec/$(BIN)"
	install -m 0755 -Dt "$(DESTDIR)/usr/include/navboard/" *.h

uninstall:
	rm -f "$(DESTDIR)/usr/bin/$(BIN)"
	rm -f "$(DESTDIR)/usr/libexec/$(BIN)"
	rm -f "$(DESTDIR)/usr/lib/libnavboard.so"
	rm -fr "$(DESTDIR)/usr/include/navboard"

libnavboard.so: $(SRCS:.c=.o) $(BOARDS_OBJ)
	${CC} ${CFLAGS} -shared -o $@ $^ ${LDFLAGS}

navboard: $(SRCS:.c=.o) $(BOARDS_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

CFLAGS := $(DEBUGGING_FLAGS) $(DTEXT_CFLAGS)
navboard-test: $(wildcard Tests/*_unit.c) $(SRCS) $(BOARDS_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -lscutest

test:
test: navboard-test
	xvfb-run -w 1 -a ./$^

clean:
	find . -name "*.o" -exec rm {} +
	rm -f *-test $(BIN) *.so *.a

.PHONY: clean install uninstall install-headers

.DELETE_ON_ERROR:
