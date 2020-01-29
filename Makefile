# CC := arm-linux-gnueabi-gcc
CC := gcc

# Needed for support of v7 assembly instructions on ARM architecture
# ARM_FLAGS := -march=armv7-a -marm

CFLAGS := -g -O2 $(ARM_FLAGS) -lm

CPPFLAGS := $(CFLAGS)

TEST_PROGS := lpm acl-fw maglev monitoring nat-tcp-v4


# ==== Rules ==================================================================

.PHONY: default clean

default: $(TEST_PROGS) 

clean:
	$(RM) $(TEST_PROGS) 

$(TEST_PROGS): $(TEST_PROGS).c Makefile
	$(CC) -static -o $@ $@.c $(CFLAGS)
