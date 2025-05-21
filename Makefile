################ Target Executable and Sources ###############

SM64_LIB        := libsm64.a
COMPRESS_TARGET := sm64compress
CKSUM_TARGET    := n64cksum
DISASM_TARGET   := mipsdisasm
EXTEND_TARGET   := sm64extend
F3D_TARGET      := f3d
F3D2OBJ_TARGET  := f3d2obj
GEO_TARGET      := sm64geo
GRAPHICS_TARGET := n64graphics
MIO0_TARGET     := mio0
SPLIT_TARGET    := n64split
WALK_TARGET     := sm64walk

LIB_SRC_FILES  := libmio0.c    \
				  libsm64.c    \
				  libsfx.c     \
				  utils.c

CKSUM_SRC_FILES := n64cksum.c

COMPRESS_SRC_FILES := sm64compress.c

DISASM_SRC_FILES := mipsdisasm.c \
					utils.c

EXTEND_SRC_FILES := sm64extend.c

F3D_SRC_FILES := f3d.c \
				 utils.c

F3D2OBJ_SRC_FILES := blast.c \
					 f3d2obj.c \
					 n64graphics.c \
					 utils.c

GEO_SRC_FILES := sm64geo.c \
				 utils.c

GRAPHICS_SRC_FILES := n64graphics.c \
					  utils.c

MI0_SRC_FILES := libmio0.c \
				 libmio0.h

SPLIT_SRC_FILES := blast.c \
				   libmio0.c \
				   libsfx.c \
				   mipsdisasm.c \
				   n64graphics.c \
				   n64split/n64split.c \
				   n64split/n64split.sm64.geo.c \
				   n64split/n64split.sm64.behavior.c \
				   n64split/n64split.sm64.collision.c \
				   n64split/n64split.sound.c \
				   strutils.c \
				   utils.c \
				   yamlconfig.c

WALK_SRC_FILES := sm64walk.c

OBJ_DIR     = ./obj
BIN_DIR     = ./bin
SPLIT_DIR   = $(OBJ_DIR)/n64split

##################### OS Detection ###########################

# Detect operating system
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

# Set cross compiler for Windows if building on other platforms
ifeq ($(DETECTED_OS),windows)
	# Use default system compiler on Windows
	CROSS :=
else
	# Uncomment if you want to cross-compile for Windows
	#WIN64_CROSS := x86_64-w64-mingw32-
	#WIN32_CROSS := i686-w64-mingw32-
	#CROSS := $(WIN64_CROSS)
	CROSS :=
endif

CC        = $(CROSS)gcc
LD        = $(CC)
AR        = $(CROSS)ar

INCLUDES  = -I. -I./ext
DEFS      = 

# OS-specific linker flags
ifeq ($(DETECTED_OS),macos)
	LDFLAGS = -Wl,-dead_strip
else
	LDFLAGS = -s -Wl,--gc-sections
endif

# Release flags
CFLAGS    = -Wall -Wextra -O2 -ffunction-sections -fdata-sections $(INCLUDES) $(DEFS) -MMD

# Debug flags
#CFLAGS    = -Wall -Wextra -O0 -g $(INCLUDES) $(DEFS) -MMD
#LDFLAGS   =

LIBS      = 
SPLIT_LIBS = -lcapstone -lyaml -lz

LIB_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(LIB_SRC_FILES:.c=.o))
CKSUM_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(CKSUM_SRC_FILES:.c=.o))
COMPRESS_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(COMPRESS_SRC_FILES:.c=.o))
EXTEND_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(EXTEND_SRC_FILES:.c=.o))
F3D_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(F3D_SRC_FILES:.c=.o))
F3D2OBJ_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(F3D2OBJ_SRC_FILES:.c=.o))
GEO_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(GEO_SRC_FILES:.c=.o))
MI0_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(MI0_SRC_FILES:.c=.o))
SPLIT_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(SPLIT_SRC_FILES:.c=.o))
WALK_OBJ_FILES = $(addprefix $(OBJ_DIR)/,$(WALK_SRC_FILES:.c=.o))
OBJ_FILES = $(LIB_OBJ_FILES) $(CKSUM_OBJ_FILES) $(COMPRESS_OBJ_FILES) \
			$(EXTEND_OBJ_FILES) $(F3D_OBJ_FILES) $(F3D2OBJ_OBJ_FILES) \
			$(GEO_OBJ_FILES) $(MI0_OBJ_FILES) $(SPLIT_OBJ_FILES) \
			$(WALK_OBJ_FILES)
DEP_FILES = $(OBJ_FILES:.o=.d)

######################## Targets #############################

default: all

all: $(EXTEND_TARGET) $(COMPRESS_TARGET) $(MIO0_TARGET) $(CKSUM_TARGET) \
	 $(SPLIT_TARGET) $(F3D_TARGET) $(F3D2OBJ_TARGET) $(GRAPHICS_TARGET) \
	 $(DISASM_TARGET) $(GEO_TARGET) $(WALK_TARGET)

