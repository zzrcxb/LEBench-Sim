CC:=gcc
INC:=include
LIB:=lib
BIN:=bin
BENCH:=bench
OBJ:=obj
SRC:=src

CFLAGS:=-O2 -g -Werror -I$(INC)
HOOK_FLAGS:=-D GEM5_HOOK -D USE_RDTSCP
EXTRA_FLAGS:=
M5FLAGS:=-O2 -DM5OP_ADDR=0xFFFF0000 -DM5OP_PIC -I$(INC)
LDFLAGS:=-O2 -pthread -lm -static -flto

SRCS:=$(shell find $(SRC) -type f -name "*.c")

RUN_OBJS=$(patsubst $(SRC)/%,$(OBJ)/%,$(SRCS:.c=.r.o))
HOOK_OBJS=$(patsubst $(SRC)/%,$(OBJ)/%,$(SRCS:.c=.h.o))
RUN_DEPS=$(patsubst $(SRC)/%,$(OBJ)/%,$(SRCS:.c=.r.d))
HOOK_DEPS=$(patsubst $(SRC)/%,$(OBJ)/%,$(SRCS:.c=.h.d))

.PHONY: run hook clean

run:  $(RUN_OBJS)
	@mkdir -p $(BIN)
	$(CC) -o $(BIN)/LEBench-run $^ $(LDFLAGS)

hook: $(OBJ)/lib/m5ops.o $(HOOK_OBJS)
	@mkdir -p $(BIN)
	$(CC) -o $(BIN)/LEBench-hook $^ $(LDFLAGS)

-include $(RUN_DEPS) $(HOOK_DEPS)

$(OBJ)/%.r.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -MMD -o $@ -c $<

$(OBJ)/%.h.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_FLAGS) -MMD $(HOOK_FLAGS) -o $@ -c $<

$(OBJ)/lib/m5ops.o: $(SRC)/lib/m5op_x86.S
	@mkdir -p $(@D)
	$(CC) $(M5FLAGS) -o $@ -c $<

clean:
	rm -rf $(BIN)/ $(OBJ)/
