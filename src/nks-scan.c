#include <fcntl.h>
#include <glib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "nks_io.h"
#include "util.h"

static void
print_usage (const char *argv0)
{
  printf ("Usage: %s FILE\n", argv0);
}

#ifdef __G_CHECKSUM_H__
bool
md5sum (const char *name, uint8_t sum[16])
{
  guchar buffer[16384];
  GChecksum *csum = NULL;
  gsize bufsize;
  int fd = -1;
  int r;

  fd = open (name, O_RDONLY | O_BINARY);
  if (fd < 0)
    goto err;

  csum = g_checksum_new (G_CHECKSUM_MD5);

  for (;;)
    {
      r = read (fd, buffer, sizeof (buffer));
      if (r < 0)
	goto err;

      if (r == 0)
	break;

      g_checksum_update (csum, buffer, r);
    }

  bufsize = 16;
  g_checksum_get_digest (csum, sum, &bufsize);

  g_checksum_free (csum);
  close (fd);

  return true;

err:
  perror (name);

  if (csum != NULL)
    g_checksum_free (csum);

  if (fd >= 0)
    close (fd);

  return false;
}
#endif

static bool
print_file_info (const char *name)
{
  struct stat st;
#ifdef __G_CHECKSUM_H__
  uint8_t checksum[16];
  int n;
#endif

  if (stat (name, &st) != 0)
    {
      perror (name);
      return false;
    }

#ifdef __G_CHECKSUM_H__
  if (!md5sum (name, checksum))
    return false;

  printf ("md5:");

  for (n = 0; n < 16; n++)
    printf ("%02x", checksum[n]);

  printf (" ");
#endif

  printf ("size:%"PRIuMAX, (uintmax_t) st.st_size);

  printf ("\n");

  return true;
}

static void
print_indent (int indent)
{
  int n;

  for (n = 0; n < indent; n++)
    printf ("  ");
}

static bool scan_chunk (int fd, const char *name, int indent);

static bool
scan_0100_entry (int fd, off_t offset, Nks0100EntryHeader *header,
		 int indent)
{
  print_indent (indent++);

  printf ("e:%08"PRIxMAX":%08"PRIx32":%04"PRIx16":%02x:%s\n",
	  (uintmax_t) offset, header->offset, header->type,
	  header->unknown[0], header->name);

  if (lseek (fd, header->offset, SEEK_SET) < 0)
    return false;

  return scan_chunk (fd, header->name, indent);
}

static bool
scan_0100_entries (int fd, uint32_t count, int indent)
{
  off_t *offsets;
  Nks0100EntryHeader *entries;
  uint32_t n;
  int r;

  entries = g_malloc0 (count * sizeof (*entries));
  offsets = g_malloc0 (count * sizeof (*offsets));

  for (n = 0; n < count; n++)
    {
      offsets[n] = lseek (fd, 0, SEEK_CUR);
      if (offsets[n] < 0)
	{
	  perror ("lseek");
	  goto err;
	}

      r = nks_read_0100_entry_header (fd, &entries[n]);
      if (r < 0)
	{
	  printf ("nks_read_0100_entry_header: %s\n", strerror (-r));
	  goto err;
	}
    }

  for (n = 0; n < count; n++)
    {
      if (!scan_0100_entry (fd, offsets[n], &entries[n], indent))
	goto err;
    }

  g_free (offsets);
  g_free (entries);
  return true;

err:
  g_free (offsets);
  g_free (entries);
  return false;
}

static bool
scan_0110_entry (int fd, off_t offset, Nks0110EntryHeader *header,
		 int indent)
{
  print_indent (indent++);

  printf ("e:%08"PRIxMAX":%08"PRIx32":%04"PRIx16":%02x%02x:%s\n",
	  (uintmax_t) offset, header->offset, header->type,
	  header->unknown[0], header->unknown[1], header->name);

  if (lseek (fd, header->offset, SEEK_SET) < 0)
    return false;

  return scan_chunk (fd, header->name, indent);
}

static bool
scan_0110_entries (int fd, uint32_t count, int indent)
{
  off_t *offsets;
  Nks0110EntryHeader *entries;
  uint32_t n;
  int r;

  entries = g_malloc0 (count * sizeof (*entries));
  offsets = g_malloc0 (count * sizeof (*offsets));

  for (n = 0; n < count; n++)
    {
      offsets[n] = lseek (fd, 0, SEEK_CUR);
      if (offsets[n] < 0)
	{
	  perror ("lseek");
	  goto err;
	}

      r = nks_read_0110_entry_header (fd, &entries[n]);
      if (r < 0)
	{
	  printf ("nks_read_0100_entry_header: %s\n", strerror (-r));
	  goto err;
	}
    }

  for (n = 0; n < count; n++)
    {
      if (!scan_0110_entry (fd, offsets[n], &entries[n], indent))
	goto err;
    }

  for (n = 0; n < count; n++)
    nks_0110_entry_header_free (&entries[n]);

  g_free (offsets);
  g_free (entries);
  return true;

err:
  g_free (offsets);

  for (n = 0; n < count; n++)
    nks_0110_entry_header_free (&entries[n]);

  g_free (entries);
  return false;
}

