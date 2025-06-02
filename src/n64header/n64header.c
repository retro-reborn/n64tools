#include "argparse.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N64HEADER_VERSION "1.0"

typedef struct {
  char *rom_file;
  bool verbose;
} arg_config;

static arg_config default_config = {
    NULL,  // ROM filename
    false, // verbose
};

typedef struct {
  // 0x00-0x03: Standard header fields
  unsigned char reserved_byte;    // 0x00: Reserved (0x80 for commercial games)
  unsigned char pi_bsd_config[3]; // 0x01-0x03: PI BSD DOM1 Configuration Flags
  unsigned int clock_rate;        // 0x04: Clock Rate
  unsigned int boot_address;      // 0x08: Boot Address (initial PC)
  unsigned int libultra_version;  // 0x0C: Libultra Version

  // 0x10-0x17: Check Code (64-bit)
  unsigned int check_code_hi; // 0x10: Check Code (high 32 bits)
  unsigned int check_code_lo; // 0x14: Check Code (low 32 bits)

  // 0x18-0x1F: Reserved fields
  unsigned int reserved1; // 0x18: Reserved
  unsigned int reserved2; // 0x1C: Reserved

  // 0x20-0x33: Game Title (20 bytes)
  char game_title[21]; // 0x20: Game Title (20 bytes + null terminator)

  // 0x34-0x3A: Reserved or Homebrew fields
  unsigned char
      reserved3[7]; // 0x34-0x3A: Reserved (or homebrew controller fields)

  // 0x3B-0x3F: Game identification
  char game_code[5];         // 0x3B: Game Code (4 bytes + null terminator)
  unsigned char rom_version; // 0x3F: ROM Version

  // Parsed fields for easier access
  char category_code;    // Category part of game code
  char unique_code[3];   // Unique part of game code (2 bytes + null)
  char destination_code; // Destination/region part of game code

  // Advanced Homebrew ROM Header detection
  bool is_homebrew_header;         // True if "ED" found at 0x3C
  unsigned char controller_1;      // 0x34: Controller 1 (homebrew)
  unsigned char controller_2;      // 0x35: Controller 2 (homebrew)
  unsigned char controller_3;      // 0x36: Controller 3 (homebrew)
  unsigned char controller_4;      // 0x37: Controller 4 (homebrew)
  unsigned char homebrew_flags[4]; // 0x38-0x3B: Homebrew flags
  unsigned char savetype;          // 0x3F: Savetype (homebrew)
} n64_header;

// Function to parse command line arguments
static int parse_arguments(int argc, char *argv[], arg_config *config) {
  arg_parser *parser;
  int result;

  // Initialize the argument parser
  parser =
      argparse_init("n64header", N64HEADER_VERSION, "N64 ROM header viewer");
  if (parser == NULL) {
    ERROR("Error: Failed to initialize argument parser\n");
    return -1;
  }

  // Add the verbose flag
  argparse_add_flag(parser, 'v', "verbose", ARG_TYPE_NONE,
                    "Enable verbose output", NULL, &config->verbose, false,
                    NULL, 0);

  // Add positional argument for ROM file
  argparse_add_positional(parser, "FILE", "N64 ROM file to analyze",
                          ARG_TYPE_STRING, &config->rom_file, true);

  // Parse the arguments
  result = argparse_parse(parser, argc, argv);

  // Free the parser
  argparse_free(parser);

  return result;
}

// Function to determine the ROM format (big endian, byte-swapped, little
// endian)
static const char *detect_rom_format(unsigned char *buf) {
  const unsigned char z64_magic[] = {0x80, 0x37, 0x12,
                                     0x40}; // big-endian (ABCD)
  const unsigned char v64_magic[] = {0x37, 0x80, 0x40,
                                     0x12}; // byte-swapped (BADC)
  const unsigned char n64_magic[] = {0x40, 0x12, 0x37,
                                     0x80}; // little-endian (DCBA)

  if (!memcmp(buf, z64_magic, sizeof(z64_magic))) {
    return "Z64 (big-endian/ABCD)";
  } else if (!memcmp(buf, v64_magic, sizeof(v64_magic))) {
    return "V64 (byte-swapped/BADC)";
  } else if (!memcmp(buf, n64_magic, sizeof(n64_magic))) {
    return "N64 (little-endian/DCBA)";
  } else {
    return "Unknown format";
  }
}

