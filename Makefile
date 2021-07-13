# Makefile for Desk Viking, a Bus Pirate inspired firmware for the STM32F103

PROJECT = desk-viking

ARCH = cortex-m
CHOPSTX ?= ../chopstx
LDSCRIPT = desk-viking.ld
CSRC = src/main.c \
       src/cmd/ccproxy.c src/cmd/cli.c src/cmd/cli_i2c.c src/cmd/cli_w1.c \
       src/proto/ccdbg.c src/proto/i2c.c src/proto/w1.c \
       src/util/debug.c src/util/dwt.c src/util/gpio.c src/util/tty.c \
       src/util/usb-cdc.c

CHIP=stm32f103

USE_SYS = yes
USE_USB = yes
ENABLE_OUTPUT_HEX = yes

###################################
CROSS ?= arm-none-eabi-
CC   = $(CROSS)gcc
LD   = $(CROSS)gcc
OBJCOPY   = $(CROSS)objcopy

MCU    = cortex-m3
CWARN  = -Wall -Wextra -Wstrict-prototypes
DEFS   = -DUSE_SYS3 -DFREE_STANDING -DMHZ=80
OPT    = -O3 -Os -g
LIBS   =
INCDIR = include

####################
include $(CHOPSTX)/rules.mk

board.h:
	@echo Please make a symbolic link \'board.h\' to a file in $(CHOPSTX)/board;
	@exit 1

distclean: clean
	rm -f board.h

build/main.o: board.h
