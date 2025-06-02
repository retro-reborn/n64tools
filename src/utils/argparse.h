#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file argparse.h
 * @brief Unified command-line argument parsing for n64tools
 */

/* Argument types */
typedef enum {
  ARG_TYPE_NONE,   // Flag without value (e.g., -v)
  ARG_TYPE_INT,    // Integer value
  ARG_TYPE_UINT,   // Unsigned integer value
  ARG_TYPE_FLOAT,  // Float value
  ARG_TYPE_STRING, // String value
  ARG_TYPE_ENUM,   // Enumerated value
} arg_type;

/* Argument definition */
typedef struct {
  char short_flag;          // Single character flag (e.g., 'v' for -v)
  const char *long_flag;    // Long form flag (e.g., "verbose" for --verbose)
  arg_type type;            // Type of argument
  const char *help;         // Help text
  const char *meta;         // Metavar (e.g., "FILE", "OFFSET")
  void *dest;               // Pointer to store result (int*, char**, etc.)
  bool required;            // Is this argument required?
  bool processed;           // Has this argument been processed?
  const char **enum_values; // Valid values for enum types
  int enum_count;           // Number of valid enum values
} arg_def;

/* Positional arguments */
typedef struct {
  const char *name; // Name of the positional argument
  const char *help; // Help text
  arg_type type;    // Type of argument
  void *dest;       // Pointer to store result
  bool required;    // Is this argument required?
  bool processed;   // Has this argument been processed?
} pos_arg_def;

/* Argument parser context */
typedef struct {
  const char *prog_name;        // Program name
  const char *prog_version;     // Program version
  const char *prog_description; // Program description

  arg_def *flags; // Array of flag definitions
  int flag_count; // Number of flags

  pos_arg_def *pos_args; // Array of positional argument definitions
  int pos_arg_count;     // Number of positional arguments

  const char *usage_suffix; // Additional usage text (e.g., "FILE [OUTPUT]")
} arg_parser;

/**
 * Initialize an argument parser
 *
 * @param prog_name Program name
 * @param prog_version Program version
 * @param prog_description Brief description of the program
 * @return A new argument parser
 */
arg_parser *argparse_init(const char *prog_name, const char *prog_version,
                          const char *prog_description);

/**
 * Free resources used by an argument parser
 *
 * @param parser The parser to free
 */
void argparse_free(arg_parser *parser);

/**
 * Add a flag argument to the parser
 *
 * @param parser The parser to add the flag to
 * @param short_flag Single character flag (e.g., 'v' for -v)
 * @param long_flag Long form flag (e.g., "verbose" for --verbose), can be NULL
 * @param type Type of argument
 * @param help Help text
 * @param meta Metavar for the value (e.g., "FILE"), can be NULL for
 * ARG_TYPE_NONE
 * @param dest Pointer to store result (bool* for NONE, int* for INT, etc.)
 * @param required Whether this flag is required
 * @param enum_values Valid values for enum types, NULL for other types
 * @param enum_count Number of valid enum values, 0 for other types
 * @return 0 on success, -1 on failure
 */
int argparse_add_flag(arg_parser *parser, char short_flag,
                      const char *long_flag, arg_type type, const char *help,
                      const char *meta, void *dest, bool required,
                      const char **enum_values, int enum_count);

/**
 * Add a positional argument to the parser
 *
 * @param parser The parser to add the argument to
 * @param name Name of the positional argument
 * @param help Help text
 * @param type Type of argument
 * @param dest Pointer to store result
 * @param required Whether this argument is required
 * @return 0 on success, -1 on failure
 */
int argparse_add_positional(arg_parser *parser, const char *name,
                            const char *help, arg_type type, void *dest,
                            bool required);

/**
 * Set additional usage text suffix
 *
 * @param parser The parser to modify
 * @param usage_suffix Additional usage text (e.g., "FILE [OUTPUT]")
 */
void argparse_set_usage_suffix(arg_parser *parser, const char *usage_suffix);

/**
 * Parse command line arguments
 *
 * @param parser The parser to use
 * @param argc Argument count from main()
 * @param argv Argument values from main()
 * @return 0 on success, -1 on failure
 */
int argparse_parse(arg_parser *parser, int argc, char *argv[]);

/**
 * Print usage and help information
 *
 * @param parser The parser to print help for
 * @param out File to print to (e.g., stdout, stderr)
 */
void argparse_print_help(arg_parser *parser, FILE *out);

/**
 * Print version information
 *
 * @param parser The parser to print version for
 * @param out File to print to (e.g., stdout, stderr)
 */
void argparse_print_version(arg_parser *parser, FILE *out);

#endif /* ARGPARSE_H */
