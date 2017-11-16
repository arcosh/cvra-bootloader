#
# settings
################################################################################

PROJNAME = bootloader

# where to place the build files
BUILD_DIR = build

# platform specific source files
PLATFORM_CSRC = platform.c

PROJ_ROOT = ../..
PLATFORM_PATH = platform/can-io-board/

include ../verbosity.mk

#
# low level settings
################################################################################

include include.mk

CSRC     += $(addprefix $(PLATFORM_PATH), $(PLATFORM_CSRC))

# libraries & includes
INCDIR   = ./ $(PROJ_ROOT)/libopencm3/include $(PROJ_ROOT)/
INCDIR   += $(addprefix $(PROJ_ROOT)/, $(PACKAGER_INCDIR))

LIBS     = -lc -lnosys
LIBS     += -L$(PROJ_ROOT)/libopencm3/lib
LIBS     += -lopencm3_stm32f3

LDSCRIPT = ./linkerscript.ld

# defines
DEFS += -DSTM32F3

# Cortex-M4f
CFLAGS   += -mthumb -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16
LFLAGS   += -mthumb -mcpu=cortex-m4 -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16

# C & C++ compiler flags
CFLAGS   += -std=gnu99
CFLAGS   += -fno-common -ffunction-sections -fdata-sections
CFLAGS   += $(OPTIMIZATION) -g  -Wall -Wextra -Wno-unused-parameter
CFLAGS   += $(DEFS) -fomit-frame-pointer -MD
CFLAGS   += $(addprefix  -I, $(INCDIR))
# C only flags
CCFLAGS  += -Wstrict-prototypes
# C++ only flags
CXXFLAGS += -fno-rtti -fno-exceptions -fno-unwind-tables
CXXFLAGS += -fno-use-cxa-atexit
# Linker flags
LFLAGS   += $(LIBS) -T$(LDSCRIPT) -nostartfiles -Wl,-Map=$(PROJNAME).map
LFLAGS   += -Wl,--gc-sections

CCFLAGS  += $(CFLAGS)
CXXFLAGS += $(CFLAGS)
ASMFLAGS += $(CFLAGS)

COBJS = $(addprefix $(BUILD_DIR)/, $(CSRC:.c=.o))
ASMOBJS = $(addprefix $(BUILD_DIR)/, $(ASMSRC:.s=.o))
CXXOBJS = $(addprefix $(BUILD_DIR)/, $(CXXSRC:.cpp=.o))

OBJS = $(COBJS) $(ASMOBJS) $(CXXOBJS)

# gcc optimization level
OPTIMIZATION    = -Os

CC  = arm-none-eabi-gcc
CXX = arm-none-eabi-g++
AS  = arm-none-eabi-gcc -x assembler-with-cpp
LD  = arm-none-eabi-g++
AR  = arm-none-eabi-ar
OC  = arm-none-eabi-objcopy
OD  = arm-none-eabi-objdump
NM  = arm-none-eabi-nm
SZ  = arm-none-eabi-size

MKDIR = mkdir -p

#
# targets:
################################################################################

-include ../targets.mk
-include ../openocd.mk


