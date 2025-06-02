#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "argparse.h"
#include "libn64.h"
#include "utils.h"

#define N64SYMBOLS_VERSION "1.0"

typedef struct {
    char *rom_file;
    char *output_file;
    bool verbose;
    bool extract_functions;
    bool extract_jumptables;
    bool extract_strings;
    bool extract_all;
    unsigned int base_address;
    unsigned int min_function_size;
    unsigned int max_string_length;
} arg_config;

static arg_config default_config = {
    NULL,       // ROM filename
    NULL,       // output filename  
    false,      // verbose
    false,      // extract_functions
    false,      // extract_jumptables
    false,      // extract_strings
    false,      // extract_all
    0x80000000, // base_address
    16,         // min_function_size
    256,        // max_string_length
};

typedef struct {
    unsigned int address;
    unsigned int size;
    char name[64];
    char type[16];
} symbol_entry;

typedef struct {
    symbol_entry *symbols;
    int count;
    int capacity;
} symbol_table;



static symbol_table *symbol_table_create(void) {
    symbol_table *table = malloc(sizeof(symbol_table));
    if (!table) return NULL;
    
    table->capacity = 1000;
    table->symbols = malloc(table->capacity * sizeof(symbol_entry));
    if (!table->symbols) {
        free(table);
        return NULL;
    }
    
    table->count = 0;
    return table;
}

static void symbol_table_free(symbol_table *table) {
    if (table) {
        free(table->symbols);
        free(table);
    }
}

static int symbol_table_add(symbol_table *table, unsigned int address, 
                           unsigned int size, const char *name, const char *type) {
    if (table->count >= table->capacity) {
        table->capacity *= 2;
        symbol_entry *new_symbols = realloc(table->symbols, 
                                          table->capacity * sizeof(symbol_entry));
        if (!new_symbols) return -1;
        table->symbols = new_symbols;
    }
    
    symbol_entry *entry = &table->symbols[table->count];
    entry->address = address;
    entry->size = size;
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    strncpy(entry->type, type, sizeof(entry->type) - 1);
    entry->type[sizeof(entry->type) - 1] = '\0';
    
    table->count++;
    return 0;
}

static bool is_valid_string_char(unsigned char c) {
    return (c >= 0x20 && c <= 0x7E) || c == '\t' || c == '\n' || c == '\r';
}

static bool is_printable_string(const unsigned char *data, int length) {
    if (length < 4) return false; // Too short to be useful
    
    int printable_count = 0;
    for (int i = 0; i < length; i++) {
        if (is_valid_string_char(data[i])) {
            printable_count++;
        } else if (data[i] == 0) {
            // Null terminator is okay at the end
            if (i == length - 1 || i >= 3) {
                break;
            }
            return false;
        } else {
            return false;
        }
    }
    
    // Must be at least 75% printable characters
    return (printable_count * 100 / length) >= 75;
}

static void extract_strings(unsigned char *rom_data, long rom_size, 
                           symbol_table *table, arg_config *config) {
    const int min_string_length = 4;
    
    if (config->verbose) {
        INFO("Scanning for strings (min length: %d, max length: %d)...\n", 
             min_string_length, config->max_string_length);
    }
    
    for (long offset = 0; offset < rom_size - min_string_length; offset++) {
        // Look for potential string start
        if (!is_valid_string_char(rom_data[offset])) continue;
        
        // Find end of string
        int length = 0;
        while (offset + length < rom_size && 
               length < (int)config->max_string_length &&
               rom_data[offset + length] != 0) {
            
            if (!is_valid_string_char(rom_data[offset + length])) {
                break;
            }
            length++;
        }
        
        // Check if we found a null terminator
        if (offset + length < rom_size && 
            rom_data[offset + length] == 0 &&
            length >= min_string_length &&
            is_printable_string(rom_data + offset, length)) {
            
            char name[128];
            char safe_preview[32];
            int preview_len = length > 20 ? 20 : length;
            
            // Create safe preview (replace non-printable chars)
            for (int i = 0; i < preview_len; i++) {
                unsigned char c = rom_data[offset + i];
                safe_preview[i] = (c >= 0x20 && c <= 0x7E) ? c : '?';
            }
            safe_preview[preview_len] = '\0';
            
            snprintf(name, sizeof(name), "str_%08lX_%s%s", 
                    (unsigned long)(config->base_address + offset),
                    safe_preview,
                    length > 20 ? "..." : "");
            
            // Remove spaces and special chars from name
            for (char *p = name; *p; p++) {
                if (!isalnum(*p) && *p != '_') *p = '_';
            }
            
            symbol_table_add(table, config->base_address + offset, 
                           length + 1, name, "string");
            
            if (config->verbose && table->count % 100 == 0) {
                INFO("Found %d strings so far...\n", table->count);
            }
            
            // Skip past this string
            offset += length;
        }
    }
    
    if (config->verbose) {
        INFO("String extraction complete. Found %d strings.\n", table->count);
    }
}

