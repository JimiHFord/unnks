#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "keys.h"
#include "libs.h"
#include "nks.h"
#include "nks_io.h"
#include "util.h"

typedef struct
{
  uint32_t set_id;
  uint8_t  data[0x10000];
} NksSetKey;

struct Nks
{
  int	   fd;
  NksEntry root_entry;
  GTree	   *set_keys;
};

static int
compare_pu32 (uint32_t *a, uint32_t *b)
{
  return (*a - *b);
}

int
nks_open (const char *file_name, Nks **ret)
{
  int fd;
  int r;

  fd = open (file_name, O_RDONLY | O_BINARY);
  if (fd < 0)
    return -errno;

  r = nks_open_fd (fd, ret);
  if (r != 0)
    {
      close (fd);
      return r;
    }

  return 0;
}

int
nks_open_fd (int fd, Nks **ret)
{
  Nks *nks;

  assert (ret != NULL);

  if (fd < 0)
    return -EINVAL;

  nks = g_malloc0 (sizeof (*nks));
  nks->root_entry.name   = "/";
  nks->root_entry.type   = NKS_ENT_DIRECTORY;
  nks->root_entry.offset = 0;
  nks->fd		 = fd;
  nks->set_keys		 = g_tree_new_full ((GCompareDataFunc) &compare_pu32,
					    NULL, NULL, &g_free);

  *ret = nks;

  return 0;
}

void
nks_close (Nks *nks)
{
  assert (nks != NULL);
  assert (nks->fd >= 0);

  g_tree_destroy (nks->set_keys);

  close (nks->fd);

  memset (nks, 0, sizeof (*nks));
  nks->fd = -1;

  g_free (nks);
}

typedef struct
{
  char	   *name;
  NksEntry *entry;
  bool	    found;
} FindEntryContext;

static bool
nks_find_sub_entry (const NksEntry *ent, FindEntryContext *ctx)
{
  char *folded;

  folded = g_utf8_casefold (ent->name, -1);

  if (g_utf8_collate (ctx->name, folded) == 0)
    {
      nks_entry_copy (ent, ctx->entry);
      ctx->found = true;
      g_free (folded);
      return false;
    }

  g_free (folded);
  return true;
}

int
nks_get_entry (Nks *nks, const NksEntry *entry, const char *name,
	       NksEntry *ret)
{
  FindEntryContext context;
  int r;

  context.name  = g_utf8_casefold (name, -1);
  context.entry = ret;
  context.found = false;

  r = nks_list_dir_entry (nks, entry, (NksTraverseFunc) nks_find_sub_entry,
			  &context);
  g_free (context.name);

  if (r != 0)
    return r;

  if (!context.found)
    return -ENOENT;

  return 0;
}

int
nks_find_entry (Nks *nks, const char *name, NksEntry *ret)
{
  NksEntry entry;
  NksEntry sub_entry;
  char buffer[FILENAME_MAX];
  int r;

  if (name == NULL || name[0] == '/')
    return -EINVAL;

  nks_entry_copy (&nks->root_entry, &entry);

  while (name != NULL)
    {
      r = extract_path_segment (name, buffer, sizeof (buffer), &name);
      if (r != 0)
	goto err;

      if (buffer[0] == 0)
	{
	  if (entry.type != NKS_ENT_DIRECTORY)
	    {
	      r = -ENOTDIR;
	      goto err;
	    }

	  break;
	}

      r = nks_get_entry (nks, &entry, buffer, &sub_entry);
      if (r != 0)
	goto err;

      nks_entry_free (&entry);
      entry = sub_entry;
    }

  *ret = entry;
  return 0;

err:
  nks_entry_free (&entry);
  return r;
}

int
nks_list_dir (Nks *nks, const char *dir, NksTraverseFunc func, void *user_data)
{
  NksEntry ent;
  int r;

  r = nks_find_entry (nks, dir, &ent);
  if (r != 0)
    return r;

  if (ent.type != NKS_ENT_DIRECTORY)
    return -ENOTDIR;

  r = nks_list_dir_entry (nks, &ent, func, user_data);
  nks_entry_free (&ent);

  return r;
}

