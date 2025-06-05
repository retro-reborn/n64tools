#!/usr/bin/env python3
"""
n64autosplit - Automatic n64split configuration generator

This tool analyzes N64 ROM files and automatically generates split configuration
files for use with n64split. It performs heuristic analysis to identify:
- ROM sections and their types
- Compressed data blocks (MIO0, BLAST, GZIP)
- Texture data and formats
- Assembly code sections
- Binary data sections
- Music and sound data

Usage: n64autosplit ROM_FILE [OUTPUT_CONFIG]
"""

import argparse
import os
import struct
import sys
from typing import Dict, List, Tuple, Optional, Any
from dataclasses import dataclass, field
from enum import Enum

class SectionType(Enum):
    """Types of ROM sections that can be detected"""
    HEADER = "header"
    ASM = "asm"
    BIN = "bin"
    MIO0 = "mio0"
    BLAST = "blast"
    GZIP = "gzip"
    M64 = "m64"
    SFX_CTL = "sfx.ctl"
    SFX_TBL = "sfx.tbl"
    INSTRUMENT_SET = "instrset"
    SM64_GEO = "geo"
    SM64_LEVEL = "level"
    SM64_BEHAVIOR = "behavior"
    PTR = "ptr"
    TEX_RGBA = "tex.rgba"
    TEX_CI = "tex.ci"
    TEX_I = "tex.i"
    TEX_IA = "tex.ia"
    TEX_SKYBOX = "tex.skybox"

@dataclass
class RomSection:
    """Represents a section of the ROM"""
    start: int
    end: int
    section_type: SectionType
    label: str = ""
    vaddr: int = 0
    confidence: float = 0.0
    metadata: Dict[str, Any] = field(default_factory=dict)

@dataclass
class RomAnalysis:
    """Results of ROM analysis"""
    rom_size: int
    rom_name: str
    basename: str
    checksum1: int
    checksum2: int
    sections: List[RomSection]
    labels: Dict[int, str]

