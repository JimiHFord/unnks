#include <errno.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

bool
read_string (int fd, char *ret, size_t size)
{
  ssize_t cnt;

  if (size > SSIZE_MAX)
    return false;

  cnt = read (fd, ret, size - 1);
  if (cnt < 0 || cnt + 1 != (ssize_t) size)
    return false;

  ret[size - 1] = 0;
  return true;
}

bool
read_utf16_le_string (int fd, char **ret)
{
  GArray *array;
  uint16_t c;

  array = g_array_new (false, true, 2);

  do
    {
      if (!read_u16_le (fd, &c))
	goto err;

      g_array_append_val (array, c);
    }
  while (c != 0);

  *ret = g_utf16_to_utf8 ((gunichar2 *) array->data, 2 * array->len,
			  NULL, NULL, NULL);
  if (*ret == NULL)
    goto err;

  g_array_free (array, true);
  return true;

err:
  g_array_free (array, true);
  return false;
}

bool
read_u32_le (int fd, uint32_t *ret)
{
  int n;
  uint8_t c;
  uint32_t num = 0;

  for (n = 0; n < 4; n++)
    {
      if (read (fd, &c, 1) != 1)
	return false;

      num |= (c << (8 * n));
    }

  *ret = num;
  return true;
}

bool
read_u16_le (int fd, uint16_t *ret)
{
  int n;
  uint8_t c;
  uint16_t num = 0;

  for (n = 0; n < 2; n++)
    {
      if (read (fd, &c, 1) != 1)
	return false;

      num |= ((uint16_t) c << (8 * n));
    }

  *ret = num;
  return true;
}

uint32_t
read_u32_be_mem (const void *mem)
{
  const uint8_t *p = mem;
  uint32_t num = 0;
  int n;

  for (n = 0; n < 4; n++)
    num |= p[3 - n] << (8 * n);

  return num;
}

int
join_path_segments (const char *prefix, const char *suffix,
		    char *buffer, size_t size)
{
  int len;

  if (prefix[0] == '\0')
    len = snprintf (buffer, size, "%s", suffix);
  else
    len = snprintf (buffer, size, "%s" SEP "%s", prefix, suffix);

  if ((size_t) len >= size)
    return -EINVAL;

  return 0;
}

int
extract_path_segment (const char *path, char *buffer, size_t len,
		      const char **rest)
{
  char *sl;

  while (*path == '/')
    path++;

  sl = strchr (path, '/');
  if (sl == NULL)
    {
      if ((size_t) snprintf (buffer, len, "%s", path) >= len)
	return -EINVAL;

      *rest = NULL;
      return 0;
    }

  if ((size_t) (sl - path) >= len)
    return -EINVAL;

  strncpy (buffer, path, sl - path);
  buffer[sl - path] = 0;

  while (*sl == '/')
    sl++;

  if (*sl != '\0')
    *rest = sl;
  else
    *rest = NULL;

  return 0;
}

bool
valid_file_name (const char *name)
{
  const char *start;
  const char *end;

  start = name;

  do
    {
      while (*start == SEP_CHAR)
	start++;

      end = strchr (start, SEP_CHAR);
      if (end == NULL)
	end = name + strlen (name);

      if (end - start == 2 && strncmp (start, "..", 2) == 0)
	return false;

      start = end;
    }
  while (*end != '\0');

  return true;
}

int
puts_utf8 (const char *text)
{
  char *converted;
  int r;

  converted = g_locale_from_utf8 (text, -1, NULL, NULL, NULL);
  if (converted != NULL)
    {
      r = puts (converted);
      g_free (converted);
    }
  else
    r = puts (text);

  return r;
}

int
vfprintf_utf8 (FILE *file, const char *fmt, va_list lst)
{
  char buffer[16384];
  char *converted;
  int r;

  g_vsnprintf (buffer, sizeof (buffer), fmt, lst);
  converted = g_locale_from_utf8 (buffer, -1, NULL, NULL, NULL);
  if (converted != NULL)
    {
      r = fprintf (file, "%s", converted);
      g_free (converted);
    }
  else
    r = fprintf (file, "%s", buffer);

  return r;
}

int
fprintf_utf8 (FILE *file, const char *fmt, ...)
{
  va_list lst;
  int r;

  va_start (lst, fmt);
  r = vfprintf_utf8 (file, fmt, lst);
  va_end (lst);

  return r;
}

int
printf_utf8 (const char *fmt, ...)
{
  va_list lst;
  int r;

  va_start (lst, fmt);
  r = vfprintf_utf8 (stdout, fmt, lst);
  va_end (lst);

  return r;
}

uint32_t
rand_ms (uint32_t *seedp)
{
  *seedp = *seedp * UINT32_C (0x343fd) + UINT32_C (0x269ec3);
  return (*seedp >> 16);
}
