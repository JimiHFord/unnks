#ifndef NKS_UTIL_H
#define NKS_UTIL_H

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __unix__
# define mkdir(dir, mode) mkdir((dir), (mode))
# define SEP "/"
#elif defined _WIN32
# define mkdir(dir, mode) mkdir(dir)
# define SEP "\\"
#else
# error Unknown operating system
#endif

#define SEP_CHAR (SEP[0])

#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

#ifndef ENOTSUP
# define ENOTSUP ENOSYS
#endif

#ifndef ENOKEY
# define ENOKEY EPERM
#endif

bool read_string (int fd, char *ret, size_t size);
bool read_utf16_le_string (int fd, char **ret);
bool read_u32_le (int fd, uint32_t *ret);
bool read_u16_le (int fd, uint16_t *ret);
uint32_t read_u32_be_mem (const void *mem);

int join_path_segments (const char *prefix, const char *suffix,
			char *buffer, size_t size);
int extract_path_segment (const char *path, char *segment, size_t len,
			  const char **rest);

bool valid_file_name (const char *name);

int puts_utf8 (const char *text);
int fprintf_utf8 (FILE *file, const char *fmt, ...);
int vfprintf_utf8 (FILE *file, const char *fmt, va_list lst);
int printf_utf8 (const char *fmt, ...);

uint32_t rand_ms (uint32_t *seedp);

#endif