static bool
scan_directory (int fd, const char *name, int indent)
{
  NksDirectoryHeader header;
  off_t off;
  int n;
  int r;

  print_indent (indent++);

  off = lseek (fd, 0, SEEK_CUR);
  if (off < 0)
    {
      perror ("lseek");
      return false;
    }

  r = nks_read_directory_header (fd, &header);
  if (r < 0)
    {
      fprintf (stderr, "nks_read_directory_header: %s: %s\n",
	       name, strerror (-r));
      return false;
    }

  printf ("d:%08"PRIxMAX":%04"PRIx16":%08"PRIx32":", (uintmax_t) off,
	  header.version, header.set_id);

  for (n = 0; n < 4; n++)
    printf ("%02x", header.unknown_0[n]);

  printf (":");

  for (n = 0; n < 4; n++)
    printf ("%02x", header.unknown_1[n]);

  printf (":%08"PRIx32":%s\n", header.entry_count, name);

  switch (header.version)
    {
    case 0x0100:
      if (!scan_0100_entries (fd, header.entry_count, indent))
	return false;
      break;

    case 0x0110:
      if (!scan_0110_entries (fd, header.entry_count, indent))
	return false;
      break;
    }

  return true;
}

static bool
scan_encrypted_file (int fd, const char *name, int indent)
{
  static const uint8_t expected_data[16] = "RIFF\x00\x00\x00\x00WAVEfmt ";

  NksEncryptedFileHeader header;
  uint8_t data[16];
  off_t off;
  int r;
  int n;

  off = lseek (fd, 0, SEEK_CUR);
  if (off < 0)
    {
      perror ("lseek");
      return false;
    }

  r = nks_read_encrypted_file_header (fd, &header);
  if (r < 0)
    {
      fprintf (stderr, "nks_read_encrypted_file_header: %s\n",
	       strerror (-r));
      return false;
    }

  if (read (fd, data, sizeof (data)) != sizeof (data))
    {
      perror ("read");
      return false;
    }

  print_indent (indent++);

  printf ("x:%08"PRIxMAX":%04"PRIx16":%08"PRIx32":%08"PRIx32":%08"PRIx32":",
	  (uintmax_t) off, header.version, header.set_id, header.key_index,
	  header.size);

  for (n = 0; n < 5; n++)
    printf ("%02x", header.unknown_1[n]);

  printf (":");

  for (n = 0; n < 8; n++)
    printf ("%02x", header.unknown_2[n]);

  printf ("\n");

  print_indent (indent);

  for (n = 0; n < 16; n++)
    {
      if (n != 0)
	printf (" ");

      if (n != 0 && (n % 4) == 0)
	printf (" ");

      printf ("%02x", data[n]);
    }

  printf ("\n");

  print_indent (indent);

  for (n = 0; n < 16; n++)
    {
      if (n != 0)
	printf (" ");

      if (n != 0 && (n % 4) == 0)
	printf (" ");

      if (n < 4 || n >= 8)
	printf ("%02x", data[n] ^ expected_data[n]);
      else
	printf ("  ");
    }

  printf ("\n");

  return true;
}

static bool
scan_file (int fd, const char *name, int indent)
{
  NksFileHeader header;
  off_t off;
  int r;
  int n;

  off = lseek (fd, 0, SEEK_CUR);
  if (off < 0)
    {
      perror ("lseek");
      return false;
    }

  r = nks_read_file_header (fd, &header);
  if (r < 0)
    {
      fprintf (stderr, "nks_read_file_header: %s\n",
	       strerror (-r));
      return false;
    }

  print_indent (indent++);

  printf ("f:%08"PRIxMAX":%04"PRIx16":", (uintmax_t) off, header.version);

  for (n = 0; n < 13; n++)
    printf ("%02x", header.unknown_1[n]);

  printf (":%08"PRIx32":", header.size);

  for (n = 0; n < 4; n++)
    printf ("%02x", header.unknown_2[n]);

  printf (":%s\n", name);

  return true;
}

static bool
scan_chunk (int fd, const char *name, int indent)
{
  uint32_t magic;
  off_t off;

  off = lseek (fd, 0, SEEK_CUR);
  if (off < 0)
    return false;

  if (!read_u32_le (fd, &magic))
    return false;

  if (lseek (fd, -0x04, SEEK_CUR) < 0)
    return false;

  switch (magic)
    {
    case NKS_MAGIC_ENCRYPTED_FILE:
      return scan_encrypted_file (fd, name, indent);

    case NKS_MAGIC_DIRECTORY:
      return scan_directory (fd, name, indent);

    case NKS_MAGIC_FILE:
      return scan_file (fd, name, indent);

    default:
      print_indent (indent);
      printf ("u:%08"PRIxMAX":%08"PRIx32":%s\n", (uintmax_t) off, magic,
	      name);
      return true;
    }
}

int
main (int argc, char **argv)
{
  int fd;
  int r;

  if (argc < 2)
    {
      print_usage (argv[0]);
      return 1;
    }

  if (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0)
    {
      print_usage (argv[0]);
      return 0;
    }

  printf ("nksscan %s\n", argv[1]);

  if (!print_file_info (argv[1]))
    return 1;

  fd = open (argv[1], O_RDONLY | O_BINARY);
  if (fd < 0)
    {
      perror (argv[1]);
      return 1;
    }

  r = (scan_chunk (fd, "/", 0) ? 0 : 1);

  close (fd);
  return r;
}
