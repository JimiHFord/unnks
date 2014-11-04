#include <errno.h>
#include <stdio.h>
#include <windows.h>

#include "libs.h"

#define REG_PATH     "Software\\Native Instruments"
#define CONTENT_PATH REG_PATH "\\Content"

static bool
read_value (HKEY key, const char *path, const char *name,
	    char *buffer, size_t len)
{
  DWORD type;
  DWORD dlen;
  DWORD r;

  dlen = len - 1;

  r = RegQueryValueEx (key, name, NULL, &type, (LPBYTE) buffer, &dlen);
  if (r != 0)
    {
      fprintf (stderr, "Failed to read HKLM\\%s\\%s.\n", path, name);
      return false;
    }

  if (type != REG_SZ)
    {
      fprintf (stderr, "Expected string at HKLM\\%s\\%s.\n", path, name);
      return false;
    }

  buffer[dlen] = '\0';

  return true;
}

static NksLibraryDesc *
create_library_desc (const char *key_name, const char *name)
{
  NksLibraryDesc *ld;
  char key_path[1024];
  char buffer[1024];
  unsigned long id;
  HKEY key;
  DWORD r;

  if (sscanf (key_name, "k2lib%lu", &id) != 1)
    {
      fprintf (stderr, "Unexpected library key: %s\n", key_name);
      return NULL;
    }

  snprintf (key_path, sizeof (key_path), REG_PATH "\\%s", name);

  r = RegOpenKeyEx (HKEY_LOCAL_MACHINE, key_path, 0, KEY_READ, &key);
  if (r != 0)
    {
      fprintf (stderr, "Failed to open HKLM\\%s.\n", key_path);
      return NULL;
    }

  ld = g_malloc0 (sizeof (*ld));
  ld->id   = id;
  ld->name = g_strdup (name);

  if (read_value (key, key_path, "JDX", buffer, sizeof (buffer)))
    nks_generating_key_set_key_str (&ld->gen_key, buffer);

  if (read_value (key, key_path, "HU", buffer, sizeof (buffer)))
    nks_generating_key_set_iv_str (&ld->gen_key, buffer);

  if (ld->gen_key.key_len == 0 || ld->gen_key.iv_len == 0)
    {
      nks_library_desc_free (ld);
      RegCloseKey (key);
      return NULL;
    }

  RegCloseKey (key);

  return ld;
}

int
nks_get_libraries (GList **rlist)
{
  GList *list = NULL;
  NksLibraryDesc *ld;
  char name[1024];
  char value[1024];
  DWORD nlen, vlen;
  DWORD type;
  HKEY key;
  LONG r;
  int n;

  r = RegOpenKeyEx (HKEY_LOCAL_MACHINE, CONTENT_PATH, 0, KEY_READ, &key);
  if (r != 0)
    {
      fprintf (stderr, "Failed to open HKLM\\" CONTENT_PATH ".\n");
      return -ENOENT;
    }

  for (n = 0;; n++)
    {
      nlen = sizeof (name);
      vlen = sizeof (value);
      r = RegEnumValue (key, n, name, &nlen, NULL,
			&type, (LPBYTE) value, &vlen);
      if (r != 0)
	break;

      if (type != REG_SZ)
	continue;
      
      ld = create_library_desc (name, value);
      if (ld == NULL)
	continue;
      
      list = g_list_append (list, ld);
    }

  *rlist = list;

  RegCloseKey (key);

  return 0;
}
