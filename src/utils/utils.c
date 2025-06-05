#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <io.h>
#include <sys/utime.h>
#else
#include <unistd.h>
#include <utime.h>
#endif

#include "utils.h"

// global verbosity setting
int g_verbosity = 0;

int read_s16_be(unsigned char *buf) {
  unsigned tmp = read_u16_be(buf);
  int ret;
  if (tmp > 0x7FFF) {
    ret = -((int)0x10000 - (int)tmp);
  } else {
    ret = (int)tmp;
  }
  return ret;
}

float read_f32_be(unsigned char *buf) {
  union {
    uint32_t i;
    float f;
  } ret;
  ret.i = read_u32_be(buf);
  return ret.f;
}

int is_power2(unsigned int val) {
  while (((val & 1) == 0) && (val > 1)) {
    val >>= 1;
  }
  return (val == 1);
}

void fprint_hex(FILE *fp, const unsigned char *buf, int length) {
  int i;
  for (i = 0; i < length; i++) {
    fprint_byte(fp, buf[i]);
    fputc(' ', fp);
  }
}

void fprint_hex_source(FILE *fp, const unsigned char *buf, int length) {
  int i;
  for (i = 0; i < length; i++) {
    if (i > 0)
      fputs(", ", fp);
    fputs("0x", fp);
    fprint_byte(fp, buf[i]);
  }
}

void print_hex(const unsigned char *buf, int length) {
  fprint_hex(stdout, buf, length);
}

void swap_bytes(unsigned char *data, long length) {
  long i;
  unsigned char tmp;
  for (i = 0; i < length; i += 2) {
    tmp = data[i];
    data[i] = data[i + 1];
    data[i + 1] = tmp;
  }
}

void reverse_endian(unsigned char *data, long length) {
  long i;
  unsigned char tmp;
  for (i = 0; i < length; i += 4) {
    tmp = data[i];
    data[i] = data[i + 3];
    data[i + 3] = tmp;
    tmp = data[i + 1];
    data[i + 1] = data[i + 2];
    data[i + 2] = tmp;
  }
}

long filesize(const char *filename) {
  struct stat st;

  if (stat(filename, &st) == 0) {
    return st.st_size;
  }

  return -1;
}

void touch_file(const char *filename) {
  int fd;
  // fd = open(filename, O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK, 0666);
  fd = open(filename, O_WRONLY | O_CREAT, 0666);
  if (fd >= 0) {
    utime(filename, NULL);
    close(fd);
  }
}

// Memory management constants for large file handling
#define CHUNK_SIZE (8 * MB)  // 8MB chunks for better memory management
#define MAX_CHUNKS 8         // Up to 64 MBs (Maximum size for n64 ROMs)

