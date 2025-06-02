# n64graphics
Converts graphics data between PNG files and N64 graphics formats.

## Overview
n64graphics is a tool for converting between standard PNG image files and the various graphics formats used by Nintendo 64 games. It handles both encoding (PNG to N64 format) and decoding (N64 format to PNG).

## Features
- Convert between PNG and various N64 graphics formats:
  - RGBA16: 16-bit color (5-5-5-1 bits for R-G-B-A)
  - RGBA32: 32-bit color (8-8-8-8 bits for R-G-B-A)
  - IA16: 16-bit intensity + alpha (8-8 bits)
  - IA8: 8-bit intensity + alpha (4-4 bits)
  - IA4: 4-bit intensity + alpha (3-1 bits)
  - IA1: 1-bit intensity + alpha
  - I8: 8-bit intensity
  - I4: 4-bit intensity
  - CI8: 8-bit color indexed
  - CI4: 4-bit color indexed
- Support for custom palettes
- Batch processing capability

## Usage
```console
n64graphics <command> [options] FILE
```

### Commands
- `decode`: Convert N64 graphics data to PNG
- `encode`: Convert PNG to N64 graphics data

### Options for Decode
- `-f FORMAT`: Graphics format (rgba16, rgba32, ia16, ia8, ia4, ia1, i8, i4, ci8, ci4)
- `-W WIDTH`: Image width in pixels
- `-H HEIGHT`: Image height in pixels
- `-o OFFSET`: Start offset in the input file (default: 0)
- `-p PALETTE`: Palette file for CI formats

### Options for Encode
- `-f FORMAT`: Graphics format (rgba16, rgba32, ia16, ia8, ia4, ia1, i8, i4, ci8, ci4)
- `-o OUTPUT`: Output file

### Examples
Decode RGBA16 texture to PNG:
```console
n64graphics decode -f rgba16 -w 64 -h 64 texture.bin texture.png
```

Encode PNG to IA8 format:
```console
n64graphics encode -f ia8 texture.png texture.bin
```

## Related Tools
- `n64split`: Uses n64graphics functionality to extract textures from ROMs