static bool looks_like_mips_instruction(unsigned int word) {
    // Check for common MIPS instruction patterns
    unsigned int opcode = (word >> 26) & 0x3F;
    
    // Common MIPS opcodes
    switch (opcode) {
        case 0x00: // SPECIAL (R-type instructions)
        case 0x01: // REGIMM  
        case 0x02: // J
        case 0x03: // JAL
        case 0x04: // BEQ
        case 0x05: // BNE
        case 0x06: // BLEZ
        case 0x07: // BGTZ
        case 0x08: // ADDI
        case 0x09: // ADDIU
        case 0x0A: // SLTI
        case 0x0B: // SLTIU
        case 0x0C: // ANDI
        case 0x0D: // ORI
        case 0x0E: // XORI
        case 0x0F: // LUI
        case 0x20: // LB
        case 0x21: // LH
        case 0x23: // LW
        case 0x24: // LBU
        case 0x25: // LHU
        case 0x28: // SB
        case 0x29: // SH
        case 0x2B: // SW
            return true;
        default:
            return false;
    }
}

static void extract_functions(unsigned char *rom_data, long rom_size, 
                             symbol_table *table, arg_config *config) {
    if (config->verbose) {
        INFO("Scanning for functions (min size: %d bytes)...\n", config->min_function_size);
    }
    
    int function_count = 0;
    
    for (long offset = 0; offset < rom_size - config->min_function_size; offset += 4) {
        // Look for potential function start
        unsigned int word = read_u32_be(rom_data + offset);
        
        if (!looks_like_mips_instruction(word)) continue;
        
        // Scan ahead to see if this looks like a function
        int instruction_count = 0;
        long scan_offset = offset;
        bool found_return = false;
        
        while (scan_offset < rom_size - 4 && 
               scan_offset < offset + 1024 && // Max function scan size
               instruction_count < 256) {
            
            unsigned int inst = read_u32_be(rom_data + scan_offset);
            
            if (!looks_like_mips_instruction(inst)) {
                break;
            }
            
            instruction_count++;
            
            // Check for return instruction (JR RA)
            if ((inst & 0xFFFFFFFF) == 0x03E00008) { // jr $ra
                found_return = true;
                scan_offset += 4;
                break;
            }
            
            scan_offset += 4;
        }
        
        // If we found enough instructions and a return, it's likely a function
        if (instruction_count >= (int)(config->min_function_size / 4) && found_return) {
            char name[64];
            snprintf(name, sizeof(name), "func_%08lX", (unsigned long)(config->base_address + offset));
            
            symbol_table_add(table, config->base_address + offset, 
                           scan_offset - offset, name, "function");
            
            function_count++;
            
            if (config->verbose && function_count % 50 == 0) {
                INFO("Found %d functions so far...\n", function_count);
            }
            
            // Skip past this function
            offset = scan_offset - 4; // -4 because loop will add 4
        }
    }
    
    if (config->verbose) {
        INFO("Function extraction complete. Found %d functions.\n", function_count);
    }
}

