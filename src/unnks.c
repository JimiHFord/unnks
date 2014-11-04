#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <glib.h>
#include <inttypes.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "nks.h"
#include "util.h"

typedef enum
{
  OP_NONE    = 0,
  OP_LIST    = 1,
  OP_EXTRACT = 2
} Operation;

static const char  *file_name  = NULL;    /* File to open */
static const char  *directory  = NULL;    /* Directory to extract to */
static char       **file_names = NULL;    /* Files to extract or NULL */
static size_t       extr_count = 0;       /* Number of extracted files */
static Operation    operation  = OP_NONE;
static bool         verbose    = false;

static void
print_help (const char *argv0)
{
  printf_utf8 (
    "Usage: %s <OPERATION> [OPTIONS] -f ARCHIVE [FILES...]\n"
    "\n"
    "Operations:\n"
    "  -x  --extract        Extract files from archive\n"
    "  -t  --list           List files in archive\n"
    "\n"
    "  -f  --file=ARCHIVE   Operate on ARCHIVE\n"
    "\n"
    "Options:\n"
    "  -C  --directory=DIR  Extract to DIR\n"
    "  -v  --verbose        Verbose operation\n"
    "      --version        Print version and license information\n"
    "  -h  --help           Print out usage instructions\n"
    "\n"
    "e.g. to extract, use: %s -xvf archive.nks\n",
    argv0, argv0);
}

static void
print_version (void)
{
  printf_utf8 (
    PACKAGE_STRING "\n"
    "Copyright (C) 2008-2009  Unavowed <unavowed@vexillium.org>\n"
    "License GPLv3+: GNU GPL version 3 or later "
	      "<http://gnu.org/licenses/gpl.html>\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n");
}

