CC:=gcc
INC:=include
LIB:=lib
BIN:=bin
BENCH:=bench

CFLAGS:=-O2 -g -Werror -I$(INC)
HOOK_FLAGS:=-D GEM5_HOOK -D USE_RDTSCP
EXTRA_FLAGS:=
M5FLAGS:=-O2 -DM5OP_ADDR=0xFFFF0000 -DM5OP_PIC -I$(INC)
LDFLAGS:=-O2 -pthread -lm -static -flto

SRCS:=$(shell find * -type f -name "*.c")
RUN_OBJS:=$(SRCS:.c=.r.o)
HOOK_OBJS:=$(SRCS:.c=.h.o)

.PHONY: run hook clean

run: $(RUN_OBJS)
	$(CC) -o $(BIN)/LEBench-run $^ $(LDFLAGS)
	find * -type f -name "*.r.o" -exec rm {} \;

hook: m5ops.o $(HOOK_OBJS)
	$(CC) -o $(BIN)/LEBench-hook $^ $(LDFLAGS)
	find * -type f -name "*.h.o" -exec rm {} \;

$(RUN_OBJS): %.r.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -o $@ -c $<

$(HOOK_OBJS): %.h.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) $(HOOK_FLAGS) -o $@ -c $<

m5ops.o: lib/m5op_x86.S
	$(CC) $(M5FLAGS) -o $@ -c $<

clean:
	find * -type f -name "*.o" -exec rm {} \;
	rm -f bin/*
