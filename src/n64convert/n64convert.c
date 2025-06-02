#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "utils.h"
#include "argparse.h"

#define N64CONVERT_VERSION "1.0"

typedef enum {
    ROM_FORMAT_Z64, // Big-endian (ABCD) - native N64 format
    ROM_FORMAT_V64, // Byte-swapped (BADC)
    ROM_FORMAT_N64, // Little-endian (DCBA)
    ROM_FORMAT_UNKNOWN
} rom_format_t;

typedef struct {
    char *input_file;
    char *output_file;
    rom_format_t target_format;
    bool force;
} arg_config;

static arg_config default_config = {
    NULL,               // input_file
    NULL,               // output_file
    ROM_FORMAT_Z64,     // target_format (default to native)
    false,              // force
};

// ROM format magic bytes
static const unsigned char z64_magic[] = {0x80, 0x37, 0x12, 0x40}; // ABCD
static const unsigned char v64_magic[] = {0x37, 0x80, 0x40, 0x12}; // BADC
static const unsigned char n64_magic[] = {0x40, 0x12, 0x37, 0x80}; // DCBA

// Function to detect ROM format
static rom_format_t detect_rom_format(unsigned char *buf) {
    if (!memcmp(buf, z64_magic, sizeof(z64_magic))) {
        return ROM_FORMAT_Z64;
    } else if (!memcmp(buf, v64_magic, sizeof(v64_magic))) {
        return ROM_FORMAT_V64;
    } else if (!memcmp(buf, n64_magic, sizeof(n64_magic))) {
        return ROM_FORMAT_N64;
    } else {
        return ROM_FORMAT_UNKNOWN;
    }
}

// Function to get format name string
static const char *get_format_name(rom_format_t format) {
    switch (format) {
        case ROM_FORMAT_Z64: return "Z64 (big-endian/ABCD)";
        case ROM_FORMAT_V64: return "V64 (byte-swapped/BADC)";
        case ROM_FORMAT_N64: return "N64 (little-endian/DCBA)";
        default: return "Unknown";
    }
}

// Function to get format extension
static const char *get_format_extension(rom_format_t format) {
    switch (format) {
        case ROM_FORMAT_Z64: return "z64";
        case ROM_FORMAT_V64: return "v64";
        case ROM_FORMAT_N64: return "n64";
        default: return "rom";
    }
}

// Swap every 2 bytes (for V64 <-> Z64 conversion)
static void swap_bytes_16(unsigned char *data, long length) {
    unsigned char tmp;
    long i;
    for (i = 0; i < length; i += 2) {
        if (i + 1 < length) {
            tmp = data[i];
            data[i] = data[i + 1];
            data[i + 1] = tmp;
        }
    }
}

// Reverse every 4 bytes (for N64 <-> Z64 conversion)
static void swap_bytes_32(unsigned char *data, long length) {
    unsigned char tmp;
    long i;
    for (i = 0; i < length; i += 4) {
        if (i + 3 < length) {
            // Swap bytes 0 and 3
            tmp = data[i];
            data[i] = data[i + 3];
            data[i + 3] = tmp;
            // Swap bytes 1 and 2
            tmp = data[i + 1];
            data[i + 1] = data[i + 2];
            data[i + 2] = tmp;
        }
    }
}

// Main conversion function
static int convert_rom_format(unsigned char *data, long length, 
                             rom_format_t from_format, rom_format_t to_format) {
    if (from_format == to_format) {
        INFO("ROM is already in target format\n");
        return 0;
    }
    
    INFO("Converting from %s to %s\n", 
         get_format_name(from_format), get_format_name(to_format));
    
    // Simple conversion logic based on format differences
    if ((from_format == ROM_FORMAT_Z64 && to_format == ROM_FORMAT_V64) ||
        (from_format == ROM_FORMAT_V64 && to_format == ROM_FORMAT_Z64)) {
        // Z64 <-> V64: swap every 2 bytes
        swap_bytes_16(data, length);
    } 
    else if ((from_format == ROM_FORMAT_Z64 && to_format == ROM_FORMAT_N64) ||
             (from_format == ROM_FORMAT_N64 && to_format == ROM_FORMAT_Z64)) {
        // Z64 <-> N64: reverse every 4 bytes
        swap_bytes_32(data, length);
    }
    else if (from_format == ROM_FORMAT_V64 && to_format == ROM_FORMAT_N64) {
        // V64 -> N64: first convert V64 to Z64, then Z64 to N64
        swap_bytes_16(data, length);  // V64 -> Z64
        swap_bytes_32(data, length);  // Z64 -> N64
    }
    else if (from_format == ROM_FORMAT_N64 && to_format == ROM_FORMAT_V64) {
        // N64 -> V64: first convert N64 to Z64, then Z64 to V64
        swap_bytes_32(data, length);  // N64 -> Z64
        swap_bytes_16(data, length);  // Z64 -> V64
    }
    
    return 0;
}

// Parse format string to enum
static rom_format_t parse_format_string(const char *format_str) {
    if (!strcasecmp(format_str, "z64") || !strcasecmp(format_str, "big") || 
        !strcasecmp(format_str, "abcd")) {
        return ROM_FORMAT_Z64;
    } else if (!strcasecmp(format_str, "v64") || !strcasecmp(format_str, "byte") || 
               !strcasecmp(format_str, "badc")) {
        return ROM_FORMAT_V64;
    } else if (!strcasecmp(format_str, "n64") || !strcasecmp(format_str, "little") || 
               !strcasecmp(format_str, "dcba")) {
        return ROM_FORMAT_N64;
    } else {
        return ROM_FORMAT_UNKNOWN;
    }
}

