#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse.h"
#include "libn64.h"
#include "utils.h"

#define SM64WALK_VERSION "0.1"

typedef struct {
  char *rom_file;
  unsigned offset;
  char region;
  bool verbose;
} arg_config;

static arg_config default_config = {
    NULL,       // ROM filename
    0xFFFFFFFF, // offset (default: auto-detect)
    0,          // region (default: auto-detect)
    false,      // verbose
};

// Function to parse command line arguments
static int parse_arguments(int argc, char *argv[], arg_config *config) {
  arg_parser *parser;
  int result;
  const char *regions[] = {"Europe", "US", "JP", "Shindou"};
  char *region_str = NULL;

  // Initialize the argument parser
  parser = argparse_init("sm64walk", SM64WALK_VERSION,
                         "Super Mario 64 script walker");
  if (parser == NULL) {
    ERROR("Error: Failed to initialize argument parser\n");
    return -1;
  }

  // Add the offset option
  argparse_add_flag(
      parser, 'o', "offset", ARG_TYPE_UINT,
      "Start decoding level scripts at OFFSET (default: auto-detect)", "OFFSET",
      &config->offset, false, NULL, 0);

  // Add region option with validation against the regions array
  argparse_add_flag(parser, 'r', "region", ARG_TYPE_STRING,
                    "Region to use. Valid: Europe, US, JP, Shindou", "REGION",
                    &region_str, false, regions, 4);

  // Add the verbose flag
  argparse_add_flag(parser, 'v', "verbose", ARG_TYPE_NONE,
                    "Enable verbose output", NULL, &config->verbose, false,
                    NULL, 0);

  // Add positional argument for ROM file
  argparse_add_positional(parser, "FILE", "Input ROM file", ARG_TYPE_STRING,
                          &config->rom_file, true);

  // Parse the arguments
  result = argparse_parse(parser, argc, argv);

  // Set verbosity global flag
  if (config->verbose) {
    g_verbosity = 1;
  }

  // Map the region string to the corresponding character
  if (region_str != NULL) {
    if (strcmp(region_str, "Europe") == 0) {
      config->region = 'E';
    } else if (strcmp(region_str, "US") == 0) {
      config->region = 'U';
    } else if (strcmp(region_str, "JP") == 0) {
      config->region = 'J';
    } else if (strcmp(region_str, "Shindou") == 0) {
      config->region = 'S';
    }
    free(region_str);
  }

  // Free the parser
  argparse_free(parser);

  return result;
}

typedef struct {
  unsigned start;
  unsigned end;
} level_t;

static void add_level(level_t levels[], unsigned *lcount, unsigned start,
                      unsigned end) {
  unsigned i;
  INFO("Adding level %06X - %06X\n", start, end);
  for (i = 0; i < *lcount; i++) {
    if (levels[i].start == start) {
      return;
    }
  }
  levels[*lcount].start = start;
  levels[*lcount].end = end;
  (*lcount)++;
}

