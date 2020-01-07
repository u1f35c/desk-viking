# Makefile for Desk Viking, a Bus Pirate inspired firmware for the STM32F103

PROJECT = desk-viking

CHOPSTX ?= ../chopstx
LDSCRIPT = desk-viking.ld
CSRC = src/main.c \
       src/cmd/cli.c \
       src/util/debug.c src/util/gpio.c src/util/usb-cdc.c

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
