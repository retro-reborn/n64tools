################ Target Executable and Sources ###############
#
# Usage:
#   make               - Build with default optimization (3)
#   make OPTFLAG=0     - Build with custom optimization level (e.g., 0, 1, 2, g)
#   make clean         - Remove all build artifacts

# Define output directory structure
OBJ_DIR  = ./obj
BIN_DIR  = ./bin
SPLIT_DIR = $(OBJ_DIR)/n64split

# Define library
SM64_LIB := libn64.a

# Define all targets
TARGETS := sm64compress n64cksum mipsdisasm sm64extend f3d f3d2obj sm64geo n64graphics mio0 n64split sm64walk

# OS Detection
ifeq ($(OS),Windows_NT)
	DETECTED_OS := windows
	EXT := .exe
	RM := del /F /Q
	RMDIR := rd /S /Q
	MKDIR := mkdir
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		DETECTED_OS := linux
	endif
	ifeq ($(UNAME_S),Darwin)
		DETECTED_OS := macos
		BREW_PREFIX := $(shell brew --prefix)
		INCLUDES += -I$(BREW_PREFIX)/include
		LDFLAGS += -L$(BREW_PREFIX)/lib
	endif
	EXT :=
	RM := rm -f
	RMDIR := rm -rf
	MKDIR := mkdir -p
endif

##################### Compiler Options #######################

# Compiler setup
CC      = $(CROSS)gcc
LD      = $(CC)
AR      = $(CROSS)ar

# Optimization level
OPTFLAG ?= 3

# Common compilation flags
INCLUDES = -I. -I./ext -Isrc -Isrc/lib -Isrc/utils -Isrc/n64split -Isrc/mipsdisasm -Isrc/n64graphics -Isrc/mio0
CFLAGS   = -Wall -Wextra -O$(OPTFLAG) -ffunction-sections -fdata-sections $(INCLUDES) $(DEFS) -MMD

# OS-specific linker flags
ifeq ($(DETECTED_OS),macos)
	LDFLAGS = -Wl,-dead_strip
else
	LDFLAGS = -s -Wl,--gc-sections
endif

# Libraries
LIBS      = 
SPLIT_LIBS = -lcapstone -lyaml -lz

################### Source Definitions #######################

# Common utility sources used by multiple targets
UTILS_SRC = src/utils/utils.c src/utils/argparse.c

# Define source files for each target
LIB_SRC  := src/mio0/libmio0.c src/lib/libn64.c src/lib/libsfx.c $(UTILS_SRC)

n64cksum_SRC := src/n64cksum/n64cksum.c

sm64compress_SRC := src/sm64compress/sm64compress.c

mipsdisasm_SRC := src/mipsdisasm/mipsdisasm.c $(UTILS_SRC)

sm64extend_SRC := src/sm64extend/sm64extend.c

f3d_SRC := src/f3d/f3d.c $(UTILS_SRC)

f3d2obj_SRC := src/lib/blast.c src/f3d2obj/f3d2obj.c src/n64graphics/n64graphics.c $(UTILS_SRC)

sm64geo_SRC := src/sm64geo/sm64geo.c $(UTILS_SRC)

n64graphics_SRC := src/n64graphics/n64graphics.c $(UTILS_SRC)

mio0_SRC := src/mio0/libmio0.c

n64split_SRC := src/lib/blast.c src/mio0/libmio0.c src/lib/libsfx.c \
			   src/mipsdisasm/mipsdisasm.c src/n64graphics/n64graphics.c \
			   src/n64split/n64split.c src/n64split/n64split.sm64.geo.c \
			   src/n64split/n64split.sm64.behavior.c src/n64split/n64split.sm64.collision.c \
			   src/n64split/n64split.sound.c src/utils/strutils.c \
			   $(UTILS_SRC) src/utils/yamlconfig.c

sm64walk_SRC := src/sm64walk/sm64walk.c

# Convert source files to object files
LIB_OBJS = $(addprefix $(OBJ_DIR)/,$(LIB_SRC:.c=.o))

##################### Build Rules ###########################

default: all

all: $(addprefix $(BIN_DIR)/,$(addsuffix $(EXT),$(TARGETS)))