// Function to convert V64 format to Z64 format (big-endian)
// This is done by swapping every two bytes
static void swap_bytes_v64(unsigned char *data, int length) {
  unsigned char tmp;
  int i;
  for (i = 0; i < length; i += 2) {
    tmp = data[i];
    data[i] = data[i + 1];
    data[i + 1] = tmp;
  }
}

// Function to convert N64 format to Z64 format (big-endian)
// This is done by swapping every four bytes
static void swap_bytes_n64(unsigned char *data, int length) {
  unsigned char tmp;
  int i;
  for (i = 0; i < length; i += 4) {
    tmp = data[i];
    data[i] = data[i + 3];
    data[i + 3] = tmp;
    tmp = data[i + 1];
    data[i + 1] = data[i + 2];
    data[i + 2] = tmp;
  }
}

// Get country name from code
static const char *get_country_name(char country_code) {
  switch (country_code) {
  case 'A':
    return "All"; // Works for all regions
  case 'B':
    return "Brazil";
  case 'C':
    return "China";
  case 'D':
    return "Germany";
  case 'E':
    return "North America";
  case 'F':
    return "France";
  case 'G':
    return "Gateway 64 (NTSC)";
  case 'H':
    return "Netherlands";
  case 'I':
    return "Italy";
  case 'J':
    return "Japan";
  case 'K':
    return "Korea";
  case 'L':
    return "Gateway 64 (PAL)";
  case 'N':
    return "Canada";
  case 'P':
    return "Europe";
  case 'S':
    return "Spain";
  case 'U':
    return "Australia";
  case 'W':
    return "Scandinavia";
  case 'X':
    return "Europe";
  case 'Y':
    return "Europe";
  case 'Z':
    return "Europe";
  case '7':
    return "Beta"; // Non-standard, but commonly found
  case '\0':
    return "Region Free"; // Null byte = region free
  default:
    return "Unknown"; // Probably fan-made or homebrew
  }
}

