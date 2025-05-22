# sm64compress
Experimental Super Mario 64 ROM alignment and compression tool

sm64compress is designed to optimize Super Mario 64 ROMs by repacking MIO0 blocks and reducing unused space.

## Features
- Packs all MIO0 blocks together, reducing unused space
- Optionally compresses MIO0 blocks (and converts 0x17 commands to 0x18)
- Configurable MIO0 block alignment (default 16 byte)
- Reduces output ROM size to 4 MB boundary
- Updates assembly reference to MIO0 blocks
- Recalculates ROM header checksums

## Usage
```console
sm64compress [-a ALIGNMENT] [-c] [-d] [-v] FILE [OUT_FILE]
```

### Options
- `-a alignment` Byte boundary to align MIO0 blocks (default = 16).
- `-c` Compress all blocks using MIO0.
- `-d` Dump MIO0 blocks to files in mio0 directory.
- `-v` Verbose output.

Output file: If unspecified, it is constructed by replacing input file extension with .out.z64
