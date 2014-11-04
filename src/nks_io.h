#ifndef NKS_IO_H
#define NKS_IO_H

#include <stdint.h>

#include "nks.h"

#define NKS_MAGIC_DIRECTORY	 UINT32_C (0x5e70ac54)
#define NKS_MAGIC_ENCRYPTED_FILE UINT32_C (0x16ccf80a)
#define NKS_MAGIC_FILE		 UINT32_C (0x4916e63c)

typedef struct
{
  uint16_t version;
  uint32_t set_id;
  uint8_t  unknown_0[4];
  uint32_t entry_count;
  uint8_t  unknown_1[4];
} NksDirectoryHeader;

typedef enum
{
  NKS_TH_DIRECTORY	= 1,
  NKS_TH_ENCRYPTED_FILE = 2,
  NKS_TH_FILE		= 3,
} NksTypeHint;

typedef struct
{
  char	   name[129];
  uint8_t  unknown[1];
  uint32_t offset;
  uint16_t type;
} Nks0100EntryHeader;

typedef struct
{
  uint8_t  unknown[2];
  uint32_t offset;
  uint16_t type;
  char 	  *name;
} Nks0110EntryHeader;

typedef struct
{
  uint16_t version;
  uint8_t  unknown_1[13];
  uint32_t size;
  uint8_t  unknown_2[4];
} NksFileHeader;

typedef struct
{
  uint16_t version;
  uint32_t set_id;
  uint32_t key_index;
  uint8_t  unknown_1[5];
  uint32_t size;
  uint8_t  unknown_2[8];
} NksEncryptedFileHeader;


int nks_read_directory_header (int fd, NksDirectoryHeader *header);
int nks_read_0100_entry_header (int fd, Nks0100EntryHeader *header);
int nks_read_0110_entry_header (int fd, Nks0110EntryHeader *header);
void nks_0110_entry_header_free (Nks0110EntryHeader *header);
int nks_read_0100_nks_entry (int fd, NksDirectoryHeader *dir, NksEntry *ent);
int nks_read_0110_nks_entry (int fd, NksDirectoryHeader *dir, NksEntry *ent);
int nks_read_encrypted_file_header (int fd, NksEncryptedFileHeader *ret);
int nks_read_file_header (int fd, NksFileHeader *ret);

#endif
