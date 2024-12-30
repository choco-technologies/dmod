#================================================================================================================================
# 	Default tools configuration
#================================================================================================================================

#
#	Toolchain configuration
#
ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE=
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
CFLAGS=-Wall -std=c99 $(CPUCONFIG_CFLAGS)
CXXFLAGS=-Wall -std=c++17 $(CPUCONFIG_CXXFLAGS)
LFLAGS=$(CPUCONFIG_LDFLAGS)