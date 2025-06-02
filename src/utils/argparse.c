#include "argparse.h"
#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file argparse.c
 * @brief Implementation of unified command-line argument parsing for n64tools
 */

/*
 * Helper function to check if a string is a valid number (currently unused)
 * Commented out to avoid unused function warning
 *
static bool is_number(const char *str, arg_type type) {
    char *endptr;

    if (str == NULL || *str == '\0') {
        return false;
    }

    if (type == ARG_TYPE_INT) {
        // Allow negative numbers for INT
        if (*str == '-') {
            str++;
        }
    }

    // Check if all remaining characters are digits
    while (*str) {
        if (type == ARG_TYPE_FLOAT && (*str == '.' || *str == 'e' || *str == 'E'
|| *str == '+' || *str == '-')) {
            // These characters are valid in floating point numbers
            str++;
            continue;
        }

        if (!isdigit((unsigned char)*str)) {
            return false;
        }
        str++;
    }

    return true;
}
*/

/* Helper function to convert string value to the appropriate type */
static int convert_value(const char *value, arg_type type, void *dest,
                         const char **enum_values, int enum_count) {
  if (value == NULL) {
    return -1;
  }

  switch (type) {
  case ARG_TYPE_NONE:
    /* Flag arguments don't have values */
    *(bool *)dest = true;
    break;

  case ARG_TYPE_INT:
    *(int *)dest = strtol(value, NULL, 0);
    break;

  case ARG_TYPE_UINT:
    *(unsigned int *)dest = strtoul(value, NULL, 0);
    break;

  case ARG_TYPE_FLOAT:
    *(float *)dest = strtof(value, NULL);
    break;

  case ARG_TYPE_STRING:
    *(char **)dest = strdup(value);
    break;

  case ARG_TYPE_ENUM: {
    int i;
    bool found = false;

    for (i = 0; i < enum_count; i++) {
      if (strcasecmp(value, enum_values[i]) == 0) {
        *(int *)dest = i;
        found = true;
        break;
      }
    }

    if (!found) {
      return -1;
    }
    break;
  }

  default:
    return -1;
  }

  return 0;
}

/* Initialize an argument parser */
arg_parser *argparse_init(const char *prog_name, const char *prog_version,
                          const char *prog_description) {
  arg_parser *parser = malloc(sizeof(arg_parser));
  if (parser == NULL) {
    return NULL;
  }

  parser->prog_name = prog_name;
  parser->prog_version = prog_version;
  parser->prog_description = prog_description;

  parser->flags = NULL;
  parser->flag_count = 0;

  parser->pos_args = NULL;
  parser->pos_arg_count = 0;

  parser->usage_suffix = NULL;

  return parser;
}

/* Free resources used by an argument parser */
void argparse_free(arg_parser *parser) {
  if (parser == NULL) {
    return;
  }

  if (parser->flags) {
    free(parser->flags);
  }

  if (parser->pos_args) {
    free(parser->pos_args);
  }

  free(parser);
}

/* Add a flag argument to the parser */
int argparse_add_flag(arg_parser *parser, char short_flag,
                      const char *long_flag, arg_type type, const char *help,
                      const char *meta, void *dest, bool required,
                      const char **enum_values, int enum_count) {
  arg_def *new_flags;

  if (parser == NULL || help == NULL || dest == NULL) {
    return -1;
  }

  /* If this is a value flag, meta must be provided */
  if (type != ARG_TYPE_NONE && meta == NULL) {
    return -1;
  }

  /* If this is an enum flag, enum_values must be provided */
  if (type == ARG_TYPE_ENUM && (enum_values == NULL || enum_count <= 0)) {
    return -1;
  }

  /* Allocate or reallocate the flags array */
  new_flags =
      realloc(parser->flags, (parser->flag_count + 1) * sizeof(arg_def));
  if (new_flags == NULL) {
    return -1;
  }

  parser->flags = new_flags;

  /* Set up the new flag */
  parser->flags[parser->flag_count].short_flag = short_flag;
  parser->flags[parser->flag_count].long_flag = long_flag;
  parser->flags[parser->flag_count].type = type;
  parser->flags[parser->flag_count].help = help;
  parser->flags[parser->flag_count].meta = meta;
  parser->flags[parser->flag_count].dest = dest;
  parser->flags[parser->flag_count].required = required;
  parser->flags[parser->flag_count].processed = false;
  parser->flags[parser->flag_count].enum_values = enum_values;
  parser->flags[parser->flag_count].enum_count = enum_count;

  parser->flag_count++;

  return 0;
}

