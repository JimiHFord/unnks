/*
 * unnks - nks and nkx archive unpacker
 * Copyright (C) 2008-2009  Unavowed <unavowed@vexillium.org>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef NKS_H__included__
#define NKS_H__included__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
  NKS_ENT_UNKNOWN,
  NKS_ENT_DIRECTORY,
  NKS_ENT_FILE,
} NksEntryType;

struct NksEntry
{
  char	      *name;
  NksEntryType type;
  off_t	       offset;
};

typedef struct NksEntry NksEntry;
typedef struct Nks Nks;

typedef bool (*NksTraverseFunc) (Nks *nks, const NksEntry *entry,
				 void *user_data);

/**
 * Opens an archive.  This must be called first, before anything else can be done
 * with archives.
 *
 * @param file_name name of the file to open
 * @param ret	    pointer to a Nks * pointer, which will be initialised upon
 * 		    success.  It has to be closed with nks_close after it is no
 * 	            longer used.
 *
 * @return 0 on success
 */                  
int nks_open (const char *file_name, Nks **ret);

/**
 * Similar to nks_open, but uses a file descriptor instead of a file name.  If
 * this function completes successfully, you must not use fd any more directly.
 * Make sure that fd is open in binary mode on systems that distinguish between
 * text and binary.
 */
int nks_open_fd (int fd, Nks **ret);

/**
 * Closes an archive.
 */
void nks_close (Nks *nks);

/**
 * Lists the contents of a directory in an archive.  It calls func for each
 * entry in the directory.  If func returns false, then then no more entries
 * will be listed.  Do not modify or free the entries provided to func.
 * If you want to copy any of the entries, you must use nks_entry_copy.
 *
 * @param nks       the archive
 * @param dir       the path to the directory to list.  It must contain slashes
 *                  ('/') as separators, and must not start with a slash.  If it
 *                  contains non-ASCII characters, it must be encoded in UTF-8.
 * @param func      the function to call for each entry
 * @param user_data an optional argument passed to func
 * 
 * @return 0 on success
 */
int nks_list_dir (Nks *nks, const char *dir, NksTraverseFunc func,
		  void *user_data);

/**
 * Lists the contents of a directory in an archive.  It is similar to nks_list,
 * but uses a NksEntry instead of a path.  The entry must correspond to a
 * directory and not a file..
 */
int nks_list_dir_entry (Nks *nks, const NksEntry *entry,
			NksTraverseFunc function, void *user_data);

/**
 * Returns the size of a file in an archive.
 *
 * @param nks   the archive
 * @param entry the entry corresponding to a file in the archive
 *
 * @return the size, or a negative value on error
 */
off_t nks_file_size (Nks *nks, const NksEntry *entry);

/**
 * Extracts a file from an archive.
 *
 * @param nks      the archive
 * @param entry    the entry corresponding to the file to extract in the archive
 * @param out_file the name of the file to extract to.  The file will be
 *                 created.
 *
 * @return 0 on success
 */
int nks_extract_file_entry (Nks *nks, const NksEntry *entry,
			    const char *out_file);

/**
 * Extracts a file from an archive.  This function is similar to
 * nks_extract_entry, but accepts an output file descriptor instead of a file
 * name.
 */
int nks_extract_file_entry_to_fd (Nks *nks, const NksEntry *entry, int out_fd);

/**
 * Finds the entry corresponding to the given path.
 *
 * @param nks  archive handle
 * @param path path to the entry
 * @param ret  pointer to a NksEntry structure to be filled in.  Must be
 * 	       disposed of with nks_entry_free.
 *
 * @return 0 on success
 */
int nks_find_entry (Nks *nks, const char *path, NksEntry *ret);

/**
 * Gets an immediate sub-entry of the given directory entry.
 *
 * @param nks   archive handle
 * @param entry directory entry
 * @param name  the name of the sub-entry
 * @param ret   pointer to a NksEntry structure to be filled in.  Must be
 *              disposed of with nks_entry_free.
 *
 * @return 0 on success
 */
int nks_get_entry (Nks *nks, const NksEntry *entry, const char *name,
		   NksEntry *ret);

/**
 * Frees an entry.
 */
void nks_entry_free (NksEntry *entry);

/**
 * Copies an entry.
 *
 * @param src pointer to the entry to copy from
 * @param dst pointer to the entry to copy to.  dst must point to a block
 *            of allocated memory.  When no longer needed, it must be freed with
 *            nks_entry_free.
 */
void nks_entry_copy (const NksEntry *src, NksEntry *dst);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
