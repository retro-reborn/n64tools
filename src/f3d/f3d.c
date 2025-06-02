#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse.h"
#include "utils.h"

#define F3D_VERSION "0.2"

#define F3D_MOVEMEM 0x03
#define F3D_VTX 0x04
#define F3D_DL 0x06
#define F3D_QUAD 0xB5
#define F3D_CLRGEOMODE 0xB6
#define F3D_SETGEOMODE 0xB7
#define F3D_ENDDL 0xB8
#define F3D_TEXTURE 0xBB
#define F3D_TRI1 0xBF
#define G_SETTILESIZE 0xF2
#define G_LOADBLOCK 0xF3
#define G_SETTILE 0xF5
#define G_SETFOGCOLOR 0xF8
#define G_SETENVCOLOR 0xFB
#define G_SETCOMBINE 0xFC
#define G_SETTIMG 0xFD

typedef struct {
  char *in_filename;
  char *out_filename;
  unsigned int offset;
  unsigned int length;
} arg_config;

static arg_config default_config = {NULL, NULL, 0, 0};

static void get_mode_string(unsigned char *data, char *description) {
  unsigned int val = read_u32_be(&data[4]);
  switch (val) {
  case 0x00022000:
    sprintf(description, "vertex RGB, no culling");
    break;
  case 0x00020000:
    sprintf(description, "vertex RGB, culling");
    break;
  case 0x00000000:
    sprintf(description, "no vertex RGB, culling");
    break;
  case 0x00002200:
    sprintf(description, "no vertex RGB, no culling");
    break;
  default:
    sprintf(description, "unknown");
    break;
  }
}