# General rule for object files
$(OBJ_DIR)/%.o: %.c
	@$(MKDIR) $(dir $@)
	@[ -d $(BIN_DIR) ] || $(MKDIR) $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

# Special rules for files that need external libraries
$(OBJ_DIR)/src/mipsdisasm/mipsdisasm.o: src/mipsdisasm/mipsdisasm.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -I$(BREW_PREFIX)/include -o $@ -c $<

$(OBJ_DIR)/src/utils/yamlconfig.o: src/utils/yamlconfig.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CFLAGS) -I$(BREW_PREFIX)/include -o $@ -c $<

# Build library
$(SM64_LIB): $(LIB_OBJS)
	$(AR) rcs $@ $^

# Rules for each target
$(BIN_DIR)/n64cksum$(EXT): $(addprefix $(OBJ_DIR)/,$(n64cksum_SRC:.c=.o)) $(SM64_LIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR)/sm64compress$(EXT): $(addprefix $(OBJ_DIR)/,$(sm64compress_SRC:.c=.o)) $(SM64_LIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR)/sm64extend$(EXT): $(addprefix $(OBJ_DIR)/,$(sm64extend_SRC:.c=.o)) $(SM64_LIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR)/f3d$(EXT): $(addprefix $(OBJ_DIR)/,$(f3d_SRC:.c=.o))
	$(LD) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/f3d2obj$(EXT): $(addprefix $(OBJ_DIR)/,$(f3d2obj_SRC:.c=.o))
	$(LD) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/sm64geo$(EXT): $(addprefix $(OBJ_DIR)/,$(sm64geo_SRC:.c=.o))
	$(LD) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/n64graphics$(EXT): $(n64graphics_SRC)
	$(CC) $(CFLAGS) -DN64GRAPHICS_STANDALONE $^ $(LDFLAGS) -o $@

$(BIN_DIR)/mio0$(EXT): src/mio0/libmio0.c $(UTILS_SRC)
	$(CC) $(CFLAGS) -DMIO0_STANDALONE $^ $(LDFLAGS) -o $@

$(BIN_DIR)/mipsdisasm$(EXT): $(mipsdisasm_SRC)
ifeq ($(DETECTED_OS),macos)
	$(CC) $(CFLAGS) -I$(BREW_PREFIX)/include -DMIPSDISASM_STANDALONE $^ $(LDFLAGS) -o $@ -L$(BREW_PREFIX)/lib -lcapstone
else
	$(CC) $(CFLAGS) -DMIPSDISASM_STANDALONE $^ $(LDFLAGS) -o $@ -lcapstone
endif

$(BIN_DIR)/n64split$(EXT): $(addprefix $(OBJ_DIR)/,$(n64split_SRC:.c=.o))
ifeq ($(DETECTED_OS),macos)
	$(LD) $(LDFLAGS) -o $@ $^ -L$(BREW_PREFIX)/lib $(SPLIT_LIBS)
else
	$(LD) $(LDFLAGS) -o $@ $^ $(SPLIT_LIBS)
endif

$(BIN_DIR)/sm64walk$(EXT): $(sm64walk_SRC) $(SM64_LIB)
	$(CC) $(CFLAGS) -o $@ $^

# Utility target
rawmips: src/mipsdisasm/rawmips.c src/utils/utils.c
ifeq ($(DETECTED_OS),macos)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@$(EXT) $^ -L$(BREW_PREFIX)/lib -lcapstone
else
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@$(EXT) $^ -lcapstone
endif

# Clean target
clean:
	$(RM) $(wildcard $(OBJ_DIR)/*/*.o) $(wildcard $(OBJ_DIR)/*/*.d) $(SM64_LIB)
	$(RM) $(addprefix $(BIN_DIR)/,$(addsuffix $(EXT),$(TARGETS)))
	-@[ -d $(SPLIT_DIR) ] && $(RMDIR) $(SPLIT_DIR) 2>/dev/null || true
	-@[ -d $(OBJ_DIR) ] && $(RMDIR) $(OBJ_DIR) 2>/dev/null || true
	-@[ -d $(BIN_DIR) ] && $(RMDIR) $(BIN_DIR) 2>/dev/null || true

.PHONY: all clean default

# Include dependency files
-include $(wildcard $(OBJ_DIR)/*/*.d)