static void
parse_arguments (int argc, char **argv)
{
  int op, index = 0;
  static struct option options[] =
  {
    {"directory", true,  NULL, 'C'},
    {"extract",   false, NULL, 'x'},
    {"file",      true,  NULL, 'f'},
    {"help",      false, NULL, 'h'},
    {"list",      false, NULL, 't'},
    {"verbose",   false, NULL, 'v'},
    {"version",   false, NULL, 'V'},
    {NULL,        false, NULL, 0}
  };

  for (;;)
    {
      op = getopt_long (argc, argv, "f:C:hxtvV", options, &index);
      if (op == -1)
	break;

      switch (op)
	{
	case 'f':
	  if (file_name != NULL)
	    {
	      fprintf_utf8 (stderr, "%s: Only one file name may be given.\n",
			    argv[0]);
	      exit (EXIT_FAILURE);
	    }
	  file_name = g_strdup (optarg);
	  break;

	case 'x':
	case 't':
	  if (operation != OP_NONE)
	    {
	      fprintf_utf8 (stderr, "%s: Only one of {extract, list} "
			    "may be given.\n", argv[0]);
	      exit (EXIT_FAILURE);
	    }
	  if (op == 'x')
	    operation = OP_EXTRACT;
	  else
	    operation = OP_LIST;
	  break;

	case 'C':
	  if (directory != NULL)
	    {
	      fprintf_utf8 (stderr, "%s: Only one directory may be given.\n",
			    argv[0]);
	      exit (EXIT_FAILURE);
	    }
	  directory = g_strdup (optarg);
	  break;

	case 'h':
	  print_help (argv[0]);
	  exit (EXIT_SUCCESS);
	  break;

	case 'v':
	  verbose = true;
	  break;

	case 'V':
	  print_version ();
	  exit (EXIT_SUCCESS);
	  break;

	default:
	  exit (EXIT_FAILURE);
	  break;
	}
    }

  if (operation == OP_NONE)
    {
      fprintf_utf8 (stderr, "%s: Nothing to do.  Try --help\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  if (optind < argc)
    file_names = argv + optind;
}

static void
free_entry_list (GList *list)
{
  GList *lp;

  for (lp = list; lp != NULL; lp = lp->next)
    {
      nks_entry_free ((NksEntry *) lp->data);
      g_free (lp->data);
    }

  g_list_free (list);
}

static bool traverse_file (Nks *nks, NksEntry *file_entry, const char *prefix);
static bool traverse_directory (Nks *nks, NksEntry *dir_entry,
				const char *prefix);

static bool
traverse_directories (Nks *nks, GList *list, const char *prefix)
{
  char prefix_buffer[FILENAME_MAX + 1];
  NksEntry *entry;
  GList *lp;
  bool ret = true;

  if (list == NULL)
    return true;

  for (lp = list; lp != NULL; lp = lp->next)
    {
      entry = lp->data;

      if (entry->type != NKS_ENT_DIRECTORY)
	continue;

      join_path_segments (prefix, entry->name,
			  prefix_buffer, sizeof (prefix_buffer));

      if (!traverse_directory (nks, entry, prefix_buffer))
	ret = false;
    }

  return ret;
}

static bool
traverse_files (Nks *nks, GList *list, const char *prefix)
{
  NksEntry *entry;
  bool ret = true;
  GList *lp;

  if (list == NULL)
    return true;

  for (lp = list; lp != NULL; lp = lp->next)
    {
      entry = lp->data;

      if (entry->type == NKS_ENT_DIRECTORY)
	continue;

      if (!traverse_file (nks, entry, prefix))
	ret = false;
    }

  return ret;
}

static bool
add_entry_to_list (Nks *nks, const NksEntry *entry, GList **list)
{
  NksEntry *new_entry;

  new_entry = g_malloc0 (sizeof (*new_entry));
  nks_entry_copy (entry, new_entry);
  *list = g_list_append (*list, new_entry);

  return true;
}

static bool
traverse_directory (Nks *nks, NksEntry *dir_entry, const char *prefix)
{
  char buffer[FILENAME_MAX + 1];
  GList *list = NULL;
  uint32_t n;
  bool ret = true;
  int r;

  assert (dir_entry->type == NKS_ENT_DIRECTORY);

  snprintf (buffer, sizeof (buffer), "%s" SEP, prefix);

  if (operation == OP_LIST)
    {
      if (prefix[0] != '\0')
	puts_utf8 (buffer);
    }

  if (operation == OP_EXTRACT && prefix[0] != '\0')
    {
      if (file_names != NULL)
	{
	  size_t len = strlen (buffer);

	  for (n = 0; file_names[n] != NULL; n++)
	    {
	      if (strncmp (buffer, file_names[n], len) == 0)
		break;
	    }

	  if (file_names[n] == NULL)
	    return true;
	}

      if (!valid_file_name (prefix))
	{
	  fprintf_utf8 (stderr, "%s: Invalid directory name.\n", prefix);
	  goto err;
	}

      if (mkdir (prefix, 0777) != 0 && errno != EEXIST)
	{
	  perror (prefix);
	  goto err;
	}

      if (verbose)
	puts_utf8 (buffer);
    }

  r = nks_list_dir_entry (nks, dir_entry,
			  (NksTraverseFunc) add_entry_to_list, &list);
  if (r != 0)
    {
      fprintf_utf8 (stderr, "%s: %s\n", buffer, strerror (-r));
      ret = false;
    }

  if (!traverse_directories (nks, list, prefix))
    ret = false;

  if (!traverse_files (nks, list, prefix))
    ret = false;

  free_entry_list (list);
  return ret;

err:
  free_entry_list (list);
  return false;
}

static bool
traverse_file (Nks *nks, NksEntry *file_entry, const char *prefix)
{
  char buffer[FILENAME_MAX + 1];
  char *file_name;
  bool ret = true;
  int r;

  assert (file_entry->type != NKS_ENT_DIRECTORY);

  join_path_segments (prefix, file_entry->name, buffer, sizeof (buffer));

  if (operation == OP_LIST)
    puts_utf8 (buffer);

  if (operation != OP_EXTRACT)
    return true;

  if (file_names != NULL)
    {
      size_t n;

      for (n = 0; file_names[n] != NULL; n++)
	{
	  if (strcmp (file_names[n], buffer) == 0)
	    break;
	}

      if (file_names[n] == NULL)
	return true;
    }

  if (!valid_file_name (buffer))
    {
      fprintf_utf8 (stderr, "%s: Invalid file name.\n", buffer);
      return false;
    }

  switch (file_entry->type)
    {
    case NKS_ENT_FILE:
      if (verbose)
	puts_utf8 (buffer);

      file_name = g_filename_from_utf8 (buffer, -1, NULL, NULL, NULL);
      if (file_name == NULL)
	file_name = buffer;

      r = nks_extract_file_entry (nks, file_entry, buffer);

      if (r == 0)
	extr_count++;
      else
	fprintf_utf8 (stderr, "%s: %s\n", buffer, strerror (-r));

      ret = (r == 0);

      if (file_name != buffer)
	g_free (file_name);

      break;

    default:
      break;
    }

  return ret;
}

int
main (int argc, char **argv)
{
  NksEntry root_entry;
  Nks *nks;
  int ret;
  int r;

  setlocale (LC_ALL, "");

  parse_arguments (argc, argv);

  if (file_name == NULL)
    {
      fprintf_utf8 (stderr, "%s: No file name given.\n", argv[0]);
      ret = EXIT_FAILURE;
      goto end;
    }

  r = nks_open (file_name, &nks);
  if (r != 0)
    {
      fprintf_utf8 (stderr, "%s: %s\n", file_name, strerror (-r));
      ret = EXIT_FAILURE;
      goto end;
    }

  if (directory != NULL)
    {
      if (chdir (directory) != 0)
	{
	  if (errno != ENOENT || mkdir (directory, 0777) != 0
	      || chdir (directory) != 0)
	    {
	      perror (directory);
	      ret = EXIT_FAILURE;
	      goto end;
	    }
	}
    }

  root_entry.name   = "";
  root_entry.offset = 0;
  root_entry.type   = NKS_ENT_DIRECTORY;

#ifdef _WIN32
  if (file_names != NULL)
    {
      size_t n, x;

      for (n = 0; file_names[n] != NULL; n++)
	{
	  for (x = 0; file_names[n][x] != 0; x++)
	    {
	      if (file_names[n][x] == '/')
		file_names[n][x] = SEP_CHAR;
	    }
	}
    }
#endif

  ret = !traverse_directory (nks, &root_entry, "");

  if (file_names != NULL)
    {
      size_t to_extract = 0;

      while (file_names[to_extract] != NULL)
	to_extract++;

      if (extr_count < to_extract)
	{
	  fprintf (stderr, "%s: Failed to extract all files\n", argv[0]);
	  ret = EXIT_FAILURE;
	}
    }

  nks_close (nks);

end:
  g_free ((char *) file_name);
  g_free ((char *) directory);

  return ret;
}
