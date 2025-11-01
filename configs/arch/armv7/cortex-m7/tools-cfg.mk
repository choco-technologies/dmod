#================================================================================================================================
# 	Default tools configuration
#================================================================================================================================

#
#	Toolchain configuration
#
ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE=arm-none-eabi-
endif
ifeq ($(CPUCONFIG_CFLAGS),)
	CPUCONFIG_CFLAGS=-mcpu=cortex-m7 -mthumb -mno-unaligned-access -DGCC_ARMCM7 -mpic-data-is-text-relative
endif
ifeq ($(CPUCONFIG_CXXFLAGS),)
	CPUCONFIG_CXXFLAGS=-mcpu=cortex-m7 -mthumb -mno-unaligned-access -DGCC_ARMCM7 -mpic-data-is-text-relative
endif
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld
OBJDUMP=$(CROSS_COMPILE)objdump
OBJCOPY=$(CROSS_COMPILE)objcopy
AR=$(CROSS_COMPILE)ar
MAKE=make
MKDIR=mkdir
RM=rm
CFLAGS=-Wall -std=c11 $(CPUCONFIG_CFLAGS)
CXXFLAGS=-Wall -std=c++17 $(CPUCONFIG_CXXFLAGS)
LFLAGS=$(CPUCONFIG_LDFLAGS)