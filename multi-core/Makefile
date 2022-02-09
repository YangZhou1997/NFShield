CC=riscv64-unknown-elf-gcc
CFLAGS=-mcmodel=medany -Wall -O2 -fno-common -fno-builtin-printf
LDFLAGS=-static -nostdlib -nostartfiles -lgcc

CC_LINUX=riscv64-unknown-linux-gnu-gcc
CFLAGS_LINUX=-Wall -O2
LDFLAGS_LINUX=-lrt

multi-core: multi-core.o crt.o syscalls.o
	$(CC) -T link.ld $(LDFLAGS) $^ -o $@
	cp multi-core workloads/

%.o: %.c util.h encoding.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@
