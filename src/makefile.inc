# Cross-compiling (e.g., on Mac OS X, install arm-none-eabi-gcc with MacPorts)
CROSS_PREFIX= arm-none-eabi-
CC = $(CROSS_PREFIX)gcc
AS = $(CROSS_PREFIX)as
AR = $(CROSS_PREFIX)ar
LD = $(CROSS_PREFIX)ld
OBJCOPY = $(CROSS_PREFIX)objcopy
OBJDUMP = $(CROSS_PREFIX)objdump

CFLAGS = -march=armv6 -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall
LDFLAGS = -L.
ASFLAGS = -march=armv6 

