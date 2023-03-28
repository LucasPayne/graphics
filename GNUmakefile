#
# GNUmakefile
#

#--------------------------------------------------------------------------------
# Configuration
#--------------------------------------------------------------------------------
# Use the vulkan loader and vulkan headers installed from the LunarG vulkan repos.
USE_LOCAL_VULKAN_SDK=0

#--------------------------------------------------------------------------------
# Paths
#--------------------------------------------------------------------------------
INCLUDE_PATH=\
    libraries/include\
    src
LIB_PATH=\
    libraries/lib
ifeq ($(USE_LOCAL_VULKAN_SDK), 1)
    INCLUDE_PATH += /usr/local/include
    LIB_PATH += /usr/local/lib
endif

#--------------------------------------------------------------------------------
# Flags
#--------------------------------------------------------------------------------
CC=g++
CFLAGS_IGNORE_WARNINGS += -Wno-missing-field-initializers # Vulkan struct initialization `= {type enum}` is convenient.
CFLAGS_IGNORE_WARNINGS += -Wno-unused-parameter # Sometimes function objects need an unused parameter to match a type.
CFLAGS_IGNORE_WARNINGS += -Wno-switch # Ignore warnings if enum switch is non-exaustive. (might want to re-enable this.)
CFLAGS   = -std=c++2a -Wextra -Wall ${CFLAGS_IGNORE_WARNINGS} -ggdb3 -march=native -O0
CFLAGS  += $(foreach d, $(INCLUDE_PATH), -I$d) # Includes search path.
LDFLAGS  = $(foreach d, $(LIB_PATH), -L$d) # Library search path.
LDFLAGS += $(foreach d, $(LIB_PATH), -Wl,-rpath=$(realpath $d)) # Embed the whole library search path in the rpath.
LDFLAGS += -lglfw \
	   -ljsoncpp \
           -lvulkan \
           -ldl

#--------------------------------------------------------------------------------
# Rules
#--------------------------------------------------------------------------------
SOURCE_FILES=\
    src/main.cc\
    src/vk_print.cc\
    src/vk.cc
INCLUDE_FILES=\
    src/vk_print.h\
    src/vk_util.h\
    src/vk.h

build/main: GNUmakefile $(SOURCE_FILES) $(INCLUDE_FILES)
	$(CC) $(CFLAGS) -o $@ $(SOURCE_FILES) $(LDFLAGS)

clean:
	rm build/main
