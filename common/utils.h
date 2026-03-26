#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

#include "constants.h"

double now_seconds(void);
int read_stdin_line(char *buffer, size_t size);
void trim_newline(char *value);
int is_safe_filename(const char *filename);
uint32_t crc32_buffer(const unsigned char *data, size_t length);
int compute_file_crc32(const char *path, uint32_t *crc_out, uint32_t *file_size_out);
size_t aligned_resume_size(size_t size);
int truncate_file_to_size(const char *path, size_t size);

#endif
