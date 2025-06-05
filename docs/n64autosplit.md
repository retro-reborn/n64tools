# n64autosplit
Automatic n64split Configuration Generator

n64autosplit is a Python script that analyzes N64 ROM files and automatically generates split configuration files for use with `n64split`. It performs heuristic analysis to intelligently identify different types of data sections within a ROM, making it easier to create initial configurations for ROM splitting projects.

## Overview
Creating configuration files for `n64split` manually can be time-consuming and error-prone, especially for unfamiliar ROMs. n64autosplit automates this process by scanning through the ROM data and detecting patterns that indicate different types of content such as code, compressed data, textures, and audio.

## Features
- **Automatic ROM format detection and conversion** (Z64, V64, N64 formats)
- **Intelligent section detection** using heuristic analysis:
  - ROM header information
  - Assembly code sections (MIPS instruction patterns)
  - Compressed data blocks (MIO0, BLAST, GZIP)
  - Texture data and formats (RGBA, CI, I, IA, Skybox)
  - Binary data sections
  - Music and sound data (M64, SFX)
  - Special format data (SM64 geo, level, behavior)
  - Pointer tables
- **Confidence scoring** for detected sections
- **YAML configuration output** compatible with n64split
- **Verbose analysis reporting**

## Detected Section Types
n64autosplit can identify the following types of ROM sections:
| Section Type |             Description             |
|--------------|-------------------------------------|
| `header`     | ROM header information              |
| `asm`        | Assembly code sections              |
| `bin`        | Raw binary data                     |
| `mio0`       | MIO0 compressed data blocks         |
| `blast`      | BLAST compressed data (Blast Corps) |
| `gzip`       | GZIP compressed data                |
| `m64`        | Music data files                    |
| `sfx.ctl`    | Sound effect control data           |
| `sfx.tbl`    | Sound effect table data             |
| `instrset`   | Instrument set data                 |
| `geo`        | SM64 geometry layout data           |
| `level`      | SM64 level data                     |
| `behavior`   | SM64 behavior script data           |
| `ptr`        | Pointer table                       |
| `tex.rgba`   | RGBA texture data                   |
| `tex.ci`     | Color-indexed texture data          |
| `tex.i`      | Intensity texture data              |
| `tex.ia`     | Intensity+Alpha texture data        |
| `tex.skybox` | Skybox texture data                 |


## Usage
```console
uv run scripts/n64autosplit.py ROM_FILE [OUTPUT_CONFIG] [OPTIONS]
```

### Arguments
- `ROM_FILE` - Input N64 ROM file to analyze
- `OUTPUT_CONFIG` - Output YAML configuration file (optional, defaults to `{rom_name}.yaml`)

### Options
- `-v, --verbose` - Enable verbose output showing detailed analysis
- `--force` - Overwrite existing output file if it exists
- `-h, --help` - Show help message

## Examples

### Basic usage
```console
# Analyze a ROM and generate configuration
uv run scripts/n64autosplit.py game.z64

# This creates game.yaml with the detected sections
```

### Specify output file
```console
# Generate configuration with custom filename
uv run scripts/n64autosplit.py mario.z64 mario_config.yaml
```

### Verbose analysis
```console
# Get detailed information about the analysis process
uv run scripts/n64autosplit.py game.z64 --verbose
```

### Force overwrite
```console
# Overwrite existing configuration file
uv run scripts/n64autosplit.py game.z64 config.yaml --force
```

Each section entry contains:
- Start address (hex)
- End address (hex) 
- Section type
- Optional label and metadata

## Analysis Process
n64autosplit uses several heuristic techniques:

1. **ROM Format Detection**: Automatically detects and converts between Z64, V64, and N64 ROM formats
2. **Header Analysis**: Extracts ROM name, checksums, and entry point from the header
3. **Pattern Matching**: Looks for known signatures (MIO0, BLAST compression headers)
4. **Instruction Analysis**: Identifies MIPS assembly code by analyzing instruction patterns and opcodes
5. **Data Structure Recognition**: Detects common N64 data structures and formats
6. **Confidence Scoring**: Assigns confidence levels to detected sections based on pattern strength

## Limitations and Notes
- **Heuristic Analysis**: Results are based on pattern recognition and may not be 100% accurate
- **Manual Review Required**: Generated configurations should be manually reviewed and refined
- **Game-Specific Knowledge**: Some sections may require game-specific knowledge to properly categorize
- **Compressed Data**: Contents of compressed blocks cannot be analyzed until decompression
- **Texture Formats**: Texture format detection may need manual verification

## Best Practices
1. **Always review output**: Manually check the generated configuration before using with n64split
2. **Start with known games**: Test with well-documented ROMs first to verify accuracy
3. **Use verbose mode**: Enable verbose output to understand the analysis decisions
4. **Iterative refinement**: Use the generated config as a starting point and refine based on n64split results
5. **Backup originals**: Keep backups of both ROM files and generated configurations

## Integration with n64split
After generating a configuration with n64autosplit, use it with n64split:

```console
# Generate configuration
uv run scripts/n64autosplit.py game.z64

# Use with n64split
./bin/n64split -c game.yaml game.z64
```

The generated configuration provides a solid foundation for ROM splitting projects, though manual adjustment may be needed for optimal results.

## Troubleshooting
- **"ROM file not found"**: Check that the ROM file path is correct
- **"Output file exists"**: Use `--force` to overwrite or choose a different output name
- **Poor detection quality**: Try different ROM formats or dumps, some may analyze better than others
- **Missing sections**: The tool may miss some sections - manual addition to the config may be needed

## See Also
- [`n64split`](n64split.md) - ROM splitting tool that uses the generated configurations
- [`n64graphics`](n64graphics.md) - Graphics format conversion tool
- [`mio0`](mio0.md) - MIO0 compression/decompression tool
