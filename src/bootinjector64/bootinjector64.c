#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libn64.h"
#include "utils.h"

#define BOOTINJECTOR64_VERSION "0.1"

// Define the boot section offset in N64 ROMs
#define BOOT_SECTION_OFFSET 0x40
#define MAX_BOOT_SIZE 0xFC0 // 4032 bytes (0x1000 - 0x40)

typedef struct {
    char *rom_file;       // Input ROM file
    char *asm_file;       // Input ASM file
    char *out_file;       // Output ROM file
    char *assembler;      // Path to assembler (default: bass)
    int verbose;          // Verbose output
    int force;            // Force overwrite even if boot code is too large
} boot_config;

static void print_usage(void) {
  ERROR("Usage: bootinjector64 [options] ROM ASM_FILE [ROM_OUT]\n"
        "\n"
        "bootinjector64 v" BOOTINJECTOR64_VERSION ": N64 Boot Section Assembly Injector\n"
        "\n"
        "File arguments:\n"
        " ROM          Input ROM file\n"
        " ASM_FILE     Input MIPS assembly file to inject\n"
        " ROM_OUT      Output ROM file (default: overwrites input ROM)\n"
        "\n"
        "Options:\n"
        " -a ASSEMBLER Path to assembler (default: bass)\n"
        " -f           Force overwrite even if boot code is too large\n"
        " -v           Verbose output\n"
        " -h           Show this help message\n");
}

static int parse_args(int argc, char *argv[], boot_config *config) {
    int opt;
    
    // Set default values
    config->verbose = 0;
    config->force = 0;
    config->assembler = "bass";
    
    // Parse options
    while ((opt = getopt(argc, argv, "a:fvh")) != -1) {
        switch (opt) {
            case 'a':
                config->assembler = optarg;
                break;
            case 'f':
                config->force = 1;
                break;
            case 'v':
                config->verbose = 1;
                break;
            case 'h':
                print_usage();
                return 0;
            default:
                print_usage();
                return 0;
        }
    }
    
    // Check if we have the minimum required arguments
    if (argc - optind < 2) {
        print_usage();
        return 0;
    }
    
    // Set input ROM file
    config->rom_file = argv[optind];
    
    // Set input ASM file
    config->asm_file = argv[optind + 1];
    
    // Set output ROM file (or use input ROM if not specified)
    if (argc - optind > 2) {
        config->out_file = argv[optind + 2];
    } else {
        config->out_file = argv[optind];
    }
    
    return 1;
}

// Compile the MIPS assembly file to binary
static int compile_assembly(boot_config *config, unsigned char **binary, long *bin_size) {
    char temp_bin_file[FILENAME_MAX];
    char cmd[FILENAME_MAX * 3];
    int result;
    
    // Create a temporary filename for the compiled binary
    sprintf(temp_bin_file, "bootcode_temp.bin");
    
    // Build the assembler command
    sprintf(cmd, "%s -o %s %s", config->assembler, temp_bin_file, config->asm_file);
    
    if (config->verbose) {
        INFO("Executing: %s\n", cmd);
    }
    
    // Execute the assembler
    result = system(cmd);
    if (result != 0) {
        ERROR("Failed to assemble the MIPS code. Make sure %s is installed.\n", config->assembler);
        return 0;
    }
    
    // Read the compiled binary
    *bin_size = read_file(temp_bin_file, binary);
    if (*bin_size <= 0) {
        ERROR("Failed to read compiled binary.\n");
        return 0;
    }
    
    // Remove the temporary file
    unlink(temp_bin_file);
    
    if (config->verbose) {
        INFO("Successfully compiled MIPS assembly to binary (%ld bytes).\n", *bin_size);
    }
    
    // Check if binary is too large for boot section
    if (*bin_size > MAX_BOOT_SIZE) {
        if (!config->force) {
            ERROR("Compiled boot code is too large (%ld bytes). Max size is %d bytes.\n", 
                  *bin_size, MAX_BOOT_SIZE);
            ERROR("Use -f to force injection anyway (may corrupt ROM).\n");
            free(*binary);
            return 0;
        } else {
            ERROR("Compiled boot code is too large (%ld bytes). Max size is %d bytes.\n", 
                 *bin_size, MAX_BOOT_SIZE);
            ERROR("Forcing injection as requested. This may corrupt the ROM.\n");
        }
    }
    
    return 1;
}

int main(int argc, char *argv[]) {
    boot_config config;
    unsigned char *rom_data;
    unsigned char *boot_binary;
    long rom_length;
    long binary_length;
    long write_length;
    
    // Parse command line arguments
    if (!parse_args(argc, argv, &config)) {
        return EXIT_FAILURE;
    }
    
    // Read the ROM file
    rom_length = read_file(config.rom_file, &rom_data);
    if (rom_length < 0) {
        ERROR("Error reading input ROM file \"%s\"\n", config.rom_file);
        return EXIT_FAILURE;
    }
    
    // Check ROM type
    rom_type type = sm64_rom_type(rom_data, rom_length);
    if (type == ROM_INVALID) {
        ERROR("Input file does not appear to be a valid N64 ROM.\n");
        free(rom_data);
        return EXIT_FAILURE;
    }
    
    // Convert to big-endian if needed
    if (type == ROM_SM64_BS) {
        INFO("ROM is in byte-swapped format. Converting to big-endian.\n");
        swap_bytes(rom_data, rom_length);
        type = ROM_SM64_BE;
    } else if (type == ROM_SM64_LE) {
        INFO("ROM is in little-endian format. Converting to big-endian.\n");
        reverse_endian(rom_data, rom_length);
        type = ROM_SM64_BE;
    }
    
    // Compile the assembly file
    if (!compile_assembly(&config, &boot_binary, &binary_length)) {
        free(rom_data);
        return EXIT_FAILURE;
    }
    
    // Inject the compiled binary into the boot section
    if (config.verbose) {
        INFO("Injecting %ld bytes of boot code at offset 0x%X\n", binary_length, BOOT_SECTION_OFFSET);
    }
    
    // Copy the compiled binary to the boot section
    memcpy(rom_data + BOOT_SECTION_OFFSET, boot_binary, binary_length);
    
    // Free the boot binary
    free(boot_binary);
    
    // Update the ROM checksums
    if (config.verbose) {
        INFO("Updating ROM checksums\n");
    }
    sm64_update_checksums(rom_data);
    
    // Write the modified ROM
    write_length = write_file(config.out_file, rom_data, rom_length);
    
    // Free the ROM data
    free(rom_data);
    
    if (write_length != rom_length) {
        ERROR("Error writing to output ROM file \"%s\"\n", config.out_file);
        return EXIT_FAILURE;
    }
    
    INFO("Successfully injected boot code and updated checksums in \"%s\"\n", config.out_file);
    
    return EXIT_SUCCESS;
}
