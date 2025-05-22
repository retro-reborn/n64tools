# bootinjector64
Standalone MIPS Assembly Injector for Nintendo 64 Boot Code.

## Overview
bootinjector64 is a tool for injecting custom MIPS assembly code into the boot section of Nintendo 64 ROMs. It allows developers to modify the boot code of N64 games, enabling custom boot sequences, patches, and other modifications.

## Features
- Inject custom MIPS assembly into the boot section of N64 ROMs
- Automatically recompile the assembly code
- Update ROM checksums after modification
- Handle different ROM formats (byte-swapped, big-endian, little-endian)
- Safety checks to prevent oversized boot code injection

## Usage
```console
bootinjector64 [options] ROM ASM_FILE [ROM_OUT]
```

### File Arguments
- `ROM` Input ROM file
- `ASM_FILE` Input MIPS assembly file to inject
- `ROM_OUT` Output ROM file (default: overwrites input ROM)

### Options
- `-a ASSEMBLER` Path to assembler (default: bass)
- `-f` Force overwrite even if boot code is too large
- `-v` Enable verbose output
- `-h` Show help message

### Examples
Inject custom boot code:
```console
bootinjector64 original.z64 custom_boot.asm modified.z64
```

Use custom assembler with verbose output:
```console
bootinjector64 -v -a /path/to/assembler original.z64 custom_boot.asm
```

## Assembly Format
Your assembly file should follow the bass assembler syntax and should be aware of the N64 boot section structure:

```asm
.n64
.create "bootcode.bin", 0x0

// N64 header is handled automatically by the tool
// Your custom code starts here

start:
    li    t0, 100      // Load immediate value 100 into t0
loop:
    addi  t0, t0, -1   // Decrement t0
    bnez  t0, loop     // Branch to loop if t0 is not zero
    nop                // Branch delay slot

    // Continue with regular boot process
    // ...
```

## Technical Notes
- The maximum size of the boot section is 4032 bytes (0xFC0)
- The tool automatically handles byte swapping and checksum recalculation
- Be careful when modifying the boot code, as incorrect code may render the ROM unbootable
- This tool is intended for advanced users who understand N64 MIPS assembly and the boot process

## Requirements
- [bass](https://github.com/ARM9/bass) assembler or another compatible MIPS assembler

## Related Tools
- `n64cksum`: Used internally to update ROM checksums
- `mipsdisasm`: Can be used to disassemble existing boot code