static int
list_directory (Nks *nks, NksDirectoryHeader *header,
		NksTraverseFunc func, void *user_data)
{
  off_t offset;
  NksEntry ent;
  uint32_t n;
  int r;

  offset = lseek (nks->fd, 0, SEEK_CUR);
  if (offset < 0)
    return -EIO;

  for (n = 0; n < header->entry_count; n++)
    {
      if (lseek (nks->fd, offset, SEEK_SET) < 0)
	return -EIO;

      switch (header->version)
	{
	case 0x0100:
	  r = nks_read_0100_nks_entry (nks->fd, header, &ent);
	  break;

	case 0x0110:
	  r = nks_read_0110_nks_entry (nks->fd, header, &ent);
	  break;

	default:
	  r = -ENOTSUP;
	  break;
	}

      if (r != 0)
	return r;

      offset = lseek (nks->fd, 0, SEEK_CUR);
      if (offset < 0)
	{
	  nks_entry_free (&ent);
	  return -EIO;
	}

      if (!func ((Nks *) nks, &ent, user_data))
	{
	  nks_entry_free (&ent);
	  break;
	}

      nks_entry_free (&ent);
    }
  
  return 0;
}

int
nks_list_dir_entry (Nks *nks, const NksEntry *entry, NksTraverseFunc func,
		    void *user_data)
{
  NksDirectoryHeader header;
  int r;

  if (entry->type != NKS_ENT_DIRECTORY)
    return -ENOTDIR;

  if (lseek (nks->fd, entry->offset, SEEK_SET) < 0)
    return -EIO;

  r = nks_read_directory_header (nks->fd, &header);
  if (r != 0)
    return r;

  return list_directory (nks, &header, func, user_data);
}

static void
allocate_file_space (int fd, off_t size)
{
#ifdef HAVE_POSIX_FALLOCATE
  posix_fallocate (fd, 0, size);
#endif
}

static int
extract_encrypted_file_entry_to_fd
  (Nks *nks, const NksEncryptedFileHeader *header, int out_fd)
{
  char buffer[16384];
  const uint8_t *key;
  size_t count;
  size_t size;
  size_t to_read;
  size_t key_length;
  off_t key_pos;
  size_t x;
  int r;

  if (header->key_index < 0xff)
    {
      if (nks_get_0100_key (header->key_index, &key, &key_length) != 0)
	return -ENOKEY;

      assert (key_length == 0x10);

      key_pos = lseek (nks->fd, 0, SEEK_CUR);
      if (key_pos < 0)
	return -EIO;
    }
  else if (header->key_index == 0x100)
    {
      const NksLibraryDesc *lib;
      NksSetKey *set_key;

      set_key = g_tree_lookup (nks->set_keys, &header->set_id);
      if (set_key == NULL)
	{
	  lib = nks_get_library_desc (header->set_id);
	  if (lib == NULL)
	    return -ENOKEY;

	  set_key = g_malloc (sizeof (*set_key));
	  set_key->set_id = header->set_id;
	  r = nks_create_0110_key (&lib->gen_key, set_key->data,
				   sizeof (set_key->data));
	  if (r != 0)
	    {
	      g_free (set_key);
	      return r;
	    }

	  g_tree_insert (nks->set_keys, &set_key->set_id, set_key);
	}

      key = set_key->data;
      key_length = 0x10000;

      key_pos = 0;
    }
  else
    return -ENOKEY;

  size = header->size;

  allocate_file_space (out_fd, size);

  while (size > 0)
    {
      to_read = MIN (sizeof (buffer), size);
      count = read (nks->fd, buffer, to_read);
      if (count != to_read)
	return -EIO;

      for (x = 0; x < to_read; x++)
	{
	  key_pos %= key_length;
	  buffer[x] ^= key[key_pos];
	  key_pos++;
	}

      count = write (out_fd, buffer, to_read);
      if (count != to_read)
	return -EIO;

      size -= to_read;
    }

  return 0;
}