// Read header from ROM file
static int read_header(const char *rom_file, n64_header *header) {
  unsigned char buf[64]; // N64 header is 64 bytes
  unsigned long length;
  FILE *file;

  if (g_verbosity) {
    printf("Opening file: %s\n", rom_file);
  }

  file = fopen(rom_file, "rb");
  if (!file) {
    ERROR("Error: Could not open file '%s'\n", rom_file);
    return -1;
  }

  length = fread(buf, 1, sizeof(buf), file);
  fclose(file);

  if (length < sizeof(buf)) {
    ERROR("Error: File '%s' is too small to be an N64 ROM\n", rom_file);
    return -1;
  }

  if (g_verbosity) {
    printf("Detecting ROM format...\n");
  }

  // Convert to big-endian if necessary
  const char *rom_format = detect_rom_format(buf);
  if (strstr(rom_format, "V64")) {
    if (g_verbosity) {
      printf("Converting from V64 (byte-swapped) format to Z64 (big-endian)\n");
    }
    swap_bytes_v64(buf, sizeof(buf));
  } else if (strstr(rom_format, "N64")) {
    if (g_verbosity) {
      printf(
          "Converting from N64 (little-endian) format to Z64 (big-endian)\n");
    }
    swap_bytes_n64(buf, sizeof(buf));
  }

  // Parse header according to official specification
  // 0x00: Reserved byte (usually 0x80)
  header->reserved_byte = buf[0x00];

  // 0x01-0x03: PI BSD DOM1 Configuration Flags
  header->pi_bsd_config[0] = buf[0x01];
  header->pi_bsd_config[1] = buf[0x02];
  header->pi_bsd_config[2] = buf[0x03];

  // 0x04: Clock Rate
  header->clock_rate = read_u32_be(&buf[0x04]);

  // 0x08: Boot Address (initial PC)
  header->boot_address = read_u32_be(&buf[0x08]);

  // 0x0C: Libultra Version
  header->libultra_version = read_u32_be(&buf[0x0C]);

  // 0x10-0x17: Check Code (64-bit value)
  header->check_code_hi = read_u32_be(&buf[0x10]);
  header->check_code_lo = read_u32_be(&buf[0x14]);

  // 0x18-0x1F: Reserved fields
  header->reserved1 = read_u32_be(&buf[0x18]);
  header->reserved2 = read_u32_be(&buf[0x1C]);

  // 0x20-0x33: Game Title (20 bytes)
  memcpy(header->game_title, &buf[0x20], 20);
  header->game_title[20] = '\0'; // Null-terminate

  // 0x34-0x3A: Reserved or Homebrew fields
  memcpy(header->reserved3, &buf[0x34], 7);

  // Check for Advanced Homebrew ROM Header (0x3C-0x3D should be "ED")
  header->is_homebrew_header = (buf[0x3C] == 'E' && buf[0x3D] == 'D');

  if (header->is_homebrew_header) {
    // Parse homebrew-specific fields
    header->controller_1 = buf[0x34];
    header->controller_2 = buf[0x35];
    header->controller_3 = buf[0x36];
    header->controller_4 = buf[0x37];

    // 0x38-0x3B: Homebrew flags
    header->homebrew_flags[0] = buf[0x38];
    header->homebrew_flags[1] = buf[0x39];
    header->homebrew_flags[2] = buf[0x3A];
    header->homebrew_flags[3] = buf[0x3B];

    // 0x3F: Savetype (for homebrew)
    header->savetype = buf[0x3F];
  }

  // 0x3B-0x3E: Game Code (4 bytes)
  memcpy(header->game_code, &buf[0x3B], 4);
  header->game_code[4] = '\0'; // Null-terminate

  // 0x3F: ROM Version
  header->rom_version = buf[0x3F];

  // Parse game code components for easier access
  if (strlen(header->game_code) >= 4) {
    header->category_code = header->game_code[0];
    header->unique_code[0] = header->game_code[1];
    header->unique_code[1] = header->game_code[2];
    header->unique_code[2] = '\0';
    header->destination_code = header->game_code[3];
  } else {
    header->category_code = '?';
    header->unique_code[0] = '?';
    header->unique_code[1] = '?';
    header->unique_code[2] = '\0';
    header->destination_code = '?';
  }

  return 0;
}

// Get category name from code
static const char *get_category_name(char category_code) {
  switch (category_code) {
  case 'N':
    return "Game Pak";
  case 'D':
    return "64DD Disk";
  case 'C':
    return "Expandable Game: Game Pak Part";
  case 'E':
    return "Expandable Game: 64DD Disk Part";
  case 'Z':
    return "Aleck64 Game Pak";
  default:
    return "Unknown";
  }
}

// Get controller name from homebrew controller code
static const char *get_controller_name(unsigned char controller_code) {
  if (controller_code == 0x00)
    return "None";
  if (controller_code >= 0x01 && controller_code <= 0x7F)
    return "Standard N64 Controller";
  if (controller_code >= 0x80 && controller_code <= 0xFE)
    return "Non-standard Controller";
  return "Unknown";
}

// Get savetype name from homebrew savetype code
static const char *get_savetype_name(unsigned char savetype) {
  // Decode savetype as a bitfield
  if (savetype & 0x80)
    return "16K EEPROM";
  if (savetype & 0x40)
    return "4K EEPROM";
  if (savetype & 0x20)
    return "128K Flash RAM";
  if (savetype & 0x10)
    return "32K SRAM";
  if (savetype & 0x08)
    return "256K Flash RAM";
  return "None";
}

// Get libultra version string from version code
static const char *get_libultra_version_string(unsigned int version) {
  // Common libultra versions
  switch (version) {
  case 0x0000144B:
    return "2.0K";
  case 0x0000144C:
    return "2.0L";
  case 0x0000144D:
    return "2.0D";
  case 0x00001446:
    return "2.0F";
  case 0x00001447:
    return "2.0G";
  case 0x00001448:
    return "2.0H";
  case 0x00001449:
    return "2.0I";
  case 0x0000144A:
    return "2.0J";
  default:
    return "Unknown";
  }
}

