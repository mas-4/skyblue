# skyblue is written in c' and built with cprime.
# Point CPRIME at the compiler if it isn't checked out next door.
CPRIME ?= ../cprime/cprime

SRCS = $(wildcard src/*.dcdx)
OBJS = $(SRCS:.dcdx=.o)
MD4C = tpc/md4c
MD4C_OBJS = $(MD4C)/md4c.o $(MD4C)/md4c-html.o $(MD4C)/entity.o

skyblue: $(OBJS) $(MD4C_OBJS)
	$(CPRIME) -o $@ $^

src/%.o: src/%.dcdx src/skyblue.h
	$(CPRIME) -I$(MD4C) -c -o $@ $<

$(MD4C)/%.o: $(MD4C)/%.c
	$(CPRIME) -I$(MD4C) -c -o $@ $<

test: skyblue
	tests/run.sh ./skyblue

clean:
	rm -f skyblue src/*.o $(MD4C)/*.o
	rm -rf tests/out

.PHONY: test clean
