CC := arm-linux-gnueabi-gcc
CCP := arm-linux-gnueabi-g++
# CC := gcc
# CCP := g++


# Needed for support of v7 assembly instructions on ARM architecture
# ARM_FLAGS := -march=armv7-a -marm

CFLAGS := -g -O3 $(ARM_FLAGS) -lm

CPPFLAGS := $(CFLAGS)

TEST_PROGS := lpm acl-fw maglev monitoring nat-tcp-v4 dpi2 dpi dpi4

TEST_PROGS_CPP := dpi3

# ==== Rules ==================================================================

.PHONY: default clean

default: $(TEST_PROGS) $(TEST_PROGS_CPP)

clean:
	$(RM) $(TEST_PROGS) $(TEST_PROGS_CPP)

$(TEST_PROGS): $(TEST_PROGS).c Makefile utils/*.h utils/*.hpp
	$(CC) -static -o $@ $@.c $(CFLAGS)

$(TEST_PROGS_CPP): $(TEST_PROGS_CPP).cpp Makefile utils/*.h utils/*.hpp
	$(CCP) -static -o $@ $@.cpp $(CFLAGS) -std=c++11
