include ../makefile.inc

CFLAGS += -iquote ../ -Wno-error=unused-but-set-variable
ASFLAGS += -I ../
LIB = usb_dwc.o usb_misc.o

libusb.a : $(LIB) 
	$(AR) -rcs libusb.a $(LIB)


all: libusb.a

clean: 
	rm -f *.o libusb.a