static int
extract_file_entry_to_fd (Nks *nks, const NksFileHeader *header, int out_fd)
{
  char buffer[16384];
  size_t to_read;
  size_t size;
  size_t count;

  size = header->size;
  allocate_file_space (out_fd, size);

  while (size > 0)
    {
      to_read = MIN (sizeof (buffer), size);
      count = read (nks->fd, buffer, to_read);
      if (count != to_read)
	return -EIO;

      count = write (out_fd, buffer, to_read);
      if (count != to_read)
	return -EIO;

      size -= to_read;
    }

  return 0;
}

int
nks_extract_file_entry_to_fd (Nks *nks, const NksEntry *entry, int out_fd)
{
  NksEncryptedFileHeader enc_header;
  NksFileHeader file_header;
  uint32_t magic;
  int r;

  if (lseek (nks->fd, entry->offset, SEEK_SET) < 0)
    return -EIO;

  if (!read_u32_le (nks->fd, &magic))
    return -EIO;

  switch (magic)
    {
    case NKS_MAGIC_ENCRYPTED_FILE:
    case NKS_MAGIC_FILE:
      break;

    case NKS_MAGIC_DIRECTORY:
      return -EISDIR;

    default:
      return -ENOTSUP;
    }

  if (lseek (nks->fd, entry->offset, SEEK_SET) < 0)
    return -EIO;

  if (magic == NKS_MAGIC_ENCRYPTED_FILE)
    {
      r = nks_read_encrypted_file_header (nks->fd, &enc_header);
      if (r != 0)
	return r;

      switch (enc_header.version)
	{
	case 0x0100:
	case 0x0110:
	  return extract_encrypted_file_entry_to_fd (nks, &enc_header, out_fd);

	default:
	  return -ENOTSUP;
	}
    }
  else
    {
      r = nks_read_file_header (nks->fd, &file_header);
      if (r != 0)
	return r;

      switch (file_header.version)
	{
	case 0x0100:
	case 0x0110:
	  return extract_file_entry_to_fd (nks, &file_header, out_fd);

	default:
	  return -ENOTSUP;
	}
    }
}

off_t
nks_file_size (Nks *nks, const NksEntry *entry)
{
  NksEncryptedFileHeader header;
  uint32_t magic;
  int r;

  if (lseek (nks->fd, entry->offset, SEEK_SET) < 0)
    return -EIO;

  if (!read_u32_le (nks->fd, &magic))
    return -EIO;

  switch (magic)
    {
    case NKS_MAGIC_ENCRYPTED_FILE:
      break;

    case NKS_MAGIC_DIRECTORY:
      return -EISDIR;

    default:
      return -ENOTSUP;
    }

  if (lseek (nks->fd, entry->offset, SEEK_SET) < 0)
    return -EIO;

  r = nks_read_encrypted_file_header (nks->fd, &header);
  if (r != 0)
    return r;

  return header.size;
}

int
nks_extract_file_entry (Nks *nks, const NksEntry *entry, const char *out_file)
{
  int fd;
  int r;

  fd = open (out_file, O_WRONLY | O_CREAT | O_BINARY, 0666);
  if (fd < 0)
    return -errno;

  r = nks_extract_file_entry_to_fd (nks, entry, fd);
  close (fd);

  if (r != 0)
    unlink (out_file);

  return r;
}

void
nks_entry_copy (const NksEntry *src, NksEntry *dst)
{
  dst->name   = g_strdup (src->name);
  dst->type   = src->type;
  dst->offset = src->offset;
}

void
nks_entry_free (NksEntry *entry)
{
  g_free (entry->name);
  memset (entry, 0, sizeof (*entry));
}
