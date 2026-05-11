#================================================================================================================================
# 	Default tools configuration
#================================================================================================================================
DMOD_ARCH=xtensa-esp32s3
DMOD_CPU=esp32s3

#
#	Toolchain configuration
#
ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE=xtensa-esp32s3-elf-
endif
ifeq ($(CPUCONFIG_CFLAGS),)
	CPUCONFIG_CFLAGS=-mlongcalls -mtext-section-literals -fstrict-volatile-bitfields -Wno-frame-address -DDMOD_ARCH="$(DMOD_ARCH)" -DDMOD_CPU="$(DMOD_CPU)"
endif
ifeq ($(CPUCONFIG_CXXFLAGS),)
	CPUCONFIG_CXXFLAGS=-mlongcalls -mtext-section-literals -fstrict-volatile-bitfields -Wno-frame-address -DDMOD_ARCH="$(DMOD_ARCH)" -DDMOD_CPU="$(DMOD_CPU)"
endif
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld
OBJDUMP=$(CROSS_COMPILE)objdump
OBJCOPY=$(CROSS_COMPILE)objcopy
AR=$(CROSS_COMPILE)ar
SIZE=$(CROSS_COMPILE)size
MAKE=make
MKDIR=mkdir
RM=rm
CFLAGS=-Wall -std=c11 $(CPUCONFIG_CFLAGS)
CXXFLAGS=-Wall -std=c++17 $(CPUCONFIG_CXXFLAGS)
LFLAGS=$(CPUCONFIG_LDFLAGS)