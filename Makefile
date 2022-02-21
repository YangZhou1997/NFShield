CC=riscv64-unknown-elf-gcc
CFLAGS=-mcmodel=medany -Wall -O2 -fno-common -fno-builtin-printf
LDFLAGS=-static -nostdlib -nostartfiles -lgcc

CC_LINUX=riscv64-unknown-linux-gnu-gcc
CFLAGS_LINUX=-Wall -O2
LDFLAGS_LINUX=-lrt

FIRESIM = ~/firesim/deploy/workloads
TEST_PROGS = nftop

default: $(addsuffix .riscv,$(TEST_PROGS))

%.riscv: %.o crt.o syscalls.o
	$(CC) -T ./utils/link.ld $(LDFLAGS) $^ -o $@
	cp *.ini ${FIRESIM}/
	cp *.json ${FIRESIM}/
	mkdir -p ${FIRESIM}/nf_workloads
	cp $@ ${FIRESIM}/nf_workloads

%.o: %.c ./utils/util.h ./utils/encoding.h ./utils/mmio.h ./utils/icenic.h
	$(CC) $(CFLAGS) -c $< -o $@

syscalls.o: ./utils/syscalls.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: ./utils/%.S
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -f *.riscv *.o