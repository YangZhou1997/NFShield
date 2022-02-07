CC := riscv64-unknown-linux-gnu-gcc

CFLAGS := -g -O3 $(ARM_FLAGS) -lm -lpthread -latomic -D_GNU_SOURCE

CPPFLAGS := $(CFLAGS)

TEST_PROGS =
TEST_PROGS += client
TEST_PROGS += server

# ==== Rules ==================================================================

.PHONY: default clean

default: $(TEST_PROGS)

clean:
	$(RM) $(TEST_PROGS)

%: %.c *.h
	$(CC) -static -o $@ $@.c $(CFLAGS)

