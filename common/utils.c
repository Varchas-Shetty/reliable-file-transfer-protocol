#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "utils.h"

double now_seconds(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0);
}

int read_stdin_line(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return -1;
    }

    trim_newline(buffer);
    return 0;
}

void trim_newline(char *value) {
    size_t length = strlen(value);

    while (length > 0 &&
           (value[length - 1] == '\n' || value[length - 1] == '\r')) {
        value[length - 1] = '\0';
        length--;
    }
}

int is_safe_filename(const char *filename) {
    size_t i;
    size_t length;

    if (filename == NULL) {
        return 0;
    }

    length = strlen(filename);
    if (length == 0 || length > MAX_FILENAME_LENGTH) {
        return 0;
    }

    if (strstr(filename, "..") != NULL ||
        strchr(filename, '/') != NULL ||
        strchr(filename, '\\') != NULL) {
        return 0;
    }

    for (i = 0; i < length; ++i) {
        unsigned char ch = (unsigned char)filename[i];

        if (!(isalnum(ch) || ch == '.' || ch == '_' || ch == '-')) {
            return 0;
        }
    }

    return 1;
}

uint32_t crc32_buffer(const unsigned char *data, size_t length) {
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;

    for (i = 0; i < length; ++i) {
        uint32_t value = (uint32_t)(crc ^ data[i]);
        int bit;

        for (bit = 0; bit < 8; ++bit) {
            if ((value & 1u) != 0u) {
                value = (value >> 1u) ^ 0xEDB88320u;
            } else {
                value >>= 1u;
            }
        }

        crc = (crc >> 8u) ^ value;
    }

    return crc ^ 0xFFFFFFFFu;
}

int compute_file_crc32(const char *path, uint32_t *crc_out, uint32_t *file_size_out) {
    unsigned char buffer[MAX_PAYLOAD_SIZE];
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t total_size = 0;
    FILE *fp = fopen(path, "rb");

    if (fp == NULL) {
        return -1;
    }

    while (1) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), fp);
        size_t i;

        if (bytes_read == 0) {
            if (ferror(fp)) {
                fclose(fp);
                return -1;
            }
            break;
        }

        total_size += (uint32_t)bytes_read;

        for (i = 0; i < bytes_read; ++i) {
            uint32_t value = (uint32_t)(crc ^ buffer[i]);
            int bit;

            for (bit = 0; bit < 8; ++bit) {
                if ((value & 1u) != 0u) {
                    value = (value >> 1u) ^ 0xEDB88320u;
                } else {
                    value >>= 1u;
                }
            }

            crc = (crc >> 8u) ^ value;
        }
    }

    fclose(fp);
    if (crc_out != NULL) {
        *crc_out = crc ^ 0xFFFFFFFFu;
    }
    if (file_size_out != NULL) {
        *file_size_out = total_size;
    }
    return 0;
}

size_t aligned_resume_size(size_t size) {
    return (size / MAX_PAYLOAD_SIZE) * MAX_PAYLOAD_SIZE;
}

int truncate_file_to_size(const char *path, size_t size) {
    return truncate(path, (off_t)size);
}
