include ../makefile.inc

CFLAGS += -iquote ../ -Wno-error=unused-but-set-variable
ASFLAGS += -I ../
LIB = usb_dwc.o usb_misc.o

libusb.a : $(LIB) 
	$(AR) -rcs libusb.a $(LIB)


all: libusb.a

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*.o *.d libusb.a *.asm *.sym vectors.S bootblock entryother \
	initcode initcode.out kernel xv6.img fs.img kernel.elf memfs mkfs \
	.gdbinit \
	$(UPROGS)
