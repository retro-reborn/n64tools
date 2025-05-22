# n64split
N64 ROM Splitter, Texture Ripper, and Recursive MIPS Disassembler

n64split is a tool capable of splitting N64 ROMs into assets that can be reused
and rebuilt. Using an outline defined in a config file, n64split will produce
assembly code, textures, levels, geometry, behavior scripts, music, and models;
and add on a build system that is able to take changes made to those files and 
reconstruct a ROM. The resulting ROM is almost bit for bit compatible with the
input ROM (exceptions: some macro instructions and relocation of MIO0 data).

The disassembler is recursive, so it will disassemble all procedures it 
discovers through JALs. It also somewhat intelligently decodes instruction 
pairs to macro instructions (like LUI/ADDIU to LA and LUI/LW pairs to LW) and 
adds proper labels to JALs and local branches.

## Features
- Splits ROM into assets: asm, textures, models, levels, behavior data
- Generates build files to rebuild the ROM
- Intelligent recursive disassembler
- Generic config file system to support multiple games

## Basic Usage
Drag and drop a ROM on the split.bat file.
This will split the ROM based on the contents of the configuration file 
"sm64.u.config". See the comments in that file if you want to adapt it.

Output goes in directory named `{CONFIG.name}.split`. e.g.:
```
sm64.split/
 +- bin/            - raw binary data from undecoded sections and MIO0 blocks
 +- geo/            - decoded geo layout data
 +- levels/         - decoded level data
 +- models/         - level and collision models
 +- music/          - M64 music files
 +- textures/       - all ripped textures
 +- behavior_data.s - behavior command bank
 +- sm64.s          - top level assembly
 +- Makefile.split  - generated Makefile with texture and level dependencies
```

## Detailed Usage
```
n64split [-c CONFIG] [-k] [-m] [-o OUTPUT_DIR] [-s SCALE] [-t] [-v] [-V] ROM
```

### Optional arguments:
- `-c CONFIG`     ROM configuration file (default: determine from checksum)
- `-o OUTPUT_DIR` output directory (default: {CONFIG.basename}.split)
- `-s SCALE`      amount to scale models by (default: 1024.0)
- `-k`            keep going as much as possible after error
- `-m`            merge related instructions in to pseudoinstructions
- `-p`            generate procedure table for analysis
- `-t`            generate large texture for MIO0 blocks
- `-v`            verbose progress output
- `-V`            print version information

### File arguments:
- `ROM`        input ROM file

### Example usage:
```
n64split "Super Mario 64 (U).z64"
```

## Build System
The build system relies on GNU make to detect changes in resources and 
mips64-elf assembler and linker to rebuild the ROM. You could of course 
circumvent 'make' and just build everything every time with a batch script, 
but this is incredibly slow due to all the texture encoding and a terribly 
inefficient MIO0 compressor. A win32 build of make is included, but you'll 
need to either build or obtain** mips64-elf binutils on your own if you want 
to rebuild the image.

**  The developer of cen64 also makes n64chain which provides a prebuilt
    mips64 toolchain. The current list is below, but you can find more info
    on his website: http://git.cen64.com/?p=n64chain.git
  - Windows (x86_64): https://www.cen64.com/uploads/n64chain-win64-tools.zip
  - Linux (x86_64): https://www.cen64.com/uploads/n64chain-linux64-tools.tgz

### Usage:
With mips64-elf bin directory in your PATH, just run 'make' from the
command prompt within the 'sm64.split' directory, and it will handle the rest.

```
make
mips64-elf-as -mtune=vr4300 -march=vr4300 -o build/sm64.o gen/sm64.s
mips64-elf-ld -Tn64.ld -Map build/sm64.map -o build/sm64.elf build/sm64.o 
mips64-elf-objcopy build/sm64.elf build/sm64.bin -O binary
tools/n64cksum build/sm64.bin sm64.gen.z64
```

### Build output:
Build artifacts will go in 'build' subdirectory. Complete ROM is sm64.z64

## Dependencies
n64split makes use of the following libraries:
- libpng: PNG decoding and encoding
- capstone: raw MIPS disassembler
- libyaml: ROM config file reading