// Generate output filename if not provided
static void generate_output_filename(const char *input_file, char *output_file, 
                                   rom_format_t target_format) {
    const char *extension = get_format_extension(target_format);
    generate_filename(input_file, output_file, (char*)extension);
}

// Parse command line arguments
static int parse_arguments(int argc, char *argv[], arg_config *config) {
    arg_parser *parser;
    int result;
    char *format_str = NULL;

    parser = argparse_init("n64convert", N64CONVERT_VERSION, 
                          "N64 ROM format converter");
    if (parser == NULL) {
        ERROR("Error: Failed to initialize argument parser\n");
        return -1;
    }

    // Add format flag
    argparse_add_flag(parser, 'f', "format", ARG_TYPE_STRING,
                     "Target format: z64/big/abcd, v64/byte/badc, n64/little/dcba (default: z64)",
                     "FORMAT", &format_str, false, NULL, 0);

    // Add output file flag
    argparse_add_flag(parser, 'o', "output", ARG_TYPE_STRING,
                     "Output file (default: auto-generated based on format)",
                     "FILE", &config->output_file, false, NULL, 0);

    // Add force flag
    argparse_add_flag(parser, 'F', "force", ARG_TYPE_NONE,
                     "Force overwrite existing output file",
                     NULL, &config->force, false, NULL, 0);

    // Add verbose flag
    argparse_add_flag(parser, 'v', "verbose", ARG_TYPE_NONE,
                     "Enable verbose output",
                     NULL, &g_verbosity, false, NULL, 0);

    // Add positional argument for input file
    argparse_add_positional(parser, "INPUT", "Input N64 ROM file",
                           ARG_TYPE_STRING, &config->input_file, true);

    // Parse arguments
    result = argparse_parse(parser, argc, argv);
    
    if (result == 0) {
        if (format_str) {
            config->target_format = parse_format_string(format_str);
            if (config->target_format == ROM_FORMAT_UNKNOWN) {
                ERROR("Error: Unknown format '%s'\n", format_str);
                result = -1;
            }
        }
        // If no format specified, keep the default (ROM_FORMAT_Z64)
    }

    argparse_free(parser);
    return result;
}

int main(int argc, char *argv[]) {
    arg_config config = default_config;
    unsigned char *rom_data = NULL;
    long rom_size;
    rom_format_t detected_format;
    char output_filename[FILENAME_MAX];
    
    // Parse command line arguments
    if (parse_arguments(argc, argv, &config) != 0) {
        return EXIT_FAILURE;
    }
    
    INFO("n64convert v" N64CONVERT_VERSION "\n");
    INFO("Input file: %s\n", config.input_file);
    INFO("Target format: %s\n", get_format_name(config.target_format));
    
    // Read ROM file
    rom_size = read_file(config.input_file, &rom_data);
    if (rom_size < 0) {
        ERROR("Error: Failed to read ROM file '%s'\n", config.input_file);
        return EXIT_FAILURE;
    }
    
    if (rom_size < 64) {
        ERROR("Error: File too small to be a valid N64 ROM (minimum 64 bytes)\n");
        free(rom_data);
        return EXIT_FAILURE;
    }
    
    INFO("ROM size: %ld bytes (%.2f MB)\n", rom_size, rom_size / (1024.0 * 1024.0));
    
    // Detect current ROM format
    detected_format = detect_rom_format(rom_data);
    if (detected_format == ROM_FORMAT_UNKNOWN) {
        ERROR("Error: Unknown or invalid ROM format\n");
        ERROR("Expected N64 ROM magic bytes not found in first 4 bytes\n");
        free(rom_data);
        return EXIT_FAILURE;
    }
    
    INFO("Detected format: %s\n", get_format_name(detected_format));
    
    // Generate output filename if not provided
    if (config.output_file == NULL) {
        generate_output_filename(config.input_file, output_filename, config.target_format);
        config.output_file = output_filename;
    }
    
    INFO("Output file: %s\n", config.output_file);
    
    // Check if output file exists and force flag
    if (!config.force && filesize(config.output_file) >= 0) {
        ERROR("Error: Output file '%s' already exists. Use -F to force overwrite.\n", 
              config.output_file);
        free(rom_data);
        return EXIT_FAILURE;
    }
    
    // Perform conversion
    if (convert_rom_format(rom_data, rom_size, detected_format, config.target_format) != 0) {
        ERROR("Error: ROM conversion failed\n");
        free(rom_data);
        return EXIT_FAILURE;
    }
    
    // Write output file
    if (write_file(config.output_file, rom_data, rom_size) != rom_size) {
        ERROR("Error: Failed to write output file '%s'\n", config.output_file);
        free(rom_data);
        return EXIT_FAILURE;
    }
    
    INFO("Conversion completed successfully\n");
    printf("Converted %s (%s) to %s (%s)\n",
           config.input_file, get_format_name(detected_format),
           config.output_file, get_format_name(config.target_format));
    
    free(rom_data);
    return EXIT_SUCCESS;
}
