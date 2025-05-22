# sm64extend
Super Mario 64 ROM Extender

sm64extend is a tool designed to work with Super Mario 64 ROMs, allowing you to extend the ROM size and decompress MIO0 blocks to the extended area.

## Features
- Accepts Z64 (BE), V64 (byte-swapped), or N64 (little-endian) ROMs as input
- Works with US, European, Japanese, and Shindou ROMs
- Decompresses all MIO0 blocks from ROM to extended area
- Configurable extended ROM size (default 64 MB)
- Configurable padding between MIO0 blocks (default 32 KB)
- Configurable MIO0 block alignment (default 1 byte)
- Changes all 0x18 level commands to 0x17
- Creates MIO0 headers for all 0x1A level commands
- Optionally fills old MIO0 blocks with 0x01
- Optionally dump compressed and uncompressed MIO0 data to files
- Updates assembly reference to MIO0 blocks
- Recalculates ROM header checksums

## Usage
```console
sm64extend [-a ALIGNMENT] [-p PADDING] [-s SIZE] [-d] [-f] [-v] FILE [OUT_FILE]
```

### Options
- `-a ALIGNMENT` Byte boundary to align MIO0 blocks (default = 1).
- `-p PADDING` Padding to insert between MIO0 blocks in KB (default = 32).
- `-s SIZE` Size of the extended ROM in MB (default: 64).
- `-d` Dump MIO0 blocks to files in mio0 directory.
- `-f` Fill old MIO0 blocks with 0x01.
- `-v` Verbose output.

Output file: If unspecified, it is constructed by replacing input file extension with .ext.z64
              
## Examples
64 MB extended ROM that is bit compatible with with generated from the M64ROMExtender1.3b, after extending to 64 MB
```console
sm64extend sm64.z64
```
               
24 MB extended ROM that is bit compatible with the ROM generated from the M64ROMExtender1.3b
```console
sm64extend -s 24 sm64.z64
```
                
Enable verbose messages and specify output filename:
```console
sm64extend -v sm64.z64 sm64_output.ext.z64
```
                 
Pad 64 KB between blocks, align blocks to 16-byte boundaries, fill old MIO0 blocks with 0x01:
```console
sm64extend -p 64 -a 16 -f sm64.z64
```
