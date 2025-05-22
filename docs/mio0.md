# mio0
Standalone MIO0 compressor/decompressor utility.

## Overview
mio0 is a command-line tool for compressing and decompressing data in the MIO0 format used by Nintendo 64 games. MIO0 (sometimes called MIO or Yay0) is a compression format developed by Nintendo that's commonly used in many first-party N64 titles.

## Features
- Compress arbitrary data into MIO0 format
- Decompress existing MIO0 data
- Support for all variants of MIO0 format found in Nintendo 64 games
- Options to control alignment and output format

## Usage
```console
mio0 [options] <command> INPUT_FILE [OUTPUT_FILE]
```

### Commands
- `d` Decompress MIO0 data
- `c` Compress data to MIO0 format

### Options
- `-a ALIGNMENT` Byte boundary to align compressed data (default = 1)
- `-o OFFSET` Start offset in the input file (default: 0)
- `-v` Enable verbose output

### Examples
Decompress MIO0 data:
```console
mio0 d compressed.bin decompressed.bin
```

Compress data to MIO0 format with 16-byte alignment:
```console
mio0 -a 16 c raw_data.bin compressed.mio0
```

## Notes
MIO0 compression is not as efficient as modern algorithms but offers a good balance of compression ratio and decompression speed for the N64 hardware.

## Related Tools
- `sm64extend`: Uses MIO0 functionality to decompress blocks in SM64 ROMs
- `sm64compress`: Uses MIO0 functionality to compress blocks in SM64 ROMs