long read_file(const char *file_name, unsigned char **data) {
  FILE *in;
  unsigned char *in_buf = NULL;
  long file_size;
  long bytes_read;
  
  DEBUG("Attempting to read file: %s\n", file_name);
  
  in = fopen(file_name, "rb");
  if (in == NULL) {
    ERROR("Failed to open file '%s': %s\n", file_name, strerror(errno));
    return -1;
  }

  // allocate buffer to read from offset to end of file
  fseek(in, 0, SEEK_END);
  file_size = ftell(in);
  
  DEBUG("File size: %ld bytes\n", file_size);

  // sanity check
  if (file_size > 256 * MB) {
    ERROR("File '%s' is too large (%ld bytes, max: %d MB)\n", file_name, file_size, 256);
    fclose(in);
    return -2;
  }
  
  if (file_size <= 0) {
    ERROR("File '%s' is empty or invalid (size: %ld)\n", file_name, file_size);
    fclose(in);
    return -3;
  }

  // For files larger than 8MB, try multiple smaller allocations
  if (file_size > 8 * MB) {
    INFO("Large file detected (%ld bytes), using fragmented allocation approach\n", file_size);
    
    // Try to allocate in progressively smaller chunks if needed
    long chunk_sizes[] = {file_size, file_size/2, file_size/4, file_size/8, 8*MB, 4*MB, 2*MB, 1*MB};
    int num_chunks = sizeof(chunk_sizes) / sizeof(chunk_sizes[0]);
    
    in_buf = NULL;
    for (int attempt = 0; attempt < num_chunks; attempt++) {
      long attempt_size = chunk_sizes[attempt];
      if (attempt_size <= 0) continue;
      
      INFO("Attempting allocation of %ld bytes (attempt %d)\n", attempt_size, attempt + 1);
      in_buf = malloc(attempt_size);
      if (in_buf != NULL) {
        if (attempt_size >= file_size) {
          // We got the full size, read directly
          INFO("Successfully allocated %ld bytes for full file\n", attempt_size);
          break;
        } else {
          // We got a smaller buffer, free it and use true streaming approach
          free(in_buf);
          in_buf = NULL;
          INFO("Using true streaming approach with %ld byte chunks\n", attempt_size);
          
          // Allocate an array to hold multiple small chunks
          long read_chunk_size = attempt_size;
          int max_chunks_in_memory = 8; // Keep max 8 chunks in memory at once
          unsigned char **chunk_buffers = calloc(max_chunks_in_memory, sizeof(unsigned char*));
          long *chunk_sizes = calloc(max_chunks_in_memory, sizeof(long));
          
          if (chunk_buffers == NULL || chunk_sizes == NULL) {
            ERROR("Failed to allocate chunk management arrays\n");
            if (chunk_buffers) free(chunk_buffers);
            if (chunk_sizes) free(chunk_sizes);
            fclose(in);
            return -4;
          }
          
          // Read file in chunks and immediately try to allocate final buffer
          fseek(in, 0, SEEK_SET);
          long total_read = 0;
          int current_chunk_idx = 0;
          
          // First, try to allocate the full buffer one more time
          in_buf = malloc(file_size);
          if (in_buf != NULL) {
            // We got the full buffer! Read directly into it
            bytes_read = fread(in_buf, 1, file_size, in);
            if (bytes_read != file_size) {
              ERROR("Failed to read complete file: read %ld of %ld bytes\n", bytes_read, file_size);
              free(in_buf);
              free(chunk_buffers);
              free(chunk_sizes);
              fclose(in);
              return -5;
            }
            
            free(chunk_buffers);
            free(chunk_sizes);
            fclose(in);
            *data = in_buf;
            INFO("Successfully read %ld bytes after retry allocation\n", bytes_read);
            return bytes_read;
          }
          
          // Still can't allocate full buffer, use chunked approach
          while (total_read < file_size) {
            long remaining = file_size - total_read;
            long current_chunk_size = (remaining < read_chunk_size) ? remaining : read_chunk_size;
            
            // Allocate chunk buffer
            chunk_buffers[current_chunk_idx] = malloc(current_chunk_size);
            if (chunk_buffers[current_chunk_idx] == NULL) {
              ERROR("Failed to allocate chunk buffer of %ld bytes\n", current_chunk_size);
              // Free any allocated chunks
              for (int i = 0; i < current_chunk_idx; i++) {
                if (chunk_buffers[i]) free(chunk_buffers[i]);
              }
              free(chunk_buffers);
              free(chunk_sizes);
              fclose(in);
              return -4;
            }
            
            // Read chunk
            bytes_read = fread(chunk_buffers[current_chunk_idx], 1, current_chunk_size, in);
            if (bytes_read != current_chunk_size) {
              ERROR("Failed to read chunk: read %ld of %ld bytes at offset %ld\n", 
                    bytes_read, current_chunk_size, total_read);
              // Free chunk buffers
              for (int i = 0; i <= current_chunk_idx; i++) {
                if (chunk_buffers[i]) free(chunk_buffers[i]);
              }
              free(chunk_buffers);
              free(chunk_sizes);
              fclose(in);
              return -5;
            }
            
            chunk_sizes[current_chunk_idx] = bytes_read;
            total_read += bytes_read;
            current_chunk_idx++;
            
            DEBUG("Read chunk %d: %ld bytes, total: %ld/%ld\n", 
                  current_chunk_idx, bytes_read, total_read, file_size);
            
            // If we've filled our chunk array or read everything, try to consolidate
            if (current_chunk_idx >= max_chunks_in_memory || total_read >= file_size) {
              // Try to allocate final buffer again
              in_buf = malloc(file_size);
              if (in_buf != NULL) {
                // Copy chunks to final buffer
                long offset = 0;
                for (int i = 0; i < current_chunk_idx; i++) {
                  memcpy(in_buf + offset, chunk_buffers[i], chunk_sizes[i]);
                  offset += chunk_sizes[i];
                  free(chunk_buffers[i]);
                  chunk_buffers[i] = NULL;
                }
                
                // Read remaining data directly if any
                if (total_read < file_size) {
                  long remaining = file_size - total_read;
                  bytes_read = fread(in_buf + offset, 1, remaining, in);
                  if (bytes_read != remaining) {
                    ERROR("Failed to read remaining data: read %ld of %ld bytes\n", 
                          bytes_read, remaining);
                    free(in_buf);
                    free(chunk_buffers);
                    free(chunk_sizes);
                    fclose(in);
                    return -5;
                  }
                  total_read += bytes_read;
                }
                
                free(chunk_buffers);
                free(chunk_sizes);
                fclose(in);
                *data = in_buf;
                INFO("Successfully read %ld bytes using chunked consolidation\n", total_read);
                return total_read;
              }
              
              // Still can't allocate, continue with more chunks
              if (total_read >= file_size) {
                // We've read everything but can't consolidate - this is a problem
                ERROR("Read complete file but cannot allocate consolidation buffer\n");
                for (int i = 0; i < current_chunk_idx; i++) {
                  if (chunk_buffers[i]) free(chunk_buffers[i]);
                }
                free(chunk_buffers);
                free(chunk_sizes);
                fclose(in);
                return -4;
              }
            }
          }
          
          ERROR("Reached end of chunked reading without success\n");
          for (int i = 0; i < current_chunk_idx; i++) {
            if (chunk_buffers[i]) free(chunk_buffers[i]);
          }
          free(chunk_buffers);
          free(chunk_sizes);
          fclose(in);
          return -4;
        }
      }
    }
    
    if (in_buf == NULL) {
      ERROR("Failed to allocate memory for file after trying multiple chunk sizes\n");
      fclose(in);
      return -4;
    }
    
    // If we got here, we allocated the full buffer, read the file normally
    fseek(in, 0, SEEK_SET);
    bytes_read = fread(in_buf, 1, file_size, in);
    if (bytes_read != file_size) {
      ERROR("Failed to read complete file '%s': read %ld of %ld bytes: %s\n", 
            file_name, bytes_read, file_size, strerror(errno));
      free(in_buf);
      fclose(in);
      return -5;
    }

    fclose(in);
    *data = in_buf;
    INFO("Successfully read %ld bytes from '%s' using full allocation\n", bytes_read, file_name);
    return bytes_read;
  } else {
    // For smaller files, use the original approach
    in_buf = malloc(file_size);
    if (in_buf == NULL) {
      ERROR("Failed to allocate %ld bytes for file '%s': %s\n", file_size, file_name, strerror(errno));
      fclose(in);
      return -4;
    }
    
    fseek(in, 0, SEEK_SET);

    // read bytes
    bytes_read = fread(in_buf, 1, file_size, in);
    if (bytes_read != file_size) {
      ERROR("Failed to read complete file '%s': read %ld of %ld bytes: %s\n", 
            file_name, bytes_read, file_size, strerror(errno));
      free(in_buf);
      fclose(in);
      return -5;
    }

    fclose(in);
    *data = in_buf;
    INFO("Successfully read %ld bytes from '%s'\n", bytes_read, file_name);
    return bytes_read;
  }
}

