#CC := arm-linux-gnueabi-gcc
#CCP := arm-linux-gnueabi-g++
#  CC := gcc
#  CCP := g++
CC := riscv64-linux-gcc
# CC := gcc


# Needed for support of v7 assembly instructions on ARM architecture
# ARM_FLAGS := -march=armv7-a -marm

CFLAGS := -g -O3 $(ARM_FLAGS) -lm -lpthread

CPPFLAGS := $(CFLAGS)

TEST_PROGS := pktgen nftop

# ==== Rules ==================================================================

.PHONY: default clean

default: $(TEST_PROGS)

clean:
	$(RM) $(TEST_PROGS)

pktgen: pktgen.c utils/*.h
	$(CC) -static -o $@ $@.c $(CFLAGS)

nftop: nftop.c utils/*.h nfs/*.h
	$(CC) -static -o $@ $@.c $(CFLAGS)