// Display header information
static void display_header(const n64_header *header, const char *rom_format) {
  int i;
  char ascii_name[21];

  // Clean up game title to printable ASCII
  for (i = 0; i < 20; i++) {
    if (header->game_title[i] >= 32 && header->game_title[i] < 127) {
      ascii_name[i] = header->game_title[i];
    } else {
      ascii_name[i] = '.';
    }
  }
  ascii_name[20] = '\0';

  printf("N64 ROM Header Information\n");
  printf("==========================\n");
  printf("Format:                   %s\n", rom_format);

  printf("\nStandard Header Fields:\n");
  printf("  Reserved byte:            0x%02X\n", header->reserved_byte);
  printf("  PI BSD DOM1 config:       0x%02X%02X%02X\n",
         header->pi_bsd_config[0], header->pi_bsd_config[1],
         header->pi_bsd_config[2]);
  printf("  Clock rate:               0x%08X\n", header->clock_rate);
  printf("  Boot address:             0x%08X\n", header->boot_address);
  printf("  Libultra version:         0x%08X (%s)\n", header->libultra_version,
         get_libultra_version_string(header->libultra_version));

  printf("\nSecurity:\n");
  printf("  Check code:               0x%08X%08X\n", header->check_code_hi,
         header->check_code_lo);

  printf("\nReserved Fields:\n");
  printf("  Reserved 1:               0x%08X\n", header->reserved1);
  printf("  Reserved 2:               0x%08X\n", header->reserved2);

  printf("\nGame Information:\n");
  printf("  Game title:               \"%s\"\n", ascii_name);
  printf("  Game code:                %s\n", header->game_code);
  printf("    Category code:          %c (%s)\n", header->category_code,
         get_category_name(header->category_code));
  printf("    Unique code:            %s\n", header->unique_code);
  printf("    Destination code:       %c (%s)\n", header->destination_code,
         get_country_name(header->destination_code));
  printf("  ROM version:              0x%02X (%d)\n", header->rom_version,
         header->rom_version);

  // Advanced Homebrew ROM Header information
  if (header->is_homebrew_header) {
    printf("\nAdvanced Homebrew ROM Header:\n");
    printf("  Game ID:                  ED (Homebrew format detected)\n");
    printf("  Controller 1:             0x%02X (%s)\n", header->controller_1,
           get_controller_name(header->controller_1));
    printf("  Controller 2:             0x%02X (%s)\n", header->controller_2,
           get_controller_name(header->controller_2));
    printf("  Controller 3:             0x%02X (%s)\n", header->controller_3,
           get_controller_name(header->controller_3));
    printf("  Controller 4:             0x%02X (%s)\n", header->controller_4,
           get_controller_name(header->controller_4));
    printf("  Homebrew flags:           0x%02X%02X%02X%02X\n",
           header->homebrew_flags[0], header->homebrew_flags[1],
           header->homebrew_flags[2], header->homebrew_flags[3]);
    printf("  Savetype:                 0x%02X (%s)\n", header->savetype,
           get_savetype_name(header->savetype));
  } else {
    printf("\nReserved/Other Fields:\n");
    printf("  Reserved 3:               ");
    for (i = 0; i < 7; i++) {
      printf("%02X", header->reserved3[i]);
    }
    printf("\n");
  }
}

int main(int argc, char *argv[]) {
  arg_config config;
  n64_header header;
  unsigned char buf[64];
  long length;
  FILE *file;

  // Initialize configuration with defaults
  config = default_config;

  // Parse arguments
  if (parse_arguments(argc, argv, &config) != 0) {
    return EXIT_FAILURE;
  }

  // Open file and read first 64 bytes
  file = fopen(config.rom_file, "rb");
  if (!file) {
    fprintf(stderr, "Error: Could not open file '%s'\n", config.rom_file);
    return EXIT_FAILURE;
  }

  length = fread(buf, 1, sizeof(buf), file);
  fclose(file);

  if (length < 64) {
    fprintf(stderr, "Error: File '%s' is too small to be an N64 ROM\n",
            config.rom_file);
    return EXIT_FAILURE;
  }

  // Get ROM format
  const char *rom_format = detect_rom_format(buf);

  // Read and parse header
  if (read_header(config.rom_file, &header) != 0) {
    return EXIT_FAILURE;
  }

  // Display header information
  display_header(&header, rom_format);

  return EXIT_SUCCESS;
}