long write_file(const char *file_name, unsigned char *data, long length) {
  FILE *out;
  long bytes_written;
  // open output file
  out = fopen(file_name, "wb");
  if (out == NULL) {
    perror(file_name);
    return -1;
  }
  bytes_written = fwrite(data, 1, length, out);
  fclose(out);
  return bytes_written;
}

void generate_filename(const char *in_name, char *out_name, char *extension) {
  char tmp_name[FILENAME_MAX];
  int len;
  int i;
  strcpy(tmp_name, in_name);
  len = strlen(tmp_name);
  for (i = len - 1; i > 0; i--) {
    if (tmp_name[i] == '.') {
      break;
    }
  }
  if (i <= 0) {
    i = len;
  }
  tmp_name[i] = '\0';
  sprintf(out_name, "%s.%s", tmp_name, extension);
}

char *basename(const char *name) {
  const char *base = name;
  while (*name) {
    if (*name++ == '/') {
      base = name;
    }
  }
  return (char *)base;
}

void make_dir(const char *dir_name) {
  struct stat st = {0};
  if (stat(dir_name, &st) == -1) {
    mkdir(dir_name, 0755);
  }
}

long copy_file(const char *src_name, const char *dst_name) {
  unsigned char *buf;
  long bytes_written;
  long bytes_read;

  bytes_read = read_file(src_name, &buf);

  if (bytes_read > 0) {
    bytes_written = write_file(dst_name, buf, bytes_read);
    if (bytes_written != bytes_read) {
      bytes_read = -1;
    }
    free(buf);
  }

  return bytes_read;
}

void dir_list_ext(const char *dir, const char *extension, dir_list *list) {
  char *pool;
  char *pool_ptr;
  struct dirent *entry;
  DIR *dfd;
  int idx;

  dfd = opendir(dir);
  if (dfd == NULL) {
    ERROR("Can't open '%s'\n", dir);
    exit(1);
  }

  pool = malloc(FILENAME_MAX * MAX_DIR_FILES);
  pool_ptr = pool;

  idx = 0;
  while ((entry = readdir(dfd)) != NULL && idx < MAX_DIR_FILES) {
    if (!extension || str_ends_with(entry->d_name, extension)) {
      sprintf(pool_ptr, "%s/%s", dir, entry->d_name);
      list->files[idx] = pool_ptr;
      pool_ptr += strlen(pool_ptr) + 1;
      idx++;
    }
  }
  list->count = idx;

  closedir(dfd);
}

void dir_list_free(dir_list *list) {
  // assume first entry in array is allocated
  if (list->files[0]) {
    free(list->files[0]);
    list->files[0] = NULL;
  }
}

int str_ends_with(const char *str, const char *suffix) {
  if (!str || !suffix) {
    return 0;
  }
  size_t len_str = strlen(str);
  size_t len_suffix = strlen(suffix);
  if (len_suffix > len_str) {
    return 0;
  }
  return (0 == strncmp(str + len_str - len_suffix, suffix, len_suffix));
}
