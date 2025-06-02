# n64convert
N64 ROM format converter utility.

## Overview
n64convert is a command-line tool for converting Nintendo 64 ROM files between different byte order formats. N64 ROMs can exist in three different formats depending on how the bytes are ordered, and this tool allows conversion between any of these formats.

## Features
- Convert between all N64 ROM formats:
  - **Z64** (big-endian/ABCD) - Native N64 format
  - **V64** (byte-swapped/BADC) - Common in some emulators
  - **N64** (little-endian/DCBA) - Used by some ROM dumpers
- Automatic format detection based on ROM header magic bytes
- Auto-generated output filenames with appropriate extensions
- Force overwrite protection for existing files
- Verbose output mode for detailed conversion information

## Usage
```console
n64convert [OPTIONS] INPUT
```

### Options
- `-f FORMAT, --format FORMAT`: Target format (default: z64)
  - `z64`, `big`, `abcd`: Z64 format (big-endian)
  - `v64`, `byte`, `badc`: V64 format (byte-swapped)
  - `n64`, `little`, `dcba`: N64 format (little-endian)
- `-o FILE, --output FILE`: Output file (default: auto-generated based on format)
- `-F, --force`: Force overwrite existing output file
- `-v, --verbose`: Enable verbose output
- `-h, --help`: Show help message
- `-V, --version`: Show version information

### Arguments
- `INPUT`: Input N64 ROM file

## Examples

Convert a ROM to Z64 format (default):
```console
n64convert rom.v64
```

Convert to V64 format with specific output filename:
```console
n64convert -f v64 -o converted.v64 original.z64
```

Convert to N64 format with verbose output:
```console
n64convert -f n64 -v original.z64
```

Force overwrite an existing file:
```console
n64convert -f z64 -F rom.n64 rom.z64
```

## Technical Details
The tool detects the input ROM format by examining the magic bytes in the first 4 bytes of the file:
- Performs format-specific byte swapping operations to convert between formats
- Maintains ROM integrity by preserving all data while only changing byte order
- Supports ROMs of any size (minimum 64 bytes for valid N64 ROM detection)

## Output File Naming
When no output filename is specified with `-o`, the tool automatically generates one based on:
- Input filename with extension changed to match target format
- Z64 format -> `.z64` extension
- V64 format -> `.v64` extension  
- N64 format -> `.n64` extension

## Error Handling
The tool will exit with an error if:
- Input file cannot be read or is too small (< 64 bytes)
- ROM format cannot be detected (invalid magic bytes)
- Output file already exists (unless `-F` flag is used)
- File write operations fail

## Related Tools
- `n64cksum`: Update ROM checksums after format conversion
- `n64header`: Display ROM header information regardless of format
- `n64split`: Works with Z64 format ROMs for game disassembly
