# f3d
Tool to decode Fast3D display lists.

## Overview
f3d is a utility for decoding Fast3D display lists from Nintendo 64 games. It converts the binary format display lists into a human-readable format that shows all the rendering commands and their parameters.

## Features
- Decodes Fast3D display lists from binary data
- Supports all Fast3D commands used in Nintendo 64 games
- Outputs human-readable text showing the command sequence

## Usage
```console
f3d [options] INPUT_FILE [OUTPUT_FILE]
```

### Options
- `-o OFFSET` Start offset in the input file (default: 0)
- `-l LENGTH` Length of data to process (default: until end of file)
- `-v` Enable verbose output

### Example
```console
f3d mario_model.bin mario_model.f3d
```

## Output Format
The output file will contain a text representation of the display list commands, with one command per line. For example:

```
gsSPVertex(0x0400B1B0, 10, 0)
gsSP2Triangles(0, 1, 2, 0, 3, 4, 5, 0)
gsSP2Triangles(0, 2, 6, 0, 7, 8, 9, 0)
gsSPEndDisplayList()
```

## Related Tools
- `f3d2obj`: Converts F3D display lists to Wavefront OBJ files
