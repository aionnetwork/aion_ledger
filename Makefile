#*******************************************************************************
# Copyright (c) 2017-2018 Aion foundation.
#
#     This file is part of the aion network project.
#
#     The aion network project is free software: you can redistribute it 
#     and/or modify it under the terms of the GNU General Public License 
#     as published by the Free Software Foundation, either version 3 of 
#     the License, or any later version.
#
#     The aion network project is distributed in the hope that it will 
#     be useful, but WITHOUT ANY WARRANTY; without even the implied 
#     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
#     See the GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with the aion network project source files.  
#     If not, see <https://www.gnu.org/licenses/>.
#
#     The aion network project leverages useful source code from other 
#     open source projects. We greatly appreciate the effort that was 
#     invested in these projects and we thank the individual contributors 
#     for their work. For provenance information and contributors
#     please see <https://github.com/aionnetwork/aion/wiki/Contributors>.
#
# Contributors to the aion source files in decreasing order of code volume:
#     Aion foundation.
#     <ether.camp> team through the ethereumJ library.
#     Ether.Camp Inc. (US) team through Ethereum Harmony.
#     John Tromp through the Equihash solver.
#     Samuel Neves through the BLAKE2 implementation.
#     Zcash project team.
#     Bitcoinj team.
#*******************************************************************************

ifeq ($(BOLOS_SDK),)
$(error BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines

# Main app configuration

APPNAME = "Aion"
APPVERSION = 1.0.0
APP_LOAD_PARAMS = --path "44'/425'" --appFlags 0x40 $(COMMON_LOAD_PARAMS)
ICONNAME=icon.gif

# Build configuration

APP_SOURCE_PATH += src
SDK_SOURCE_PATH += lib_stusb lib_stusb_impl

DEFINES += APPVERSION=\"$(APPVERSION)\"

DEFINES += OS_IO_SEPROXYHAL IO_SEPROXYHAL_BUFFER_SIZE_B=128
DEFINES += HAVE_BAGL HAVE_SPRINTF
DEFINES += PRINTF\(...\)=

DEFINES += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=7 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
#DEFINES += TESTING_ENABLED

# Compiler, assembler, and linker

ifneq ($(BOLOS_ENV),)
$(info BOLOS_ENV=$(BOLOS_ENV))
CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
GCCPATH := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
else
$(info BOLOS_ENV is not set: falling back to CLANGPATH and GCCPATH)
endif

ifeq ($(CLANGPATH),)
$(info CLANGPATH is not set: clang will be used from PATH)
endif

ifeq ($(GCCPATH),)
$(info GCCPATH is not set: arm-none-eabi-* will be used from PATH)
endif

CC := $(CLANGPATH)clang
CFLAGS += -O3 -Os

AS := $(GCCPATH)arm-none-eabi-gcc
AFLAGS +=

LD := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS += -O3 -Os
LDLIBS += -lm -lgcc -lc

# Main rules

all: default

load: all
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

delete:
	python -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

# Import generic rules from the SDK

include $(BOLOS_SDK)/Makefile.rules

