#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "libn64.h"
#include "utils.h"
#include "argparse.h"

#define SM64EXTEND_VERSION "0.3.2"

// default configuration
static const sm64_config default_config = {
    NULL, // input filename
    NULL, // extended filename
    64,   // extended size
    32,   // MIO0 padding
    1,    // MIO0 alignment
    0,    // fill old MIO0 blocks
    0,    // dump MIO0 blocks to files
};

// parse command line arguments
static int parse_arguments(int argc, char *argv[], sm64_config *config) {
  arg_parser *parser;
  int result;

  // Initialize the argument parser
  parser = argparse_init("sm64extend", SM64EXTEND_VERSION, 
                        "Super Mario 64 ROM extender");
  if (parser == NULL) {
    ERROR("Error: Failed to initialize argument parser\n");
    return -1;
  }

  // Add flag arguments
  argparse_add_flag(parser, 'a', "alignment", ARG_TYPE_UINT,
                   "byte boundary to align MIO0 blocks (default: 1)",
                   "ALIGNMENT", &config->alignment, false, NULL, 0);
  
  argparse_add_flag(parser, 'p', "padding", ARG_TYPE_UINT,
                   "padding to insert between MIO0 blocks in KB (default: 32)",
                   "PADDING", &config->padding, false, NULL, 0);
  
  argparse_add_flag(parser, 's', "size", ARG_TYPE_UINT,
                   "size of the extended ROM in MB (default: 64)",
                   "SIZE", &config->ext_size, false, NULL, 0);
  
  argparse_add_flag(parser, 'd', "dump", ARG_TYPE_NONE,
                   "dump MIO0 blocks to files in 'mio0files' directory",
                   NULL, &config->dump, false, NULL, 0);
  
  argparse_add_flag(parser, 'f', "fill", ARG_TYPE_NONE,
                   "fill old MIO0 blocks with 0x01",
                   NULL, &config->fill, false, NULL, 0);
  
  argparse_add_flag(parser, 'v', "verbose", ARG_TYPE_NONE,
                   "verbose progress output",
                   NULL, &g_verbosity, false, NULL, 0);

  // Add positional arguments
  argparse_add_positional(parser, "FILE", "input ROM file", 
                         ARG_TYPE_STRING, &config->in_filename, true);
  
  argparse_add_positional(parser, "OUT_FILE", 
                         "output ROM file (default: replaces FILE extension with .ext.z64)",
                         ARG_TYPE_STRING, &config->ext_filename, false);

  // Parse the arguments
  result = argparse_parse(parser, argc, argv);
  
  // Free the parser
  argparse_free(parser);
  
  return result;
}

int main(int argc, char *argv[]) {
  char ext_filename[FILENAME_MAX];
  sm64_config config;
  unsigned char *in_buf = NULL;
  unsigned char *out_buf = NULL;
  long in_size;
  long bytes_written;
  rom_type rtype;
  rom_version rversion;

  // Initialize configuration with defaults
  config = default_config;
  
  // Parse command line arguments
  if (parse_arguments(argc, argv, &config) != 0) {
    return EXIT_FAILURE;
  }
  
  // Generate output filename if not provided
  if (config.ext_filename == NULL) {
    config.ext_filename = ext_filename;
    generate_filename(config.in_filename, config.ext_filename, "ext.z64");
  }

  // validate arguments
  if (config.ext_size < 16 || config.ext_size > 64) {
    ERROR("Error: Extended size must be between 16 and 64 MB\n");
    exit(EXIT_FAILURE);
  }
  if (!is_power2(config.alignment)) {
    ERROR("Error: Alignment must be power of 2\n");
    exit(EXIT_FAILURE);
  }

  // convert sizes to bytes
  config.ext_size *= MB;
  config.padding *= KB;

  // generate MIO0 directory
  if (config.dump) {
    make_dir(MIO0_DIR);
  }

  // read input file into memory
  in_size = read_file(config.in_filename, &in_buf);
  if (in_size <= 0) {
    ERROR("Error reading input file \"%s\"\n", config.in_filename);
    exit(EXIT_FAILURE);
  }

  // confirm valid SM64
  rtype = sm64_rom_type(in_buf, in_size);
  switch (rtype) {
  case ROM_INVALID:
    ERROR("This does not appear to be a valid SM64 ROM\n");
    exit(EXIT_FAILURE);
    break;
  case ROM_SM64_BS:
    INFO("Converting ROM from byte-swapped to big-endian\n");
    swap_bytes(in_buf, in_size);
    break;
  case ROM_SM64_BE:
    break;
  case ROM_SM64_LE:
    INFO("Converting ROM from little to big-endian\n");
    reverse_endian(in_buf, in_size);
    break;
  case ROM_SM64_BE_EXT:
    ERROR("This ROM is already extended!\n");
    exit(EXIT_FAILURE);
    break;
  }

  rversion = sm64_rom_version(in_buf);
  if (rversion == VERSION_UNKNOWN) {
    ERROR("Unknown SM64 ROM version\n");
    exit(EXIT_FAILURE);
  }

  // allocate output memory
  out_buf = malloc(config.ext_size);

  // copy file from input to output
  memcpy(out_buf, in_buf, in_size);

  // fill new space with 0x01
  memset(&out_buf[in_size], 0x01, config.ext_size - in_size);

  // decode SM64 MIO0 files and adjust pointers
  sm64_decompress_mio0(&config, in_buf, in_size, out_buf);

  // update N64 header CRC
  sm64_update_checksums(out_buf);

  // write to output file
  bytes_written = write_file(config.ext_filename, out_buf, config.ext_size);
  if (bytes_written < (long)config.ext_size) {
    ERROR("Error writing bytes to output file \"%s\"\n", config.ext_filename);
    exit(EXIT_FAILURE);
  }

  return EXIT_SUCCESS;
}
