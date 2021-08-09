# Makefile for Desk Viking, a Bus Pirate inspired firmware for the STM32F103

PROJECT = desk-viking
GIT_VERSION := $(shell git describe --tags --dirty="-modified")

CHOPSTX ?= ../chopstx
LDSCRIPT = desk-viking.ld
CSRC = src/main.c \
       src/cmd/bpbin.c src/cmd/bpbin_i2c.c src/cmd/bpbin_raw.c \
       src/cmd/bpbin_w1.c \
       src/cmd/ccproxy.c \
       src/cmd/cli.c src/cmd/cli_dio.c src/cmd/cli_i2c.c src/cmd/cli_w1.c \
       src/proto/buspirate.c src/proto/ccdbg.c src/proto/i2c.c src/proto/w1.c \
       src/util/debug.c src/util/tty.c src/util/usb-cdc.c src/util/util.c

USE_SYS = yes
USE_USB = yes

ifeq ($(EMULATION),)
ARCH   = cortex-m
MCU    = cortex-m3
CHIP   = stm32f103
CROSS ?= arm-none-eabi-
DEFS   = -DUSE_SYS3 -DFREE_STANDING -DMHZ=72
LIBS   =
ENABLE_OUTPUT_HEX = yes
else
ARCH = gnu-linux
CHIP = gnu-linux
DEFS = -DGNU_LINUX_EMULATION -DMHZ=80
LIBS = -lpthread
endif

# These sources have per-platform versions.
CSRC += src/util/dwt-$(CHIP).c src/util/gpio-$(CHIP).c

CC      = $(CROSS)gcc
LD      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy

CWARN  = -Wall -Wextra -Wstrict-prototypes
OPT    = -O3 -Os -g
INCDIR = include

include $(CHOPSTX)/rules.mk

include/version.h: include/version.h.in .git/index
	sed -e 's/@GIT@/$(GIT_VERSION)/' include/version.h.in > include/version.h

board.h:
	@echo Please make a symbolic link \'board.h\' to a file in $(CHOPSTX)/board;
	@exit 1

distclean: clean
	rm -f board.h include/version.h

build/main.o: board.h include/version.h

build/cli.o: include/version.h