/* Add a positional argument to the parser */
int argparse_add_positional(arg_parser *parser, const char *name,
                            const char *help, arg_type type, void *dest,
                            bool required) {
  pos_arg_def *new_pos_args;

  if (parser == NULL || name == NULL || help == NULL || dest == NULL) {
    return -1;
  }

  /* ARG_TYPE_NONE is invalid for positional arguments */
  if (type == ARG_TYPE_NONE) {
    return -1;
  }

  /* Allocate or reallocate the positional arguments array */
  new_pos_args = realloc(parser->pos_args,
                         (parser->pos_arg_count + 1) * sizeof(pos_arg_def));
  if (new_pos_args == NULL) {
    return -1;
  }

  parser->pos_args = new_pos_args;

  /* Set up the new positional argument */
  parser->pos_args[parser->pos_arg_count].name = name;
  parser->pos_args[parser->pos_arg_count].help = help;
  parser->pos_args[parser->pos_arg_count].type = type;
  parser->pos_args[parser->pos_arg_count].dest = dest;
  parser->pos_args[parser->pos_arg_count].required = required;
  parser->pos_args[parser->pos_arg_count].processed = false;

  parser->pos_arg_count++;

  return 0;
}

/* Set additional usage text suffix */
void argparse_set_usage_suffix(arg_parser *parser, const char *usage_suffix) {
  if (parser != NULL) {
    parser->usage_suffix = usage_suffix;
  }
}

/* Parse command line arguments */
int argparse_parse(arg_parser *parser, int argc, char *argv[]) {
  int i, j;
  int pos_arg_index = 0;

  if (parser == NULL || argc < 1 || argv == NULL) {
    return -1;
  }

  /* Skip the program name */
  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      /* Flag argument */
      if (argv[i][1] == '-') {
        /* Long flag (--flag) */
        char *long_flag = argv[i] + 2;
        char *equals = strchr(long_flag, '=');
        char *flag_value = NULL;

        /* Handle --flag=value format */
        if (equals != NULL) {
          *equals = '\0';
          flag_value = equals + 1;
        }

        /* Special case: --help */
        if (strcmp(long_flag, "help") == 0) {
          argparse_print_help(parser, stdout);
          exit(0);
        }

        /* Special case: --version */
        if (strcmp(long_flag, "version") == 0) {
          argparse_print_version(parser, stdout);
          exit(0);
        }

        /* Find the matching flag */
        bool found = false;
        for (j = 0; j < parser->flag_count; j++) {
          if (parser->flags[j].long_flag != NULL &&
              strcmp(parser->flags[j].long_flag, long_flag) == 0) {

            if (parser->flags[j].type == ARG_TYPE_NONE) {
              /* Flag without value */
              *(bool *)parser->flags[j].dest = true;
              parser->flags[j].processed = true;
            } else {
              /* Flag with value */
              if (flag_value == NULL) {
                /* Take the next argument as the value */
                i++;
                if (i >= argc) {
                  ERROR("Error: missing value for --%s\n", long_flag);
                  return -1;
                }
                flag_value = argv[i];
              }

              if (convert_value(flag_value, parser->flags[j].type,
                                parser->flags[j].dest,
                                parser->flags[j].enum_values,
                                parser->flags[j].enum_count) != 0) {
                ERROR("Error: invalid value for --%s: %s\n", long_flag,
                      flag_value);
                return -1;
              }
              parser->flags[j].processed = true;
            }

            found = true;
            break;
          }
        }

        if (!found) {
          ERROR("Error: unknown option --%s\n", long_flag);
          return -1;
        }

        /* Restore the equals sign if we modified the string */
        if (equals != NULL) {
          *equals = '=';
        }
      } else {
        /* Short flag (-f) */
        char short_flag = argv[i][1];

        /* Special case: -h */
        if (short_flag == 'h') {
          argparse_print_help(parser, stdout);
          exit(0);
        }

        /* Special case: -V */
        if (short_flag == 'V') {
          argparse_print_version(parser, stdout);
          exit(0);
        }

        /* Find the matching flag */
        bool found = false;
        for (j = 0; j < parser->flag_count; j++) {
          if (parser->flags[j].short_flag == short_flag) {
            if (parser->flags[j].type == ARG_TYPE_NONE) {
              /* Flag without value */
              *(bool *)parser->flags[j].dest = true;
              parser->flags[j].processed = true;
            } else {
              /* Flag with value */
              char *flag_value = NULL;

              /* Check if the value is part of the flag (-fvalue) */
              if (argv[i][2] != '\0') {
                flag_value = argv[i] + 2;
              } else {
                /* Take the next argument as the value */
                i++;
                if (i >= argc) {
                  ERROR("Error: missing value for -%c\n", short_flag);
                  return -1;
                }
                flag_value = argv[i];
              }

              if (convert_value(flag_value, parser->flags[j].type,
                                parser->flags[j].dest,
                                parser->flags[j].enum_values,
                                parser->flags[j].enum_count) != 0) {
                ERROR("Error: invalid value for -%c: %s\n", short_flag,
                      flag_value);
                return -1;
              }
              parser->flags[j].processed = true;
            }

            found = true;
            break;
          }
        }

        if (!found) {
          ERROR("Error: unknown option -%c\n", short_flag);
          return -1;
        }
      }
    } else {
      /* Positional argument */
      if (pos_arg_index < parser->pos_arg_count) {
        if (convert_value(argv[i], parser->pos_args[pos_arg_index].type,
                          parser->pos_args[pos_arg_index].dest, NULL, 0) != 0) {
          ERROR("Error: invalid value for %s: %s\n",
                parser->pos_args[pos_arg_index].name, argv[i]);
          return -1;
        }
        parser->pos_args[pos_arg_index].processed = true;
        pos_arg_index++;
      } else {
        ERROR("Error: unexpected argument: %s\n", argv[i]);
        return -1;
      }
    }
  }

  /* Check for required arguments that are missing */
  for (i = 0; i < parser->flag_count; i++) {
    if (parser->flags[i].required && !parser->flags[i].processed) {
      if (parser->flags[i].long_flag != NULL) {
        ERROR("Error: required option --%s is missing\n",
              parser->flags[i].long_flag);
      } else {
        ERROR("Error: required option -%c is missing\n",
              parser->flags[i].short_flag);
      }
      return -1;
    }
  }

  for (i = 0; i < parser->pos_arg_count; i++) {
    if (parser->pos_args[i].required && !parser->pos_args[i].processed) {
      ERROR("Error: required argument %s is missing\n",
            parser->pos_args[i].name);
      return -1;
    }
  }

  return 0;
}