class N64AutoSplit:
    """Main class for automatic N64 split configuration generation"""
    
    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.rom_data: bytes = b""
        self.rom_size: int = 0
        
        # Known patterns for different data types
        self.mio0_signature = b'MIO0'
        self.blast_signature = b'BLST'  # Blast Corps compression
        
        # MIPS instruction patterns for code detection
        self.common_mips_opcodes = {
            0x00,  # SPECIAL (R-type instructions)
            0x01,  # REGIMM (branch instructions)
            0x02,  # J
            0x03,  # JAL
            0x04,  # BEQ
            0x05,  # BNE
            0x06,  # BLEZ
            0x07,  # BGTZ
            0x08,  # ADDI
            0x09,  # ADDIU
            0x0A,  # SLTI
            0x0B,  # SLTIU
            0x0C,  # ANDI
            0x0D,  # ORI
            0x0E,  # XORI
            0x0F,  # LUI
            0x10,  # COP0 (Coprocessor 0 instructions)
            0x11,  # COP1 (FPU instructions)
            0x12,  # COP2 (RSP vector instructions)
            0x13,  # COP3
            0x14,  # BEQL (branch likely instructions)
            0x15,  # BNEL
            0x16,  # BLEZL
            0x17,  # BGTZL
            0x18,  # DADDI (64-bit add immediate)
            0x19,  # DADDIU
            0x1A,  # LDL
            0x1B,  # LDR
            0x1C,  # SPECIAL2
            0x1D,  # JALX
            0x1E,  # N64 special
            0x1F,  # SPECIAL3
            0x20,  # LB
            0x21,  # LH
            0x22,  # LWL
            0x23,  # LW
            0x24,  # LBU
            0x25,  # LHU
            0x26,  # LWR
            0x27,  # LWU
            0x28,  # SB
            0x29,  # SH
            0x2A,  # SWL
            0x2B,  # SW
            0x2C,  # SDL
            0x2D,  # SDR
            0x2E,  # SWR
            0x2F,  # CACHE
            0x30,  # LL
            0x31,  # LWC1 (load to FPU)
            0x32,  # LWC2 (load to RSP)
            0x33,  # LWC3
            0x34,  # LLD
            0x35,  # LDC1 (load double to FPU)
            0x36,  # LDC2
            0x37,  # LD
            0x38,  # SC
            0x39,  # SWC1 (store from FPU)
            0x3A,  # SWC2 (store from RSP)
            0x3B,  # SWC3
            0x3C,  # SCD
            0x3D,  # SDC1 (store double from FPU)
            0x3E,  # SDC2
            0x3F,  # SD
        }
    
    def log(self, message: str):
        """Print verbose logging message"""
        if self.verbose:
            print(f"[INFO] {message}")
    
    def read_u32_be(self, offset: int) -> int:
        """Read big-endian 32-bit value from ROM"""
        if offset + 4 > len(self.rom_data):
            return 0
        return struct.unpack(">I", self.rom_data[offset:offset+4])[0]
    
    def read_u16_be(self, offset: int) -> int:
        """Read big-endian 16-bit value from ROM"""
        if offset + 2 > len(self.rom_data):
            return 0
        return struct.unpack(">H", self.rom_data[offset:offset+2])[0]
    
    def detect_rom_format(self) -> str:
        """Detect and convert ROM format if necessary"""
        if len(self.rom_data) < 4:
            return "invalid"
        
        # Check for different ROM formats
        magic = self.rom_data[:4]
        
        if magic == b'\x80\x37\x12\x40':  # Z64 (big-endian)
            return "z64"
        elif magic == b'\x37\x80\x40\x12':  # V64 (byte-swapped)
            self.log("Converting V64 (byte-swapped) ROM to Z64 format")
            self.convert_v64_to_z64()
            return "v64"
        elif magic == b'\x40\x12\x37\x80':  # N64 (little-endian)
            self.log("Converting N64 (little-endian) ROM to Z64 format")
            self.convert_n64_to_z64()
            return "n64"
        else:
            return "unknown"
    
    def convert_v64_to_z64(self):
        """Convert V64 format to Z64 (swap every 2 bytes)"""
        data = bytearray(self.rom_data)
        for i in range(0, len(data) - 1, 2):
            data[i], data[i+1] = data[i+1], data[i]
        self.rom_data = bytes(data)
    
    def convert_n64_to_z64(self):
        """Convert N64 format to Z64 (reverse every 4 bytes)"""
        data = bytearray(self.rom_data)
        for i in range(0, len(data) - 3, 4):
            data[i:i+4] = data[i:i+4][::-1]
        self.rom_data = bytes(data)
    
    def extract_rom_info(self) -> Tuple[str, str, int, int]:
        """Extract basic ROM information from header"""
        # Game title (0x20-0x33, ASCII)
        title_bytes = self.rom_data[0x20:0x34]
        title = title_bytes.decode('ascii', errors='ignore').strip('\x00').strip()
        
        # Create basename from title
        basename = ''.join(c.lower() if c.isalnum() else '_' for c in title)
        basename = basename.strip('_')
        if not basename:
            basename = "unknown_rom"
        
        # Checksums (0x10-0x17)
        checksum1 = self.read_u32_be(0x10)
        checksum2 = self.read_u32_be(0x14)
        
        return title, basename, checksum1, checksum2
    
    def is_likely_mips_instruction(self, word: int) -> bool:
        """Check if a 32-bit word looks like a MIPS instruction"""
        opcode = (word >> 26) & 0x3F
        return opcode in self.common_mips_opcodes
    
    def analyze_code_section(self, start: int, length: int) -> float:
        """Analyze a section to determine if it contains MIPS code"""
        if length < 16:  # Too small
            return 0.0
        
        instruction_count = 0
        likely_instructions = 0
        
        for offset in range(start, min(start + length, self.rom_size - 4), 4):
            word = self.read_u32_be(offset)
            instruction_count += 1
            
            if self.is_likely_mips_instruction(word):
                likely_instructions += 1
            
            # Sample analysis - don't check every instruction for performance
            if instruction_count >= 100:
                break
        
        if instruction_count == 0:
            return 0.0
        
        confidence = likely_instructions / instruction_count
        return confidence
    
    def detect_mio0_blocks(self) -> List[RomSection]:
        """Detect MIO0 compressed blocks"""
        sections = []
        offset = 0
        
        while offset < self.rom_size - 16:
            if self.rom_data[offset:offset+4] == self.mio0_signature:
                # Found MIO0 signature
                uncompressed_size = self.read_u32_be(offset + 4)
                compressed_offset = self.read_u32_be(offset + 8)
                uncompressed_offset = self.read_u32_be(offset + 12)
                
                # Validate MIO0 header
                if (compressed_offset >= 16 and uncompressed_offset >= compressed_offset
                    and uncompressed_size > 0 and uncompressed_size < 0x1000000):
                    
                    # Estimate compressed size (this is approximate)
                    estimated_size = max(compressed_offset, uncompressed_offset) + (uncompressed_size // 8)
                    end_offset = offset + estimated_size
                    
                    # Make sure we don't go past ROM end
                    if end_offset > self.rom_size:
                        end_offset = self.rom_size
                    
                    section = RomSection(
                        start=offset,
                        end=end_offset,
                        section_type=SectionType.MIO0,
                        label=f"mio0_{offset:06X}",
                        confidence=0.9,
                        metadata={
                            'uncompressed_size': uncompressed_size,
                            'compressed_offset': compressed_offset,
                            'uncompressed_offset': uncompressed_offset
                        }
                    )
                    sections.append(section)
                    self.log(f"Found MIO0 block at 0x{offset:06X}-0x{end_offset:06X} (uncompressed: {uncompressed_size} bytes)")
                    
                    offset = end_offset
                    continue
            
            offset += 4
        
        return sections
    
    def detect_blast_blocks(self) -> List[RomSection]:
        """Detect Blast Corps compressed blocks"""
        sections = []
        offset = 0
        
        while offset < self.rom_size - 8:
            # Look for blast compression patterns
            # Blast Corps uses a different compression scheme
            word1 = self.read_u32_be(offset)
            word2 = self.read_u32_be(offset + 4)
            
            # Heuristic: blast blocks often start with specific patterns
            # This is game-specific and may need adjustment
            if ((word1 & 0xFF000000) == 0x00000000 and 
                (word2 & 0xFFFF0000) != 0x00000000 and
                word1 < 0x10000 and word2 < 0x100000):
                
                # Try to determine block size heuristically
                block_size = 0x1000  # Default size, will be refined
                
                # Look for patterns that might indicate end of block
                for test_offset in range(offset + 0x100, min(offset + 0x10000, self.rom_size), 0x10):
                    test_word = self.read_u32_be(test_offset)
                    if test_word == 0 or test_word == 0xFFFFFFFF:
                        # Potential end marker
                        block_size = test_offset - offset
                        break
                
                section = RomSection(
                    start=offset,
                    end=offset + block_size,
                    section_type=SectionType.BLAST,
                    label=f"blast_{offset:06X}",
                    confidence=0.6,  # Lower confidence for heuristic detection
                    metadata={'detected_size': block_size}
                )
                sections.append(section)
                self.log(f"Potential BLAST block at 0x{offset:06X}-0x{offset + block_size:06X}")
                
                offset += block_size
                continue
            
            offset += 4
        
        return sections
    
    def detect_texture_data(self, start: int, length: int) -> Optional[Dict]:
        """Analyze data to determine if it contains texture information"""
        if length < 64:  # Too small for meaningful texture
            return None
        
        # Sample some data to check for texture patterns
        sample_size = min(1024, length)
        sample_data = self.rom_data[start:start + sample_size]
        
        # Check for common texture dimensions (power of 2)
        common_dimensions = [8, 16, 32, 64, 128, 256]
        
        # Look for RGBA16 patterns (2 bytes per pixel)
        for width in common_dimensions:
            for height in common_dimensions:
                expected_size = width * height * 2  # RGBA16
                if abs(length - expected_size) < 16:
                    return {
                        'format': 'rgba',
                        'depth': 16,
                        'width': width,
                        'height': height
                    }
        
        # Look for CI8 patterns (1 byte per pixel + palette)
        for width in common_dimensions:
            for height in common_dimensions:
                expected_size = width * height + 512  # CI8 + 256 color palette
                if abs(length - expected_size) < 64:
                    return {
                        'format': 'ci',
                        'depth': 8,
                        'width': width,
                        'height': height
                    }
        
        return None
    
    def detect_music_data(self, start: int, length: int) -> float:
        """Detect if a section contains music/audio data"""
        if length < 64:
            return 0.0
        
        # Look for M64 music sequence patterns
        # M64 files often have specific header patterns
        header = self.rom_data[start:start + 16]
        
        # Check for common M64 patterns
        confidence = 0.0
        
        # Look for sequence data patterns
        if len(header) >= 8:
            word1 = struct.unpack(">I", header[0:4])[0]
            word2 = struct.unpack(">I", header[4:8])[0]
            
            # Heuristics for music data
            if word1 < 0x1000 and word2 < 0x10000:
                confidence += 0.3
            
            # Check for instrument patterns
            if header[0] in [0x00, 0x01, 0x02] and header[1] < 0x10:
                confidence += 0.2
        
        return confidence
    
    def analyze_data_section(self, start: int, length: int) -> SectionType:
        """Analyze a data section to determine its most likely type"""
        if length < 16:
            return SectionType.BIN
        
        # Check for code
        code_confidence = self.analyze_code_section(start, length)
        if code_confidence > 0.7:
            return SectionType.ASM
        
        # Check for music
        music_confidence = self.detect_music_data(start, length)
        if music_confidence > 0.5:
            return SectionType.M64
        
        # Default to binary data
        return SectionType.BIN
    
    def find_function_boundaries(self) -> List[Tuple[int, int]]:
        """Find potential function boundaries in the ROM"""
        functions = []
        current_start = None
        
        for offset in range(0x1000, self.rom_size - 4, 4):  # Skip header
            word = self.read_u32_be(offset)
            
            # Look for function prologue patterns
            if self.is_likely_mips_instruction(word):
                if current_start is None:
                    current_start = offset
            else:
                if current_start is not None:
                    length = offset - current_start
                    if length >= 64:  # Minimum function size
                        functions.append((current_start, offset))
                    current_start = None
        
        # Handle function at end of ROM
        if current_start is not None:
            functions.append((current_start, self.rom_size))
        
        return functions
    
    def create_sections(self, compressed_blocks: List[RomSection]) -> List[RomSection]:
        """Create sections for the entire ROM, filling gaps between compressed blocks"""
        sections = []
        
        # Start with header
        sections.append(RomSection(
            start=0x00,
            end=0x40,
            section_type=SectionType.HEADER,
            label="header",
            confidence=1.0
        ))
        
        # Sort compressed blocks by start address
        compressed_blocks.sort(key=lambda x: x.start)
        
        current_offset = 0x40  # After header
        
        for block in compressed_blocks:
            # Fill gap before this block
            if current_offset < block.start:
                gap_length = block.start - current_offset
                section_type = self.analyze_data_section(current_offset, gap_length)
                
                label = ""
                if section_type == SectionType.ASM:
                    label = f"code_{current_offset:06X}"
                elif section_type == SectionType.M64:
                    label = f"music_{current_offset:06X}"
                else:
                    label = f"data_{current_offset:06X}"
                
                sections.append(RomSection(
                    start=current_offset,
                    end=block.start,
                    section_type=section_type,
                    label=label,
                    confidence=0.5
                ))
            
            # Add the compressed block
            sections.append(block)
            current_offset = block.end
        
        # Fill final gap
        if current_offset < self.rom_size:
            gap_length = self.rom_size - current_offset
            section_type = self.analyze_data_section(current_offset, gap_length)
            
            label = ""
            if section_type == SectionType.ASM:
                label = f"code_{current_offset:06X}"
            elif section_type == SectionType.M64:
                label = f"music_{current_offset:06X}"
            else:
                label = f"data_{current_offset:06X}"
                
            sections.append(RomSection(
                start=current_offset,
                end=self.rom_size,
                section_type=section_type,
                label=label,
                confidence=0.5
            ))
        
        return sections
    
    def analyze_rom(self, rom_path: str) -> RomAnalysis:
        """Perform complete ROM analysis"""
        self.log(f"Loading ROM: {rom_path}")
        
        # Read ROM file
        with open(rom_path, 'rb') as f:
            self.rom_data = f.read()
        self.rom_size = len(self.rom_data)
        
        if self.rom_size < 0x1000:
            raise ValueError("File too small to be a valid N64 ROM")
        
        # Detect and convert ROM format
        rom_format = self.detect_rom_format()
        if rom_format == "unknown":
            raise ValueError("Unknown ROM format - not a valid N64 ROM")
        
        self.log(f"ROM size: {self.rom_size:,} bytes ({self.rom_size / (1024*1024):.1f} MB)")
        
        # Extract basic ROM information
        title, basename, checksum1, checksum2 = self.extract_rom_info()
        self.log(f"ROM title: {title}")
        self.log(f"Checksums: 0x{checksum1:08X}, 0x{checksum2:08X}")
        
        # Detect compressed blocks
        mio0_blocks = self.detect_mio0_blocks()
        blast_blocks = self.detect_blast_blocks()
        
        all_compressed = mio0_blocks + blast_blocks
        self.log(f"Found {len(mio0_blocks)} MIO0 blocks and {len(blast_blocks)} BLAST blocks")
        
        # Create sections for entire ROM
        sections = self.create_sections(all_compressed)
        
        self.log(f"Created {len(sections)} sections total")
        
        # Extract labels (for now, just basic ones)
        labels = {}
        
        return RomAnalysis(
            rom_size=self.rom_size,
            rom_name=title,
            basename=basename,
            checksum1=checksum1,
            checksum2=checksum2,
            sections=sections,
            labels=labels
        )
    
    def generate_config(self, analysis: RomAnalysis) -> str:
        """Generate YAML configuration from analysis"""
        lines = []
        
        # Header comments
        lines.append("# ROM splitter configuration file")
        lines.append("# Auto-generated by n64autosplit")
        lines.append(f'name: "{analysis.rom_name}"')
        lines.append("")
        
        # Checksums
        lines.append("# checksums from ROM header offsets 0x10 and 0x14")
        lines.append("# used for auto configuration detection")
        lines.append(f"checksum1: 0x{analysis.checksum1:08X}")
        lines.append(f"checksum2: 0x{analysis.checksum2:08X}")
        lines.append("")
        
        # Base filename
        lines.append("# base filename used for outputs (please, no spaces)")
        lines.append(f'basename: "{analysis.basename}"')
        lines.append("")
        
        # Section type descriptions
        lines.append("# ranges to split the ROM into")
        lines.append("# types:")
        lines.append("#   asm      - MIPS assembly block")
        lines.append("#   bin      - raw binary, usually data")
        lines.append("#   blast    - Blast Corps compressed blocks")
        lines.append("#   gzip     - gzip compressed blocks")
        lines.append("#   header   - ROM header block")
        lines.append("#   m64      - M64 music sequence bank")
        lines.append("#   mio0     - MIO0 compressed data block")
        lines.append("#   ptr      - RAM address or ROM offset pointer")
        lines.append("#")
        lines.append("#   texture types:")
        lines.append("#      rgba   - 32/16 bit RGBA")
        lines.append("#      ci     - color index with palette")
        lines.append("#      i      - intensity (grayscale)")
        lines.append("#      ia     - intensity + alpha")
        lines.append("ranges:")
        
        # Generate sections
        for section in analysis.sections:
            if section.section_type == SectionType.HEADER:
                lines.append(f'   - [0x{section.start:06X}, 0x{section.end:06X}, "{section.section_type.value}", "{section.label}"]')
            elif section.section_type == SectionType.ASM and section.vaddr > 0:
                lines.append(f'   - [0x{section.start:06X}, 0x{section.end:06X}, "{section.section_type.value}", "{section.label}", 0x{section.vaddr:08X}]')
            elif section.section_type in [SectionType.MIO0, SectionType.BLAST]:
                lines.append(f'   - [0x{section.start:06X}, 0x{section.end:06X}, "{section.section_type.value}", "{section.label}"]')
                # Add potential texture sub-sections for compressed blocks
                if 'uncompressed_size' in section.metadata:
                    lines.append("      # Add texture definitions here after manual analysis")
                    lines.append("      # - [0x0, \"tex.rgba\", 16, 32, 32]")
            else:
                lines.append(f'   - [0x{section.start:06X}, 0x{section.end:06X}, "{section.section_type.value}"]')
        
        # Labels section
        if analysis.labels:
            lines.append("")
            lines.append("labels:")
            for addr, label in sorted(analysis.labels.items()):
                lines.append(f"   0x{addr:08X}: {label}")
        
        return '\n'.join(lines)

def main():
    parser = argparse.ArgumentParser(
        description="Generate n64split configuration files automatically",
        epilog="This tool performs heuristic analysis and may require manual refinement."
    )
    parser.add_argument("rom_file", help="Input N64 ROM file")
    parser.add_argument("output_config", nargs='?', help="Output YAML config file (default: auto-generated)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output")
    parser.add_argument("--force", action="store_true", help="Overwrite existing output file")
    
    args = parser.parse_args()
    
    # Validate input file
    if not os.path.isfile(args.rom_file):
        print(f"Error: ROM file '{args.rom_file}' not found", file=sys.stderr)
        return 1
    
    # Generate output filename if not provided
    if not args.output_config:
        base_name = os.path.splitext(os.path.basename(args.rom_file))[0]
        args.output_config = f"{base_name}.yaml"
    
    # Check if output file exists
    if os.path.exists(args.output_config) and not args.force:
        print(f"Error: Output file '{args.output_config}' already exists. Use --force to overwrite.", file=sys.stderr)
        return 1
    
    try:
        # Analyze ROM
        analyzer = N64AutoSplit(verbose=args.verbose)
        analysis = analyzer.analyze_rom(args.rom_file)
        
        # Generate configuration
        config_text = analyzer.generate_config(analysis)
        
        # Write output
        with open(args.output_config, 'w') as f:
            f.write(config_text)
        
        print(f"Generated configuration file: {args.output_config}")
        print(f"ROM: {analysis.rom_name}")
        print(f"Sections: {len(analysis.sections)}")
        print(f"Checksums: 0x{analysis.checksum1:08X}, 0x{analysis.checksum2:08X}")
        
        if args.verbose:
            print("\nSection breakdown:")
            for section in analysis.sections:
                size = section.end - section.start
                confidence_str = f" (confidence: {section.confidence:.1f})" if section.confidence < 1.0 else ""
                print(f"  0x{section.start:06X}-0x{section.end:06X}: {section.section_type.value} ({size:,} bytes){confidence_str}")
        
        print("\nNote: This is an automatic analysis. Manual review and adjustment may be needed.")
        print("Especially check compressed block contents and texture format definitions.")
        
        return 0
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    sys.exit(main())
