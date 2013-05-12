include ../makefile.inc

CFLAGS += -iquote ../ -Wno-error=unused-but-set-variable
ASFLAGS += -I ../
LIBUSB = dwc_otg.o dwc_otg-hcd.o usb.o usb_hub.o usb_kbd.o


all: $(LIBUSB)

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg \
	*.o *.d *.asm *.sym vectors.S bootblock entryother \
	initcode initcode.out kernel xv6.img fs.img kernel.elf memfs mkfs \
	.gdbinit \
	$(UPROGS)
