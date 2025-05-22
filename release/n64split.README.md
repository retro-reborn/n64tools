# n64split: N64 ROM splitter, texture ripper, recursive MIPS disassembler

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
n64split [-c CONFIG] [-o OUTPUT_DIR] [-s SCALE] [-t] [-v] [-V] ROM
```

### Optional arguments:
- `-c CONFIG`     ROM configuration file (default: determine from checksum)
- `-o OUTPUT_DIR` output directory (default: {CONFIG.basename}.split)
- `-s SCALE`      amount to scale models by (default: 1024.0)
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

## Build System: build system to reconstruct ROM after making changes

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

Source is MIT licensed and available at:
https://github.com/queueRAM/sm64tools/

n64split makes use of the following libraries. See the licenses directory for
more information:
- libpng: PNG decoding and encoding
- capstone: raw MIPS disassembler
- libyaml: ROM config file reading

## Changelog:

### v0.4a: More data: music, collision, more configs
- new config files: SM64 (J), SM64 (E), MK64 (U)
- add IA4 and IA1 graphics types
- add command line options:
  - generate large texture from MIO0 block
  - generate procedure table
  - specify output directory
- disassembler changes:
  - add more address detection and %hi/%lo macro insertion
  - handle BAL instructions
- add automatic config file detection based on header checksum
- corrected a lot of textures in the SM64 configs
- add music sequence bank and instrument set parsing
- add collision data parser and export to Wavefront OBJ
- add sm64walk tool to walk through SM64 level scripts
- correct SM64 behavior script lengths

### v0.3.1a: Bugfixes and cleanup
- fix major bug introduced in n64graphics due to texture renaming
- update INFO and stats messages from n64split
- add ROM validation and support for .v64 (BADC ordered) ROMs
- add hello world example

### v0.3a: Complete disassembly and decoding of scripts
- 100% of behavior scripts decoded and most behavior references detected
- ~99% of asm sections disassembled
- update geo layout commands 0x0A, 0x0E, 0x0F, 0x12, 0x18, 0x19, 0x1C
- generate assembly files for standalone geo layout sections
- detect more LA that are split across JALs
- add option to configure number of columns in data ptr tables
- decode interaction table and functions
- decode cut scene tables and functions
- decode camera change table and functions
- decode camera preset tables and functions
- automatic detection and disassembly of dummy stub functions
- add examples to skip startup screens and start right in level
- switch HUD toggle example based off of Kaze's method from Green Stars
- removed individual texture subdirectories

### v0.2a: Most scripts decoded
- supports decoding level scripts, geometry layout, behavior scripts and assembly routines
- adds many more procedure labels
- automatically generates linker script from config file

### v0.1a: Initial release
- supports texture ripping, level decoding, and recursive disassembler
