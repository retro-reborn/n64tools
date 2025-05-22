# sm64geo
Standalone SM64 geometry layout decoder.

## Overview
sm64geo is a specialized tool for decoding geometry layout data from Super Mario 64. Geometry layouts define how 3D models and scenes are constructed in the game.

## Features
- Decodes binary geometry layout commands into readable text format
- Supports all known geometry layout commands used in SM64
- Analyzes and follows references to display lists
- Identifies standard object types and behaviors

## Usage
```console
sm64geo [options] INPUT_FILE [OUTPUT_FILE]
```

### Options
- `-o OFFSET`: Start offset in the input file (default: 0)
- `-l LENGTH`: Length of data to process (default: until end of file)
- `-v`: Enable verbose output
- `-a ADDRESS`: Base virtual address of the data (default: 0x80000000)

### Examples
Decode a geometry layout file:
```console
sm64geo mario_geo.bin mario_geo.txt
```

Decode geometry layout at specific offset:
```console
sm64geo -o 0x12345 level.bin level_geo.txt
```

## Output Format
The output is a text representation of the geometry layout commands. For example:

```
GEO_SCALE(0x00, 16384)
GEO_OPEN_NODE()
  GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, NULL)
  GEO_OPEN_NODE()
    GEO_ANIMATED_PART(LAYER_OPAQUE, 0, 0, 0, mario_butt_dl)
    GEO_OPEN_NODE()
      GEO_ANIMATED_PART(LAYER_OPAQUE, 68, 0, 0, mario_torso_dl)
    GEO_CLOSE_NODE()
  GEO_CLOSE_NODE()
GEO_CLOSE_NODE()
GEO_END()
```

## Related Tools
- `n64split`: Uses sm64geo functionality for extracting and rebuilding geometry layouts
- `f3d`: Can be used with sm64geo to disassemble the display lists referenced in geometry layouts
