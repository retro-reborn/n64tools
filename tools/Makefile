################ Target Executable and Sources ###############

TARGET := montage

UTILS_SRC = ../src/utils/utils.c
SRC_FILES  := montage.c \
              ../src/utils/yamlconfig.c \
              $(UTILS_SRC)

##################### Compiler Options #######################

# OS Detection
ifeq ($(OS),Windows_NT)
	DETECTED_OS := windows
	MKDIR := mkdir
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		DETECTED_OS := linux
	endif
	ifeq ($(UNAME_S),Darwin)
		DETECTED_OS := macos
		BREW_PREFIX := $(shell brew --prefix)
	endif
	MKDIR := mkdir -p
endif

WIN64_CROSS = x86_64-w64-mingw32-
WIN32_CROSS = i686-w64-mingw32-
#CROSS     = $(WIN32_CROSS)
CC        = $(CROSS)gcc
LD        = $(CC)

INCLUDES  = -I.. -I../src -I../src/utils
DEFS      = 
# Add Homebrew includes for macOS
ifeq ($(DETECTED_OS),macos)
	INCLUDES += -I$(BREW_PREFIX)/include
endif
CFLAGS    = -Wall -Wextra -O2 -ffunction-sections -fdata-sections $(INCLUDES) $(DEFS)

# OS-specific linker flags
ifeq ($(DETECTED_OS),macos)
	# Ensure the Homebrew library path is properly set for macOS
	LDFLAGS += -L$(BREW_PREFIX)/lib -Wl,-dead_strip
else
	LDFLAGS += -s -Wl,--gc-sections
endif

LIBS      = -lconfig -lyaml

######################## Targets #############################

default: all

all: $(TARGET) matchsigs sm64collision

# Build target with all includes and libraries already in CFLAGS and LDFLAGS
$(TARGET): $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

# Build matchsigs using global CFLAGS and LDFLAGS
matchsigs: match_signatures.c $(UTILS_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lcapstone

sm64collision: sm64collision.c $(UTILS_SRC)
	$(CC) $(CFLAGS) -o $@ $^

sm64text: sm64text.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) matchsigs sm64collision sm64text

.PHONY: all clean default

