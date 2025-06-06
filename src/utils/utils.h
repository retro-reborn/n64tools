#ifndef UTILS_H_
#define UTILS_H_

#include <errno.h>
#include <stdio.h>

// defines

// printing size_t varies by compiler
#if defined(_MSC_VER) || defined(__MINGW32__)
#define SIZE_T_FORMAT "%Iu"
#else
#define SIZE_T_FORMAT "%zu"
#endif

#define KB 1024
#define MB (1024 * KB)

// number of elements in statically declared array
#define DIM(S_ARR_) (sizeof(S_ARR_) / sizeof(S_ARR_[0]))

#define MIN(A_, B_) ((A_) < (B_) ? (A_) : (B_))
#define MAX(A_, B_) ((A_) > (B_) ? (A_) : (B_))

// align value to N-byte boundary
#define ALIGN(VAL_, ALIGNMENT_)                                                \
  (((VAL_) + ((ALIGNMENT_) - 1)) & ~((ALIGNMENT_) - 1))

// read/write u32/16 big/little endian
#define read_u32_be(buf)                                                       \
  (unsigned int)(((buf)[0] << 24) + ((buf)[1] << 16) + ((buf)[2] << 8) +       \
                 ((buf)[3]))
#define read_u32_le(buf)                                                       \
  (unsigned int)(((buf)[1] << 24) + ((buf)[0] << 16) + ((buf)[3] << 8) +       \
                 ((buf)[2]))
#define write_u32_be(buf, val)                                                 \
  do {                                                                         \
    (buf)[0] = ((val) >> 24) & 0xFF;                                           \
    (buf)[1] = ((val) >> 16) & 0xFF;                                           \
    (buf)[2] = ((val) >> 8) & 0xFF;                                            \
    (buf)[3] = (val) & 0xFF;                                                   \
  } while (0)
#define read_u16_be(buf) (((buf)[0] << 8) + ((buf)[1]))
#define write_u16_be(buf, val)                                                 \
  do {                                                                         \
    (buf)[0] = ((val) >> 8) & 0xFF;                                            \
    (buf)[1] = ((val)) & 0xFF;                                                 \
  } while (0)

// print nibbles and bytes
#define fprint_nibble(FP, NIB_)                                                \
  fputc((NIB_) < 10 ? ('0' + (NIB_)) : ('A' + (NIB_) - 0xA), FP)
#define fprint_byte(FP, BYTE_)                                                 \
  do {                                                                         \
    fprint_nibble(FP, (BYTE_) >> 4);                                           \
    fprint_nibble(FP, (BYTE_) & 0x0F);                                         \
  } while (0)
#define print_nibble(NIB_) fprint_nibble(stdout, NIB_)
#define print_byte(BYTE_) fprint_byte(stdout, BYTE_)

// Windows compatibility
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <direct.h>
#define mkdir(DIR_, PERM_) _mkdir(DIR_)
#ifndef strcasecmp
#define strcasecmp(A, B) stricmp(A, B)
#endif
#endif

// typedefs

#define MAX_DIR_FILES 128
typedef struct {
  char *files[MAX_DIR_FILES];
  int count;
} dir_list;

// global verbosity setting
extern int g_verbosity;

// ANSI color codes
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_RESET   "\033[0m"

#define ERROR(...) fprintf(stderr, COLOR_RED "[ERROR] " COLOR_RESET __VA_ARGS__)
#define WARNING(...) fprintf(stderr, COLOR_YELLOW "[WARNING] " COLOR_RESET __VA_ARGS__)
#define INFO(...)                                                              \
  if (g_verbosity >= 1)                                                        \
  printf(COLOR_CYAN "[INFO] " COLOR_RESET __VA_ARGS__)
#define DEBUG(...)                                                             \
  if (g_verbosity >= 2)                                                        \
  printf(COLOR_BLUE "[DEBUG] " COLOR_RESET __VA_ARGS__)
#define VERBOSE(...)                                                           \
  if (g_verbosity >= 3)                                                        \
  printf(COLOR_MAGENTA "[VERBOSE] " COLOR_RESET __VA_ARGS__)
#define INFO_HEX(...)                                                          \
  if (g_verbosity >= 1) {                                                      \
    printf(COLOR_CYAN "[INFO] " COLOR_RESET);                                  \
    print_hex(__VA_ARGS__);                                                    \
  }
#define DEBUG_HEX(...)                                                         \
  if (g_verbosity >= 2) {                                                      \
    printf(COLOR_BLUE "[DEBUG] " COLOR_RESET);                                 \
    print_hex(__VA_ARGS__);                                                    \
  }

// functions

// convert two bytes in big-endian to signed int
int read_s16_be(unsigned char *buf);

// convert four bytes in big-endian to float
float read_f32_be(unsigned char *buf);

// determine if value is power of 2
// returns 1 if val is power of 2, 0 otherwise
int is_power2(unsigned int val);

// print buffer as hex bytes
// fp: file pointer
// buf: buffer to read bytes from
// length: length of buffer to print
void fprint_hex(FILE *fp, const unsigned char *buf, int length);
void fprint_hex_source(FILE *fp, const unsigned char *buf, int length);
void print_hex(const unsigned char *buf, int length);

// perform byteswapping to convert from v64 to z64 ordering
void swap_bytes(unsigned char *data, long length);

// reverse endian to convert from n64 to z64 ordering
void reverse_endian(unsigned char *data, long length);

// get size of file without opening it;
// returns file size or negative on error
long filesize(const char *file_name);

// update file timestamp to now, creating it if it doesn't exist
void touch_file(const char *filename);

// read entire contents of file into buffer
// returns file size or negative on error
long read_file(const char *file_name, unsigned char **data);

// write buffer to file
// returns number of bytes written out or -1 on failure
long write_file(const char *file_name, unsigned char *data, long length);

// generate an output file name from input name by replacing file extension
// in_name: input file name
// out_name: buffer to write output name in
// extension: new file extension to use
void generate_filename(const char *in_name, char *out_name, char *extension);

// extract base filename from file path
// name: path to file
// returns just the file name after the last '/'
char *basename(const char *name);

// make a directory if it doesn't exist
// dir_name: name of the directory
void make_dir(const char *dir_name);

// copy a file from src_name to dst_name. will not make directories
// src_name: source file name
// dst_name: destination file name
long copy_file(const char *src_name, const char *dst_name);

// list a directory, optionally filtering files by extension
// dir: directory to list files in
// extension: extension to filter files by (NULL if no filtering)
// list: output list and count
void dir_list_ext(const char *dir, const char *extension, dir_list *list);

// free associated date from a directory list
// list: directory list filled in by dir_list_ext() call
void dir_list_free(dir_list *list);

// determine if a string ends with another string
// str: string to check if ends with 'suffix'
// suffix: string to see if 'str' ends with
// returns 1 if 'str' ends with 'suffix'
int str_ends_with(const char *str, const char *suffix);

#endif // UTILS_H_
