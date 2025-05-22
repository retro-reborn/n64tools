# mipsdisasm
Standalone recursive MIPS disassembler.

## Overview
mipsdisasm is a powerful recursive MIPS disassembler designed specifically for N64 ROMs. It can intelligently follow function calls to disassemble entire code trees and produce well-annotated assembly code.

## Features
- Recursive disassembly follows JAL/JALR instructions
- Intelligent macro instruction recognition (LUI/ADDIU pairs become LA, etc.)
- Proper label generation for jumps and branches
- Data section recognition
- Symbol table generation
- Support for all MIPS R4300i instructions used by N64

## Usage
```console
mipsdisasm [options] INPUT_FILE [OUTPUT_FILE]
```

### Options
- `-s START_ADDR`: Start address to begin disassembly (default: ROM entry point)
- `-e END_ADDR`: End address to stop disassembly
- `-b BASE_ADDR`: Base address to load ROM (default: 0x80000000)
- `-m`: Merge related instructions into pseudoinstructions
- `-v`: Enable verbose output
- `-p`: Generate procedure table for analysis

### Examples
Basic disassembly from entry point:
```console
mipsdisasm rom.z64 disassembly.s
```

Disassemble a specific function:
```console
mipsdisasm -s 0x80246A40 -e 0x80246B20 rom.z64 function.s
```

## Output Format
The output is a standard MIPS assembly file that can be reassembled with a MIPS assembler like GNU as. It includes:

- Labels for function entry points
- Labels for branch targets
- Comments indicating function calls
- Register usage hints
- Data sections with appropriate directives

## Related Tools
- `n64split`: Uses mipsdisasm functionality for code analysis
