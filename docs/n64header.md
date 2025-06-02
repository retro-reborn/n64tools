# n64header

## Overview
`n64header` is a comprehensive utility for viewing N64 ROM header information in a readable format. It follows the official [N64brew.dev ROM Header specification](https://n64brew.dev/wiki/ROM_Header) and supports both standard commercial ROMs and Advanced Homebrew ROM Header format.

## Usage
```bash
n64header [OPTIONS] FILE

# View help
n64header -h

# Analyze a ROM
n64header game.z64

# Verbose output
n64header -v game.z64
```

Command line options:
```
Usage: n64header [OPTIONS] FILE

n64header v1.0: N64 ROM header viewer

Optional arguments:
  -h, --help     Show this help message and exit
  -V, --version  Show version information and exit
  -v, --verbose  Enable verbose output

Arguments:
  FILE           N64 ROM file to analyze
```

## Technical Details
The N64 ROM header is a 64-byte structure at the beginning of every N64 ROM that contains various information about the ROM. This implementation follows the official [N64brew.dev ROM Header specification](https://n64brew.dev/wiki/ROM_Header):

### Standard Header Fields
* 0x00:      Reserved byte (usually 0x80 for commercial games)
* 0x01-0x03: PI BSD DOM1 Configuration Flags (usually 0x371240)
* 0x04-0x07: Clock Rate
* 0x08-0x0B: Boot Address (initial PC in RDRAM)
* 0x0C-0x0F: Libultra Version
* 0x10-0x17: Check Code (64-bit integrity check, not CRC)
* 0x18-0x1F: Reserved fields
* 0x20-0x33: Game Title (20 characters, ASCII or JIS X 0201)
* 0x34-0x3A: Reserved (or homebrew controller fields if Advanced Homebrew Header)
* 0x3B-0x3E: Game Code (4 characters: Category + Unique + Destination)
* 0x3F:      ROM Version

### Game Code Breakdown
The 4-character Game Code is split into three parts:
* **Category Code** (1 char): N=Game Pak, D=64DD Disk, C=Expandable Game, etc.
* **Unique Code** (2 chars): Game identifier (e.g., "SM" for Super Mario 64)
* **Destination Code** (1 char): Region/country (E=North America, J=Japan, P=Europe, etc.)

### Supported Destination Codes
The tool recognizes the following destination/region codes:
* **A** - All regions
* **B** - Brazil
* **C** - China
* **D** - Germany
* **E** - North America
* **F** - France
* **G** - Gateway 64 (NTSC)
* **H** - Netherlands
* **I** - Italy
* **J** - Japan
* **K** - Korea
* **L** - Gateway 64 (PAL)
* **N** - Canada
* **P** - Europe
* **S** - Spain
* **U** - Australia
* **W** - Scandinavia
* **X, Y, Z** - Europe
* **7** - Beta (non-standard)
* **\0** - Region Free
* Other codes are marked as "Unknown"

### Libultra Version Decoding
Common libultra SDK versions are automatically decoded:
* 0x0000144B - libultra 2.0K
* 0x0000144C - libultra 2.0L
* 0x0000144D - libultra 2.0D
* 0x00001446 - libultra 2.0F
* And others...

### Advanced Homebrew ROM Header
When bytes 0x3C-0x3D contain "ED", the ROM uses the Advanced Homebrew ROM Header format with additional fields:
* 0x34-0x37: Controller specifications for ports 1-4
* 0x38-0x3B: Homebrew-specific flags
* 0x3F:      Savetype specification (bitfield)

This tool supports multiple N64 ROM formats:
* Z64: Big-endian (ABCD) - the native N64 format
* V64: Byte-swapped (BADC) - used by some N64 emulators
* N64: Little-endian (DCBA) - Intel byte order

## Example Output
```
N64 ROM Header Information
==========================
Format:                   Z64 (big-endian/ABCD)

Standard Header Fields:
  Reserved byte:            0x80
  PI BSD DOM1 config:       0x371240
  Clock rate:               0x0000000F
  Boot address:             0x80125C00
  Libultra version:         0x0000144B (2.0K)

Security:
  Check code:               0x65EEE53AED7D733C

Reserved Fields:
  Reserved 1:               0x00000000
  Reserved 2:               0x00000000

Game Information:
  Game title:               "PAPER MARIO         "
  Game code:                NMQE
    Category code:          N (Game Pak)
    Unique code:            MQ
    Destination code:       E (North America)
  ROM version:              0x00 (0)

Reserved/Other Fields:
  Reserved 3:               00000000000000
```

### Advanced Homebrew ROM Header Example
When a homebrew ROM is detected (Game ID "ED"), additional information is displayed:
```
Advanced Homebrew ROM Header:
  Game ID:                  ED (Homebrew format detected)
  Controller 1:             0x01 (Standard N64 Controller)
  Controller 2:             0x00 (None)
  Controller 3:             0x00 (None)
  Controller 4:             0x00 (None)
  Homebrew flags:           0x00000000
  Savetype:                 0x40 (4K EEPROM)
```

## Features
- Automatic ROM format detection and conversion (Z64/V64/N64)
- Complete ROM header analysis following official specification
- Advanced Homebrew ROM Header support
- Detailed game code breakdown
- Libultra version decoding
- Comprehensive region/destination code mapping
- Clean, organized output format
