#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse.h"
#include "utils.h"

#define SM64GEO_VERSION "0.1"

typedef struct {
  char *in_filename;
  char *out_filename;
  unsigned int offset;
  unsigned int length;
} arg_config;

static arg_config default_config = {NULL, NULL, 0, 0};

static void print_spaces(FILE *fp, int count) {
  int i;
  for (i = 0; i < count; i++) {
    fputc(' ', fp);
  }
}

void print_geo(FILE *out, unsigned char *data, unsigned int offset,
               unsigned int length) {
  unsigned int a = offset;
  int indent = 0;
  int i;
  while (a < offset + length) {
    switch (data[a]) {
    case 0x00: // end
    case 0x01:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x09:
    case 0x0B:
    case 0x0C:
    case 0x17:
    case 0x20:
      i = 4;
      break;
    case 0x02:
    case 0x0D:
    case 0x0E:
    case 0x12:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x18:
    case 0x19:
      i = 8;
      break;
    case 0x08:
    case 0x13:
    case 0x1C:
      i = 12;
      break;
    case 0x10:
      i = 16;
      break;
    case 0x0F:
      i = 20;
      break;
    case 0x0A:
      i = 8;
      if (data[a + 1]) {
        i += 4;
      }
      break;
    case 0x11:
    case 0x1D:
      i = 8;
      if (data[a + 1] & 0x80) {
        i += 4;
      }
      break;
    default:
      i = 4;
      ERROR("WHY? %06X %2X\n", a, data[a]);
    }
    if (data[a] == 0x05 && indent > 1) {
      indent -= 2;
    }
    if (data[a] == 0x01) {
      indent = 0;
    }
    fprintf(out, "%4X: ", a);
    print_spaces(out, indent);
    fprintf(out, "[ ");
    fprint_hex(out, &data[a], i);
    fprintf(out, "]\n");
    if (data[a] == 0x04) {
      indent += 2;
    }
    a += i;
  }
}

// parse command line arguments using the new argparse utility
static int parse_arguments(int argc, char *argv[], arg_config *config) {
  arg_parser *parser;
  int result;

  // Initialize the argument parser
  parser = argparse_init("sm64geo", SM64GEO_VERSION,
                         "Super Mario 64 geometry layout decoder");
  if (parser == NULL) {
    ERROR("Error: Failed to initialize argument parser\n");
    return -1;
  }

  // Add the length flag
  argparse_add_flag(
      parser, 'l', "length", ARG_TYPE_UINT,
      "length of data to decode in bytes (default: length of file)", "LENGTH",
      &config->length, false, NULL, 0);

  // Add the offset flag
  argparse_add_flag(parser, 'o', "offset", ARG_TYPE_UINT,
                    "starting offset in FILE (default: 0)", "OFFSET",
                    &config->offset, false, NULL, 0);

  // Add verbose flag
  argparse_add_flag(parser, 'v', "verbose", ARG_TYPE_NONE,
                    "verbose progress output", NULL, &g_verbosity, false, NULL,
                    0);

  // Add positional arguments
  argparse_add_positional(parser, "FILE", "input file", ARG_TYPE_STRING,
                          &config->in_filename, true);

  argparse_add_positional(parser, "OUTPUT", "output file (default: stdout)",
                          ARG_TYPE_STRING, &config->out_filename, false);

  // Parse the arguments
  result = argparse_parse(parser, argc, argv);

  // Free the parser
  argparse_free(parser);

  return result;
}

int main(int argc, char *argv[]) {
  arg_config config;
  FILE *fout;
  unsigned char *data;
  long size;

  // Initialize configuration with defaults
  config = default_config;

  // Parse command line arguments
  if (parse_arguments(argc, argv, &config) != 0) {
    return EXIT_FAILURE;
  }

  // Setup output file
  if (config.out_filename == NULL) {
    fout = stdout;
  } else {
    fout = fopen(config.out_filename, "w");
    if (fout == NULL) {
      perror("Error opening output file");
      return EXIT_FAILURE;
    }
  }

  // operation
  size = read_file(config.in_filename, &data);
  if (size < 0) {
    perror("Error opening input file");
    return EXIT_FAILURE;
  }
  if (config.length == 0) {
    config.length = size - config.offset;
  }
  if (config.offset >= (unsigned int)size) {
    ERROR("Error: offset greater than file size (%X > %X)\n", config.offset,
          (unsigned int)size);
    return EXIT_FAILURE;
  }
  if (config.offset + config.length > (unsigned int)size) {
    ERROR("Warning: length goes beyond file size (%X > %X), truncating\n",
          config.offset + config.length, (unsigned int)size);
    config.length = size - config.offset;
  }
  print_geo(fout, data, config.offset, config.length);
  free(data);

  if (fout != stdout) {
    fclose(fout);
  }

  return EXIT_SUCCESS;
}
