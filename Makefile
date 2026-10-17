# skyblue is written in c' and built with cprime: a checkout next door
# if there is one, otherwise the cprime in your PATH. Or set CPRIME.
CPRIME ?= $(if $(wildcard ../cprime/cprime),../cprime/cprime,cprime)

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# The commit skyblue is built from, for --version and ${sky.version}.
VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo unknown)

SRCS = $(wildcard src/*.dcdx)
OBJS = $(SRCS:.dcdx=.o)
MD4C = tpc/md4c
MD4C_OBJS = $(MD4C)/md4c.o $(MD4C)/md4c-html.o $(MD4C)/entity.o

skyblue: $(OBJS) $(MD4C_OBJS)
	$(CPRIME) -o $@ $^

src/%.o: src/%.dcdx src/skyblue.h
	$(CPRIME) -I$(MD4C) -DSKYBLUE_VERSION='"$(VERSION)"' -c -o $@ $<

$(MD4C)/%.o: $(MD4C)/%.c
	$(CPRIME) -I$(MD4C) -c -o $@ $<

test: skyblue
	tests/run.sh ./skyblue

install: skyblue
	install -Dm755 skyblue $(DESTDIR)$(BINDIR)/skyblue

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/skyblue

clean:
	rm -f skyblue src/*.o $(MD4C)/*.o
	rm -rf tests/out

.PHONY: test install uninstall clean
