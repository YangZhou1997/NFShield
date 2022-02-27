CC=riscv64-unknown-elf-gcc
CFLAGS=-mcmodel=medany -Wall -O2 -fno-common -fno-builtin-printf -Wno-unused-function $(CONFIG)
LDFLAGS=-static -nostdlib -nostartfiles -lgcc

CC_LINUX=riscv64-unknown-linux-gnu-gcc
CFLAGS_LINUX=-Wall -O2
LDFLAGS_LINUX=-lrt

FIRESIM = ~/firesim/deploy/workloads
TEST_PROGS = nftop

default: $(addsuffix .riscv,$(TEST_PROGS))

%.riscv: %.o crt.o syscalls.o ac.o acl_fw_hashmap_dleft.o
	$(CC) -T ./utils/link.ld $(LDFLAGS) $^ -o $@
	cp *.ini ${FIRESIM}/
	cp *.json ${FIRESIM}/
	mkdir -p ${FIRESIM}/nf_workloads
	cp $@ ${FIRESIM}/nf_workloads

%.o: %.c config.h ./utils/* ./nfs/*
	$(CC) $(CFLAGS) -c $< -o $@

syscalls.o: ./utils/syscalls.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: ./utils/%.S
	$(CC) $(CFLAGS) -c $< -o $@

ac.o: ./data_embed/ac.c
	$(CC) $(CFLAGS) -c $< -o $@

acl_fw_hashmap_dleft.o: ./data_embed/acl_fw_hashmap_dleft.c
	$(CC) $(CFLAGS) -c $< -o $@

# make CONFIG='-DNF_STRINGS=\"l2_fwd:nat_tcp_v4:dpi:acl_fw\"' syscalls_test
syscalls_test: syscalls_test.c
	gcc $(CONFIG) -o syscalls_test syscalls_test.c

clean: 
	rm -f *.riscv *.o syscalls_test