static void extract_jumptables(unsigned char *rom_data, long rom_size, 
                              symbol_table *table, arg_config *config) {
    if (config->verbose) {
        INFO("Scanning for jump tables...\n");
    }
    
    int jumptable_count = 0;
    
    // Look for sequences of addresses that could be jump tables
    for (long offset = 0; offset < rom_size - 16; offset += 4) {
        unsigned int addr1 = read_u32_be(rom_data + offset);
        unsigned int addr2 = read_u32_be(rom_data + offset + 4);
        unsigned int addr3 = read_u32_be(rom_data + offset + 8);
        unsigned int addr4 = read_u32_be(rom_data + offset + 12);
        
        // Check if these look like N64 addresses in sequence
        if ((addr1 >= 0x80000000 && addr1 < 0x80800000) &&
            (addr2 >= 0x80000000 && addr2 < 0x80800000) &&
            (addr3 >= 0x80000000 && addr3 < 0x80800000) &&
            (addr4 >= 0x80000000 && addr4 < 0x80800000)) {
            
            // Count how many consecutive addresses we have
            int count = 4;
            long scan_offset = offset + 16;
            
            while (scan_offset < rom_size - 4 && count < 64) {
                unsigned int addr = read_u32_be(rom_data + scan_offset);
                
                if (addr < 0x80000000 || addr >= 0x80800000) {
                    break;
                }
                
                count++;
                scan_offset += 4;
            }
            
            // If we found a reasonable number of addresses, call it a jump table
            if (count >= 4) {
                char name[64];
                snprintf(name, sizeof(name), "jtbl_%08lX", (unsigned long)(config->base_address + offset));
                
                symbol_table_add(table, config->base_address + offset, 
                               count * 4, name, "jumptable");
                
                jumptable_count++;
                
                if (config->verbose && jumptable_count % 10 == 0) {
                    INFO("Found %d jump tables so far...\n", jumptable_count);
                }
                
                // Skip past this jump table
                offset = scan_offset - 4; // -4 because loop will add 4
            }
        }
    }
    
    if (config->verbose) {
        INFO("Jump table extraction complete. Found %d jump tables.\n", jumptable_count);
    }
}

static int symbol_compare(const void *a, const void *b) {
    const symbol_entry *sa = (const symbol_entry *)a;
    const symbol_entry *sb = (const symbol_entry *)b;
    
    if (sa->address < sb->address) return -1;
    if (sa->address > sb->address) return 1;
    return 0;
}

static void write_symbol_table(symbol_table *table, const char *filename, bool verbose) {
    if (table->count == 0) {
        if (verbose) {
            INFO("No symbols found, not creating output file.\n");
        }
        return;
    }
    
    // Sort symbols by address
    qsort(table->symbols, table->count, sizeof(symbol_entry), symbol_compare);
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        ERROR("Failed to create output file: %s\n", filename);
        return;
    }
    
    // Write header
    fprintf(fp, "# N64 Symbol Table\n");
    fprintf(fp, "# Generated by n64symbols v" N64SYMBOLS_VERSION "\n");
    fprintf(fp, "# Format: ADDRESS SIZE TYPE NAME\n\n");
    
    // Write symbols
    for (int i = 0; i < table->count; i++) {
        symbol_entry *sym = &table->symbols[i];
        fprintf(fp, "%08X %08X %-10s %s\n", 
                sym->address, sym->size, sym->type, sym->name);
    }
    
    fclose(fp);
    
    if (verbose) {
        INFO("Symbol table written to: %s\n", filename);
        INFO("Total symbols: %d\n", table->count);
    }
}