$(OBJ_DIR)/%.o: %.c
	@[ -d $(OBJ_DIR) ] || $(MKDIR) $(OBJ_DIR)
	@[ -d $(BIN_DIR) ] || $(MKDIR) $(BIN_DIR)
	@[ -d $(SPLIT_DIR) ] || $(MKDIR) $(SPLIT_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

# Special rules for files that need external libraries
$(OBJ_DIR)/mipsdisasm.o: mipsdisasm.c
	@[ -d $(OBJ_DIR) ] || $(MKDIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(BREW_PREFIX)/include -o $@ -c $<

$(OBJ_DIR)/n64graphics.o: n64graphics.c
	@[ -d $(OBJ_DIR) ] || $(MKDIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/yamlconfig.o: yamlconfig.c
	@[ -d $(OBJ_DIR) ] || $(MKDIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(BREW_PREFIX)/include -o $@ -c $<

$(SM64_LIB): $(LIB_OBJ_FILES)
	$(RM) $@
	$(AR) rcs $@ $^

$(CKSUM_TARGET): $(CKSUM_OBJ_FILES) $(SM64_LIB)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) $^ $(LIBS)

$(COMPRESS_TARGET): $(COMPRESS_OBJ_FILES) $(SM64_LIB)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) $^ $(LIBS)

$(EXTEND_TARGET): $(EXTEND_OBJ_FILES) $(SM64_LIB)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) $^ $(LIBS)

$(F3D_TARGET): $(F3D_OBJ_FILES)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) $^

$(F3D2OBJ_TARGET): $(F3D2OBJ_OBJ_FILES)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) $^

$(GEO_TARGET): $(GEO_OBJ_FILES)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) $^

$(GRAPHICS_TARGET): $(GRAPHICS_SRC_FILES)
	$(CC) $(CFLAGS) -DN64GRAPHICS_STANDALONE $^ $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT)

$(MIO0_TARGET): $(MI0_SRC_FILES)
	$(CC) $(CFLAGS) -DMIO0_STANDALONE $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) $<

$(DISASM_TARGET): $(DISASM_SRC_FILES)
ifeq ($(DETECTED_OS),macos)
	$(CC) $(CFLAGS) -I$(BREW_PREFIX)/include -DMIPSDISASM_STANDALONE $^ $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) -L$(BREW_PREFIX)/lib -lcapstone
else
	$(CC) $(CFLAGS) -DMIPSDISASM_STANDALONE $^ $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) -lcapstone
endif

$(SPLIT_TARGET): $(SPLIT_OBJ_FILES)
ifeq ($(DETECTED_OS),macos)
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) $^ -L$(BREW_PREFIX)/lib $(SPLIT_LIBS)
else
	$(LD) $(LDFLAGS) -o $(BIN_DIR)/$@$(EXT) $^ $(SPLIT_LIBS)
endif

$(WALK_TARGET): $(WALK_SRC_FILES) $(SM64_LIB)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@$(EXT) $^

rawmips: rawmips.c utils.c
ifeq ($(DETECTED_OS),macos)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@$(EXT) $^ -L$(BREW_PREFIX)/lib -lcapstone
else
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@$(EXT) $^ -lcapstone
endif

clean:
	$(RM) $(OBJ_FILES) $(DEP_FILES) $(SM64_LIB)
	$(RM) $(BIN_DIR)/$(CKSUM_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(COMPRESS_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(DISASM_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(EXTEND_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(F3D_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(F3D2OBJ_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(GEO_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(MIO0_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(GRAPHICS_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(SPLIT_TARGET)$(EXT)
	$(RM) $(BIN_DIR)/$(WALK_TARGET)$(EXT)
	$(RM) *.d
ifeq ($(DETECTED_OS),windows)
	if exist $(SPLIT_DIR) $(RMDIR) $(SPLIT_DIR)
	if exist $(OBJ_DIR) $(RMDIR) $(OBJ_DIR)
	if exist $(BIN_DIR) $(RMDIR) $(BIN_DIR)
else
ifeq ($(DETECTED_OS),macos)
	-@[ -d $(SPLIT_DIR) ] && rmdir $(SPLIT_DIR) 2>/dev/null || true
	-@[ -d $(OBJ_DIR) ] && rmdir $(OBJ_DIR) 2>/dev/null || true
	-@[ -d $(BIN_DIR) ] && rmdir $(BIN_DIR) 2>/dev/null || true
else
	-@[ -d $(SPLIT_DIR) ] && rmdir --ignore-fail-on-non-empty $(SPLIT_DIR) || true
	-@[ -d $(OBJ_DIR) ] && rmdir --ignore-fail-on-non-empty $(OBJ_DIR) || true
	-@[ -d $(BIN_DIR) ] && rmdir --ignore-fail-on-non-empty $(BIN_DIR) || true
endif
endif

.PHONY: all clean default

#################### Dependency Files ########################

-include $(DEP_FILES)