static void decode_level(unsigned char *data, level_t levels[], unsigned int l,
                         unsigned *lcount) {
  unsigned int ptr_start;
  unsigned int ptr_end;
  unsigned int dst;
  unsigned int a;
  int i;

  printf("Decoding level script %X\n", levels[l].start);

  a = levels[l].start;
  // length = 0 ends level script
  while (a < levels[l].end && data[a + 1] != 0) {
    printf("%06X [%03X] ", a, a - levels[l].start);
    switch (data[a]) {
    case 0x00:
      printf("LoadJump0");
      break; // load and jump from ROM into a RAM segment
    case 0x01:
      printf("LoadJump1");
      break; // load and jump from ROM into a RAM segment
    case 0x02:
      printf("EndLevel ");
      break; // end of level layout data
    case 0x03:
      printf("Delay03  ");
      break; // delay frames
    case 0x04:
      printf("Delay04  ");
      break; // delay frames and signal end
    case 0x05:
      printf("JumpSeg  ");
      break; // jump to level script at segmented address
    case 0x06:
      printf("PushJump ");
      break; // push script stack and jump to segmented address
    case 0x07:
      printf("PopScript");
      break; // pop script stack, return to prev 0x06 or 0x0C
    case 0x08:
      printf("Push16   ");
      break; // push script stack and 16-bit value
    case 0x09:
      printf("Pop16    ");
      break; // pop script stack and 16-bit value
    case 0x0A:
      printf("PushNull ");
      break; // push script stack and 32-bit 0x00000000
    case 0x0B:
      printf("CondPop  ");
      break; // conditional stack pop
    case 0x0C:
      printf("CondJump ");
      break; // conditional jump to segmented address
    case 0x0D:
      printf("CondPush ");
      break; // conditional stack push
    case 0x0E:
      printf("CondSkip ");
      break; // conditional skip over following 0x0F and 0x10 commands
    case 0x0F:
      printf("SkipNext ");
      break; // skip over following 0x10 commands
    case 0x10:
      printf("NoOp     ");
      break; // no operation
    case 0x11:
      printf("AccumAsm1");
      break; // set accumulator from ASM function
    case 0x12:
      printf("AccumAsm2");
      break; // actively set accumulator from ASM function
    case 0x13:
      printf("SetAccum ");
      break; // set accumulator to constant value
    case 0x14:
      printf("PushPool ");
      break; // push pool state
    case 0x15:
      printf("PopPool  ");
      break; // pop pool state
    case 0x16:
      printf("LoadASM  ");
      break; // load ASM into RAM
    case 0x17:
      printf("ROM->Seg ");
      break; // copy uncompressed data from ROM to a RAM segment
    case 0x18:
      printf("MIO0->Seg");
      break; // decompress MIO0 data from ROM and copy it into a RAM segment
    case 0x19:
      printf("MarioFace");
      break; // create Mario face for demo screen
    case 0x1A:
      printf("MIO0Textr");
      break; // decompress MIO0 data from ROM and copy it into a RAM segment
             // (for texture only segments?)
    case 0x1B:
      printf("StartLoad");
      break; // start RAM loading sequence (before 17, 18, 1A)
    case 0x1D:
      printf("EndLoad  ");
      break; // end RAM loading sequence (after 17, 18, 1A)
    case 0x1F:
      printf("StartArea");
      break; // start of an area
    case 0x20:
      printf("EndArea  ");
      break; // end of an area
    case 0x21:
      printf("LoadPoly ");
      break; // load polygon data without geo layout
    case 0x22:
      printf("LdPolyGeo");
      break; // load polygon data with geo layout
    case 0x24:
      printf("PlaceObj ");
      break; // place object in level with behavior
    case 0x25:
      printf("LoadMario");
      break; // load mario object with behavior
    case 0x26:
      printf("ConctWarp");
      break; // connect warps
    case 0x27:
      printf("PaintWarp");
      break; // level warps for paintings
    case 0x28:
      printf("Transport");
      break; // transport Mario to an area
    case 0x2B:
      printf("MarioStrt");
      break; // Mario's default position
    case 0x2E:
      printf("Collision");
      break; // load collision data
    case 0x2F:
      printf("RendrArea");
      break; // decide which area of level geo to render
    case 0x31:
      printf("Terrain  ");
      break; // set default terrain type
    case 0x33:
      printf("FadeColor");
      break; // fade/overlay screen with color
    case 0x34:
      printf("Blackout ");
      break; // blackout screen
    case 0x36:
      printf("Music36  ");
      break; // set music
    case 0x37:
      printf("Music37  ");
      break; // set music
    case 0x39:
      printf("MulObject");
      break; // multiple objects from main level segment
    case 0x3B:
      printf("JetStream");
      break; // define jet streams that repulse / pull Mario
    case 0x3C:
      printf("GetPut   ");
      break; // get/put remote value
    default:
      printf("         ");
      break;
    }
    printf(" %02X %02X %02X%02X ", data[a], data[a + 1], data[a + 2],
           data[a + 3]);
    switch (data[a]) {
    case 0x00: // load and jump from ROM into a RAM segment
    case 0x01: // load and jump from ROM into a RAM segment
      ptr_start = read_u32_be(&data[a + 4]);
      ptr_end = read_u32_be(&data[a + 8]);
      printf("%08X %08X %08X\n", ptr_start, ptr_end,
             read_u32_be(&data[a + 0xc]));
      add_level(levels, lcount, ptr_start, ptr_end);
      break;
    case 0x17: // copy uncompressed data from ROM to a RAM segment
    case 0x18: // decompress MIO0 data from ROM and copy it into a RAM segment
    case 0x1A: // decompress MIO0 data from ROM and copy it into a RAM segment
               // (for texture only segments?)
      ptr_start = read_u32_be(&data[a + 4]);
      ptr_end = read_u32_be(&data[a + 8]);
      printf("%08X %08X\n", ptr_start, ptr_end);
      break;
    case 0x11: // call function
    case 0x12: // call function
      ptr_start = read_u32_be(&data[a + 0x4]);
      printf("%08X\n", ptr_start);
      break;
    case 0x16: // load ASM into RAM
      dst = read_u32_be(&data[a + 0x4]);
      ptr_start = read_u32_be(&data[a + 0x8]);
      ptr_end = read_u32_be(&data[a + 0xc]);
      printf("%08X %08X %08X\n", dst, ptr_start, ptr_end);
      break;
    case 0x25: // load mario object with behavior
    case 0x24: // load object with behavior
      printf("%08X", read_u32_be(&data[a]));
      for (i = 4; i < data[a + 1] - 4; i += 4) {
        printf(" %08X", read_u32_be(&data[a + i]));
      }
      dst = read_u32_be(&data[a + i]);
      printf(" %08X\n", dst);
      break;
    default:
      for (i = 4; i < data[a + 1]; i += 4) {
        printf("%08X ", read_u32_be(&data[a + i]));
      }
      printf("\n");
      break;
    }
    a += data[a + 1];
  }
  printf("Done %X\n\n", levels[l].start);
}

