#PREFIX=powerpc-elf
PREFIX=powerpc-linux

#PREFIX=arm-none-linux-gnueabi

#export LD_LIBRARY_PATH=/home/sunmingze/ppc/ppc_toolchain/lib
#export LD_LIBRARY_PATH=/home/sunmingze/ppc/ppc_toolchain_linux/lib

CC=$(PREFIX)-gcc
AS=$(PREFIX)-as
LD=$(PREFIX)-ld
OBJCOPY=$(PREFIX)-objcopy

OPT=-nostdlib

LD_SCRIPT=test.ld

obj = startup.o test.o

test:$(obj)
	$(LD) -T $(LD_SCRIPT) $(obj) -o des_demo.elf
	$(OBJCOPY) -O binary des_demo.elf des_demo.bin

startup.o:startup.S
	$(AS) -g startup.S -o startup.o -maltivec -m440

test.o:test.s
	$(AS) -g test.s -o test.o -maltivec -m440
test.s:des_demo.c
	$(CC) -S -static -nostdlib -g des_demo.c -o test.s

clean:
	rm -f *.o *.elf *.bin *.s	
