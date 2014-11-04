#include <glib.h>
#include <string.h>

#include "nks.h"
#include "nks_io.h"
#include "util.h"

int
nks_read_directory_header (int fd, NksDirectoryHeader *header)
{
  uint32_t magic;

  if (!read_u32_le (fd, &magic))
    return -EIO;

  if (magic != NKS_MAGIC_DIRECTORY)
    return -EILSEQ;

  if (!read_u16_le (fd, &header->version))
    return -EIO;

  if (!read_u32_le (fd, &header->set_id))
    return -EIO;

  if (read (fd, header->unknown_0, 0x04) != 0x04)
    return -EIO;

  if (!read_u32_le (fd, &header->entry_count))
    return -EIO;

  if (read (fd, header->unknown_1, 0x04) != 0x04)
    return -EIO;

  switch (header->version)
    {
    case 0x0100:
    case 0x0110:
      break;

    default:
      return -ENOTSUP;
    }

  return 0;
}

static uint32_t
decode_offset (uint32_t offset)
{
  return (offset ^ UINT32_C (0x1f4e0c8d));
}

int
nks_read_0100_entry_header (int fd, Nks0100EntryHeader *header)
{
  if (!read_string (fd, header->name, 129))
    return -EIO;

  if (read (fd, header->unknown, 0x01) != 0x01)
    return -EIO;

  if (!read_u32_le (fd, &header->offset))
    return -EIO;

  if (!read_u16_le (fd, &header->type))
    return -EIO;

  if (header->type == NKS_TH_ENCRYPTED_FILE)
    header->offset = decode_offset (header->offset);

  return 0;
}

int
nks_read_0110_entry_header (int fd, Nks0110EntryHeader *header)
{
  if (read (fd, header->unknown, 0x02) != 0x02)
    return -EIO;

  if (!read_u32_le (fd, &header->offset))
    return -EIO;

  if (!read_u16_le (fd, &header->type))
    return -EIO;

  if (!read_utf16_le_string (fd, &header->name))
    return -EIO;

  if (header->type == NKS_TH_ENCRYPTED_FILE)
    header->offset = decode_offset (header->offset);

  return 0;
}

void
nks_0110_entry_header_free (Nks0110EntryHeader *header)
{
  if (header->name != NULL)
    g_free (header->name);

  memset (header, 0, sizeof (*header));
}

static NksEntryType
type_hint_to_entry_type (uint16_t type_hint)
{
  switch (type_hint)
    {
    case NKS_TH_DIRECTORY:
      return NKS_ENT_DIRECTORY;

    case NKS_TH_ENCRYPTED_FILE:
    case NKS_TH_FILE:
      return NKS_ENT_FILE;

    default:
      return NKS_ENT_UNKNOWN;
    }
}

int
nks_read_0100_nks_entry (int fd, NksDirectoryHeader *dir, NksEntry *ent)
{
  Nks0100EntryHeader hdr;
  int r;

  r = nks_read_0100_entry_header (fd, &hdr);
  if (r != 0)
    return r;

  ent->name    = g_strdup (hdr.name);
  ent->offset  = hdr.offset;
  ent->type    = type_hint_to_entry_type (hdr.type);

  return 0;
}

int
nks_read_0110_nks_entry (int fd, NksDirectoryHeader *dir, NksEntry *ent)
{
  Nks0110EntryHeader hdr;
  int r;

  r = nks_read_0110_entry_header (fd, &hdr);
  if (r != 0)
    return r;

  ent->name    = g_strdup (hdr.name);
  ent->offset  = hdr.offset;
  ent->type    = type_hint_to_entry_type (hdr.type);

  nks_0110_entry_header_free (&hdr);

  return 0;
}

int
nks_read_file_header (int fd, NksFileHeader *ret)
{
  uint32_t magic;

  if (!read_u32_le (fd, &magic))
    return -EIO;

  if (magic != NKS_MAGIC_FILE)
    return -EILSEQ;

  if (!read_u16_le (fd, &ret->version))
    return -EIO;

  if (read (fd, ret->unknown_1, 13) != 13)
    return -EIO;

  if (!read_u32_le (fd, &ret->size))
    return -EIO;

  if (read (fd, ret->unknown_2, 4) != 4)
    return -EIO;

  return 0;
}

int
nks_read_encrypted_file_header (int fd, NksEncryptedFileHeader *ret)
{
  uint32_t magic;

  if (!read_u32_le (fd, &magic))
    return -EIO;
  
  if (magic != NKS_MAGIC_ENCRYPTED_FILE)
    return -EILSEQ;

  if (!read_u16_le (fd, &ret->version))
    return -EIO;

  if (!read_u32_le (fd, &ret->set_id))
    return -EIO;

  if (!read_u32_le (fd, &ret->key_index))
    return -EIO;

  if (read (fd, ret->unknown_1, 0x05) != 0x05)
    return -EIO;

  if (!read_u32_le (fd, &ret->size))
    return -EIO;

  if (read (fd, ret->unknown_2, 0x08) != 0x08)
    return -EIO;

  switch (ret->version)
    {
    case 0x0100:
    case 0x0110:
      break;

    default:
      return -ENOTSUP;
    }

  return 0;
}