static void print_f3d(FILE *fout, unsigned char *data) {
  char description[64];
  unsigned char offset;
  unsigned int address;
  unsigned int val;
  // default description
  description[0] = '\0';
  switch (data[0]) {
  case F3D_MOVEMEM:
    switch (data[1]) {
    case 0x86:
      sprintf(description, "light");
      break;
    case 0x88:
      sprintf(description, "dark ");
      break;
    }
    address = read_u32_be(&data[4]);
    fprintf(fout, "%14s %s %08X", "F3D_MOVEMEM", description, address);
    break;
  case F3D_VTX:
    offset = data[1];
    val = read_u16_be(&data[2]);
    address = read_u32_be(&data[4]);
    fprintf(fout, "%14s %02X %04X (%d) %08X", "F3D_VTX", offset, val,
            val / 0x10, address);
    break;
  case F3D_DL:
    address = read_u32_be(&data[4]);
    fprintf(fout, "%14s %08X", "F3D_DL", address);
    break;
  case F3D_QUAD: {
    unsigned char vertex[6];
    vertex[0] = data[1] / 0x0A;
    vertex[1] = data[2] / 0x0A;
    vertex[2] = data[3] / 0x0A;
    // data[6] unused
    vertex[3] = data[5] / 0x0A;
    vertex[4] = data[6] / 0x0A;
    vertex[5] = data[7] / 0x0A;
    fprintf(fout, "%14s %3d %3d %3d %3d %3d %3d", "F3D_QUAD", vertex[0],
            vertex[1], vertex[2], vertex[3], vertex[4], vertex[5]);
    break;
  }
  case F3D_CLRGEOMODE:
    get_mode_string(data, description);
    fprintf(fout, "%14s %s", "F3D_CLRGEOMODE", description);
    break;
  case F3D_SETGEOMODE:
    get_mode_string(data, description);
    fprintf(fout, "%14s %s", "F3D_SETGEOMODE", description);
    break;
  case F3D_ENDDL:
    fprintf(fout, "%14s %s", "F3D_ENDL", description);
    break;
  case F3D_TEXTURE:
    switch (data[3]) {
    case 0x00:
      val = read_u32_be(&data[4]);
      if (val == 0xFFFFFFFF) {
        sprintf(description, "end, reset scale to 0");
      }
      break;
    case 0x01:
      val = read_u32_be(&data[4]);
      if (val == 0xFFFFFFFF) {
        sprintf(description, "start, set scale to 1");
      } else if (val == 0x0F8007C0) {
        sprintf(description, "start environment mapping");
      }
      break;
    }
    fprintf(fout, "%14s %s", "F3D_TEXTURE", description);
    break;
  case F3D_TRI1: {
    unsigned char vertex[3];
    vertex[0] = data[5] / 0x0A;
    vertex[1] = data[6] / 0x0A;
    vertex[2] = data[7] / 0x0A;
    fprintf(fout, "%14s %3d %3d %3d", "F3D_TRI1", vertex[0], vertex[1],
            vertex[2]);
    break;
  }
  case G_SETTILESIZE: {
    unsigned short width, height;
    width = (((data[5] << 8) | (data[6] & 0xF0)) >> 6) + 1;
    height = (((data[6] & 0x0F) << 8 | data[7]) >> 2) + 1;
    fprintf(fout, "%14s %2d %2d", "G_SETTILESIZE", width, height);
    break;
  }
  case G_LOADBLOCK: {
    unsigned w0 = read_u32_be(data);
    unsigned w1 = read_u32_be(&data[4]);
    unsigned uls = (w0 >> 12) & 0x3FF;
    unsigned ult = w0 & 0x3FF;
    unsigned lrs = (w1 >> 12) & 0x3FF;
    unsigned dxt = w1 & 0x3FF;
    fprintf(fout, "%14s %03X %03X %03X %u", "G_LOADBLOCK", uls, ult, lrs, dxt);
    break;
  }
  case G_SETTILE: {
    const char *fmt_table[] = {"RGBA", "YUV", "CI", "IA", "I"};
    unsigned format = (data[1] >> 5) & 0x7; // bits 21-23
    unsigned size = (data[1] >> 3) & 0x3;   // bits 19-20
    unsigned depth = 0;
    switch (size) {
    case 0:
      depth = 4;
      break;
    case 1:
      depth = 8;
      break;
    case 2:
      depth = 16;
      break;
    case 3:
      depth = 32;
      break;
    default:
      ERROR("Unknown depth: %d\n", size);
    }
    if (format < DIM(fmt_table)) {
      sprintf(description, "%s %d", fmt_table[format], depth);
    }
    fprintf(fout, "%14s %s", "G_SETTILE", description);
    break;
  }
  case G_SETFOGCOLOR:
    fprintf(fout, "%14s %3d, %3d, %3d, %3d", "G_SETFOGCOLOR", data[4], data[5],
            data[6], data[7]);
    break;
  case G_SETENVCOLOR:
    fprintf(fout, "%14s %3d, %3d, %3d, %3d", "G_SETENVCOLOR", data[4], data[5],
            data[6], data[7]);
    break;
  case G_SETCOMBINE: {
    struct {
      unsigned char data[7];
      char *description;
    } table[] = {
        {{0x12, 0x7F, 0xFF, 0xFF, 0xFF, 0xF8, 0x38}, "solid RGBA"},
        {{0x12, 0x18, 0x24, 0xFF, 0x33, 0xFF, 0xFF}, "alpha RGBA"},
    };
    unsigned i;
    for (i = 0; i < DIM(table); i++) {
      if (!memcmp(table[i].data, &data[1], 7)) {
        strcpy(description, table[i].description);
      }
    }
    fprintf(fout, "%14s %s", "G_SETCOMBINE", description);
    break;
  }
  case G_SETTIMG:
    address = read_u32_be(&data[4]);
    fprintf(fout, "%14s %08X", "G_SETTIMG", address);
    break;
  default:
    fprintf(fout, "%14s %s", "Unknown", description);
    break;
  }
}

// parse command line arguments
static int parse_arguments(int argc, char *argv[], arg_config *config) {
  arg_parser *parser;
  int result;

  // Initialize the argument parser
  parser = argparse_init("f3d", F3D_VERSION, "N64 Fast3D display list decoder");
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
  unsigned int i;
  int stop_on_enddl = 0;

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
    stop_on_enddl = 1;
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

  for (i = config.offset; i < config.offset + config.length; i += 8) {
    fprintf(fout, "%05X: %08X %08X", i, read_u32_be(&data[i]),
            read_u32_be(&data[i + 4]));
    print_f3d(fout, &data[i]);
    fprintf(fout, "\n");
    if (stop_on_enddl && F3D_ENDDL == data[i]) {
      break;
    }
  }

  free(data);
  if (fout != stdout) {
    fclose(fout);
  }

  return 0;
}