char detectRegion(unsigned char *data) {
  unsigned checksum = read_u32_be(&data[0x10]);
  // add main entry level script
  switch (checksum) {
  case 0xA03CF036:
    return 'E';
  case 0x4EAA3D0E:
    return 'J';
  case 0xD6FBA4A8:
    return 'S'; // Shindou Edition (J)
  case 0x635A2BFF:
    return 'U';
  default:
    ERROR("Unknown ROM checksum: 0x%08X\n", checksum);
    exit(1);
  }
}

unsigned getRegionOffset(char region) {
  switch (region) {
  case 'E':
    return 0xDE160;
  case 'J':
    return 0x1076A0;
  case 'S':
    return 0xE42C0;
  case 'U':
    return 0x108A10;
  default:
    ERROR("Unknown region: '%c'\n", region);
    exit(1);
  }
  return 0;
}

static void walk_scripts(unsigned char *data, unsigned offset) {
  level_t levelscripts[100];
  unsigned lcount = 0;
  unsigned l = 0;
  levelscripts[0].start = offset;
  levelscripts[0].end = offset + 0x30;
  lcount++;
  while (l < lcount) {
    decode_level(data, levelscripts, l, &lcount);
    l++;
  }
}

int main(int argc, char *argv[]) {
  unsigned char *in_buf = NULL;
  long in_size;
  int rom_type;

  arg_config config;

  // Initialize configuration with defaults
  config = default_config;

  // Parse arguments
  if (parse_arguments(argc, argv, &config) != 0) {
    return EXIT_FAILURE;
  }

  // read input file into memory
  in_size = read_file(config.rom_file, &in_buf);
  if (in_size <= 0) {
    ERROR("Error reading input file \"%s\"\n", config.rom_file);
    exit(EXIT_FAILURE);
  }

  // confirm valid SM64
  rom_type = sm64_rom_type(in_buf, in_size);
  if (rom_type < 0) {
    ERROR("This does not appear to be a valid SM64 ROM\n");
    exit(EXIT_FAILURE);
  } else if (rom_type == 1) {
    // byte-swapped BADC format, swap to big-endian ABCD format for processing
    INFO("Byte-swapping ROM\n");
    swap_bytes(in_buf, in_size);
  }

  if (config.offset == 0xFFFFFFFF) {
    if (config.region == 0) {
      config.region = detectRegion(in_buf);
    }
    config.offset = getRegionOffset(config.region);
  }

  // walk those scripts
  walk_scripts(in_buf, config.offset);

  // cleanup
  free(in_buf);

  return EXIT_SUCCESS;
}