/* Helper function to calculate option width dynamically */
static int calculate_option_width(char short_flag, const char *long_flag, 
                                  arg_type type, const char *meta) {
  int width = 0;

  /* Calculate width for short flag */
  if (short_flag != '\0') {
    width += 2; /* -f */

    if (type != ARG_TYPE_NONE && meta != NULL) {
      width += 1 + strlen(meta); /* -f META */
    }
  }

  /* Add space between short and long flags */
  if (short_flag != '\0' && long_flag != NULL) {
    width += 2; /* ", " */
  }

  /* Calculate width for long flag */
  if (long_flag != NULL) {
    width += 2 + strlen(long_flag); /* --flag */

    if (type != ARG_TYPE_NONE && meta != NULL) {
      width += 1 + strlen(meta); /* --flag META */
    }
  }

  return width;
}

/* Print usage and help information */
void argparse_print_help(arg_parser *parser, FILE *out) {
  int i;
  int max_option_width = 0;

  if (parser == NULL || out == NULL) {
    return;
  }

  /* Calculate the maximum width of the option column dynamically */
  /* Check built-in help flag */
  int help_width = calculate_option_width('h', "help", ARG_TYPE_NONE, NULL);
  if (help_width > max_option_width) {
    max_option_width = help_width;
  }
  
  /* Check built-in version flag */
  int version_width = calculate_option_width('V', "version", ARG_TYPE_NONE, NULL);
  if (version_width > max_option_width) {
    max_option_width = version_width;
  }
  
  /* Check all user-defined flags */
  for (i = 0; i < parser->flag_count; i++) {
    int width = calculate_option_width(parser->flags[i].short_flag,
                                       parser->flags[i].long_flag,
                                       parser->flags[i].type,
                                       parser->flags[i].meta);
    if (width > max_option_width) {
      max_option_width = width;
    }
  }

  /* Add some padding */
  max_option_width += 2;

  /* Print usage */
  fprintf(out, "Usage: %s [OPTIONS]", parser->prog_name);

  /* Add positional arguments to usage */
  for (i = 0; i < parser->pos_arg_count; i++) {
    if (parser->pos_args[i].required) {
      fprintf(out, " %s", parser->pos_args[i].name);
    } else {
      fprintf(out, " [%s]", parser->pos_args[i].name);
    }
  }

  /* Add usage suffix */
  if (parser->usage_suffix != NULL) {
    fprintf(out, " %s", parser->usage_suffix);
  }

  fprintf(out, "\n\n");

  /* Print description */
  fprintf(out, "%s v%s: %s\n\n", parser->prog_name, parser->prog_version,
          parser->prog_description);

  /* Print optional arguments */
  if (parser->flag_count > 0) {
    fprintf(out, "Optional arguments:\n");

    /* First print -h/--help */
    fprintf(out, "  -h, --help");
    for (i = 0; i < max_option_width - help_width; i++) {
      fprintf(out, " ");
    }
    fprintf(out, "Show this help message and exit\n");

    /* Then print -V/--version */
    fprintf(out, "  -V, --version");
    for (i = 0; i < max_option_width - version_width; i++) {
      fprintf(out, " ");
    }
    fprintf(out, "Show version information and exit\n");

    /* Then print all other flags */
    for (i = 0; i < parser->flag_count; i++) {
      /* Calculate width using the helper function */
      int width = calculate_option_width(parser->flags[i].short_flag,
                                         parser->flags[i].long_flag,
                                         parser->flags[i].type,
                                         parser->flags[i].meta);

      fprintf(out, "  ");

      /* Print short flag */
      if (parser->flags[i].short_flag != '\0') {
        fprintf(out, "-%c", parser->flags[i].short_flag);

        if (parser->flags[i].type != ARG_TYPE_NONE) {
          fprintf(out, " %s", parser->flags[i].meta);
        }
      }

      /* Print separator between short and long flags */
      if (parser->flags[i].short_flag != '\0' &&
          parser->flags[i].long_flag != NULL) {
        fprintf(out, ", ");
      }

      /* Print long flag */
      if (parser->flags[i].long_flag != NULL) {
        fprintf(out, "--%s", parser->flags[i].long_flag);

        if (parser->flags[i].type != ARG_TYPE_NONE) {
          fprintf(out, " %s", parser->flags[i].meta);
        }
      }

      /* Add padding - ensure at least 2 spaces before help text */
      int padding_needed = max_option_width - width;
      if (padding_needed < 2) {
        padding_needed = 2;
      }
      for (int j = 0; j < padding_needed; j++) {
        fprintf(out, " ");
      }

      /* Print help text */
      fprintf(out, "%s", parser->flags[i].help);

      /* Print enum values */
      if (parser->flags[i].type == ARG_TYPE_ENUM &&
          parser->flags[i].enum_values != NULL) {
        fprintf(out, " (choices: ");
        for (int j = 0; j < parser->flags[i].enum_count; j++) {
          if (j > 0) {
            fprintf(out, ", ");
          }
          fprintf(out, "%s", parser->flags[i].enum_values[j]);
        }
        fprintf(out, ")");
      }

      fprintf(out, "\n");
    }

    fprintf(out, "\n");
  }

  /* Print positional arguments */
  if (parser->pos_arg_count > 0) {
    fprintf(out, "Arguments:\n");

    for (i = 0; i < parser->pos_arg_count; i++) {
      fprintf(out, "  %s", parser->pos_args[i].name);

      /* Add padding */
      int name_len = strlen(parser->pos_args[i].name);
      for (int j = 0; j < max_option_width - name_len; j++) {
        fprintf(out, " ");
      }

      /* Print help text */
      fprintf(out, "%s\n", parser->pos_args[i].help);
    }

    fprintf(out, "\n");
  }
}

/* Print version information */
void argparse_print_version(arg_parser *parser, FILE *out) {
  if (parser == NULL || out == NULL) {
    return;
  }

  fprintf(out, "%s v%s\n", parser->prog_name, parser->prog_version);
}
