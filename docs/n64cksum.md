# n64cksum
Standalone N64 checksum generator.

## Overview
n64cksum is a simple utility for calculating and updating the checksums in Nintendo 64 ROM files. The N64 ROM header contains two checksums that are used by the console to verify the integrity of the ROM.

## Features
- Calculate correct N64 ROM header checksums
- Update checksums in-place or output to a new file
- Support for all N64 ROM formats (Z64, V64, N64)

## Usage
```console
n64cksum INPUT_FILE [OUTPUT_FILE]
```

If `OUTPUT_FILE` is not specified, checksums will be updated in-place in the `INPUT_FILE`.

### Examples
Update checksums in-place:
```console
n64cksum modified_rom.z64
```

Calculate checksums and save to a new file:
```console
n64cksum modified_rom.z64 fixed_rom.z64
```

## Technical Details
The N64 ROM header contains two 32-bit checksums at offsets 0x10 and 0x14:
1. The first checksum (CRC1) at 0x10 is calculated over the entire ROM excluding the header
2. The second checksum (CRC2) at 0x14 is calculated over only parts of the ROM

This tool calculates both checksums according to the official Nintendo algorithm and updates them in the ROM header.

## Related Tools
- `n64split`: Uses n64cksum functionality to ensure valid ROMs after splitting