static int parse_arguments(int argc, char *argv[], arg_config *config) {
    arg_parser *parser;
    int result;

    // Initialize the argument parser
    parser = argparse_init("n64symbols", N64SYMBOLS_VERSION, "N64 ROM symbol table generator");
    if (parser == NULL) {
        ERROR("Error: Failed to initialize argument parser\n");
        return -1;
    }

    // Add flag arguments
    argparse_add_flag(parser, 'v', "verbose", ARG_TYPE_NONE,
                      "enable verbose output", NULL, &config->verbose, false, NULL, 0);

    argparse_add_flag(parser, 'f', "functions", ARG_TYPE_NONE,
                      "extract function symbols", NULL, &config->extract_functions, false, NULL, 0);

    argparse_add_flag(parser, 'j', "jumptables", ARG_TYPE_NONE,
                      "extract jump table symbols", NULL, &config->extract_jumptables, false, NULL, 0);

    argparse_add_flag(parser, 's', "strings", ARG_TYPE_NONE,
                      "extract string symbols", NULL, &config->extract_strings, false, NULL, 0);

    argparse_add_flag(parser, 'a', "all", ARG_TYPE_NONE,
                      "extract all symbol types", NULL, &config->extract_all, false, NULL, 0);

    argparse_add_flag(parser, 'b', "base", ARG_TYPE_UINT,
                      "base address for symbol calculation (default: 0x80000000)", "ADDR",
                      &config->base_address, false, NULL, 0);

    argparse_add_flag(parser, 0, "min-func", ARG_TYPE_UINT,
                      "minimum function size in bytes (default: 16)", "SIZE",
                      &config->min_function_size, false, NULL, 0);

    argparse_add_flag(parser, 0, "max-str", ARG_TYPE_UINT,
                      "maximum string length (default: 256)", "LEN",
                      &config->max_string_length, false, NULL, 0);

    // Add positional arguments
    argparse_add_positional(parser, "ROM", "N64 ROM file to analyze",
                            ARG_TYPE_STRING, &config->rom_file, true);

    argparse_add_positional(parser, "OUTPUT", "output symbol file (optional)",
                            ARG_TYPE_STRING, &config->output_file, false);

    // Parse the arguments
    result = argparse_parse(parser, argc, argv);

    // Free the parser
    argparse_free(parser);

    return result;
}

int main(int argc, char *argv[]) {
    arg_config config = default_config;
    unsigned char *rom_data;
    long rom_size;
    symbol_table *table;
    char default_output[512];
    
    if (parse_arguments(argc, argv, &config) != 0) {
        return EXIT_FAILURE;
    }
    
    // Set default output filename if not provided
    if (!config.output_file) {
        snprintf(default_output, sizeof(default_output), "%s.sym", config.rom_file);
        config.output_file = default_output;
    }
    
    // If no extraction types specified, default to all
    if (!config.extract_functions && !config.extract_jumptables && 
        !config.extract_strings && !config.extract_all) {
        config.extract_all = true;
    }
    
    if (config.extract_all) {
        config.extract_functions = true;
        config.extract_jumptables = true;
        config.extract_strings = true;
    }
    
    if (config.verbose) {
        INFO("n64symbols v" N64SYMBOLS_VERSION "\n");
        INFO("Analyzing ROM: %s\n", config.rom_file);
        INFO("Output file: %s\n", config.output_file);
        INFO("Base address: 0x%08X\n", config.base_address);
        
        INFO("Extraction options:\n");
        if (config.extract_functions) INFO("  - Functions (min size: %d bytes)\n", config.min_function_size);
        if (config.extract_jumptables) INFO("  - Jump tables\n");
        if (config.extract_strings) INFO("  - Strings (max length: %d)\n", config.max_string_length);
    }
    
    // Read ROM file
    rom_size = read_file(config.rom_file, &rom_data);
    if (rom_size < 0) {
        ERROR("Error reading ROM file \"%s\"\n", config.rom_file);
        return EXIT_FAILURE;
    }
    
    // Create symbol table
    table = symbol_table_create();
    if (!table) {
        ERROR("Failed to create symbol table\n");
        free(rom_data);
        return EXIT_FAILURE;
    }
    
    // Extract symbols based on configuration
    if (config.extract_strings) {
        extract_strings(rom_data, rom_size, table, &config);
    }
    
    if (config.extract_functions) {
        extract_functions(rom_data, rom_size, table, &config);
    }
    
    if (config.extract_jumptables) {
        extract_jumptables(rom_data, rom_size, table, &config);
    }
    
    // Write output file
    write_symbol_table(table, config.output_file, config.verbose);
    
    if (config.verbose) {
        INFO("Symbol extraction complete.\n");
    }
    
    // Cleanup
    symbol_table_free(table);
    free(rom_data);
    
    return EXIT_SUCCESS;
}
