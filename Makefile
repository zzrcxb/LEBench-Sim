CC:=gcc
INC:=include
LIB:=lib
BIN:=bin
BENCH:=bench

CFLAGS:=-O2 -g -Werror -I$(INC) -D USE_RDTSCP
M5FLAGS:=-O2 -DM5OP_ADDR=0xFFFF0000 -DM5OP_PIC -I$(INC)
LDFLAGS:=-pthread -lm -static

SRCS:=$(wildcard $(BENCH)/*.c)
RUN_OBJS:=$(SRCS:$(BENCH)/%.c=%.r.o)
HOOK_OBJS:=$(SRCS:$(BENCH)/%.c=%.h.o)

.PHONY: run hook clean

run: $(RUN_OBJS) main.r.o
	$(CC) -o $(BIN)/LEBench-run $^ $(LDFLAGS)
	rm -f *.o

hook: m5ops.o $(HOOK_OBJS) main.h.o
	$(CC) -o $(BIN)/LEBench-hook $^ $(LDFLAGS)
	rm -f *.o

$(HOOK_OBJS): CFLAGS+=-D GEM5_HOOK -D USE_RDTSCP

%.r.o %.h.o: $(BENCH)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.r.o %.h.o: main.c
	$(CC) $(CFLAGS) -o $@ -c $<

m5ops.o: lib/m5op_x86.S
	$(CC) $(M5FLAGS) -o $@ -c $<

clean:
	rm -f *.o bin/*
