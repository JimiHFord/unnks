#include <CoreFoundation/CoreFoundation.h>
#include <errno.h>
#include <glib.h>

#include "libs.h"

#define PACKAGE_PREFIX "com.native-instruments"

typedef struct
{
  GTree *libs;
  GList *path_list;
} Context;

static bool
print_cfstring (FILE *file, CFStringRef str)
{
  char buffer[1024];

  if (str == NULL)
    return false;

  if (!CFStringGetCString
         (str, buffer, sizeof (buffer), kCFStringEncodingUTF8))
    return false;

  fprintf (file, "%s", buffer);
  return true;
}

static CFPropertyListRef
read_plist (const char *file_name)
{
  CFURLRef url = NULL;
  CFDataRef data = NULL;
  CFStringRef name_str = NULL;
  CFStringRef error_str = NULL;
  CFPropertyListRef plist = NULL;

  name_str = CFStringCreateWithCString (NULL, file_name, kCFStringEncodingUTF8);
  url = CFURLCreateWithFileSystemPath
          (NULL, name_str, kCFURLPOSIXPathStyle, false);

  if (!CFURLCreateDataAndPropertiesFromResource
         (NULL, url, &data, NULL, NULL, NULL))
    goto err;

  plist = CFPropertyListCreateFromXMLData
            (NULL, data, kCFPropertyListImmutable, &error_str);
  if (plist == NULL)
    {
      fprintf (stderr, "%s: ", file_name);
      print_cfstring (stderr, error_str);
      fprintf (stderr, ".\n");

      if (error_str != NULL)
	CFRelease (error_str);

      goto err;
    }

  CFRelease (data);
  CFRelease (name_str);
  CFRelease (url);

  return plist;

err:
  if (data != NULL)
    CFRelease (data);

  if (url != NULL)
    CFRelease (url);

  if (name_str != NULL)
    CFRelease (name_str);

  return NULL;
}

static GList *
create_path_list (void)
{
  GList *path_list = NULL;
  char home_prefs[1024];
  char *home;

  path_list = g_list_append (path_list, g_strdup ("/Library/Preferences"));

  home = getenv ("HOME");
  if (home != NULL)
    {
      snprintf (home_prefs, sizeof (home_prefs), "%s/Preferences", home);
      path_list = g_list_append (path_list, g_strdup (home_prefs));
    }

  return path_list;
}

static int
compare_uint32 (uint32_t *n, uint32_t *m)
{
  return (*n - *m);
}

static void
process_content_entry (Context *c, CFPropertyListRef plist, CFStringRef key_str)
{
  CFStringRef value_str;
  NksLibraryDesc *ld;
  char key[1024];
  char value[1024];
  unsigned long ul_id;
  uint32_t id;

  if (!CFStringGetCString (key_str, key, sizeof (key), kCFStringEncodingUTF8))
    return;

  if (sscanf (key, "k2lib%lu", &ul_id) != 1)
    return;

  id = ul_id;
  value_str = CFDictionaryGetValue (plist, key_str);

  if (CFGetTypeID (value_str) != CFStringGetTypeID ())
    return;

  if (!CFStringGetCString (value_str, value, sizeof (value),
			   kCFStringEncodingUTF8))
    return;

  ld = g_tree_lookup (c->libs, &id);
  if (ld != NULL)
    return;

  ld = g_malloc0 (sizeof (*ld));
  ld->id = id;
  ld->name = g_strdup (value);
  g_tree_insert (c->libs, &ld->id, ld);
}

static bool
process_content_file (Context *c, CFPropertyListRef plist)
{
  CFStringRef *keys;
  CFIndex n;

  if (CFGetTypeID (plist) != CFDictionaryGetTypeID ())
    return false;

  keys = g_malloc0 (CFDictionaryGetCount (plist) * sizeof (CFStringRef));
  CFDictionaryGetKeysAndValues (plist, (const void **) keys, NULL);

  for (n = 0; n < CFDictionaryGetCount (plist); n++)
    {
      if (CFGetTypeID (keys[n]) != CFStringGetTypeID ())
	continue;

      process_content_entry (c, plist, keys[n]);
    }

  g_free (keys);
  return true;
}

static void
process_content_files (Context *c)
{
  CFPropertyListRef plist;
  char content_file[1024];
  GList *l;

  for (l = c->path_list; l != NULL; l = l->next)
    {
      snprintf (content_file, sizeof (content_file),
		"%s/" PACKAGE_PREFIX ".Content.plist", (char *) l->data);

      plist = read_plist (content_file);
      if (plist == NULL)
	continue;

      process_content_file (c, plist);
      CFRelease (plist);
    }
}

static bool
get_string (CFDictionaryRef dict, CFStringRef key, char *buffer, size_t len)
{
  CFStringRef str;

  str = CFDictionaryGetValue (dict, key);
  if (str == NULL || CFGetTypeID (str) != CFStringGetTypeID ())
    return false;

  if (!CFStringGetCString (str, buffer, len, kCFStringEncodingUTF8))
    return false;

  return true;
}

static bool
process_library_file (Context *c, CFPropertyListRef plist, NksLibraryDesc *ld)
{
  char buffer[1024];

  if (CFGetTypeID (plist) != CFDictionaryGetTypeID ())
    return false;

  if (!get_string (plist, CFSTR ("JDX"), buffer, sizeof (buffer)))
    return false;

  nks_generating_key_set_key_str (&ld->gen_key, buffer);

  if (!get_string (plist, CFSTR ("HU"), buffer, sizeof (buffer)))
    return false;

  nks_generating_key_set_iv_str (&ld->gen_key, buffer);

  return true;
}

static gboolean
process_library (uint32_t *id, NksLibraryDesc *ld, Context *c)
{
  CFPropertyListRef plist;
  char plist_name[1024];
  GList *l;

  for (l = c->path_list; l != NULL; l = l->next)
    {
      snprintf (plist_name, sizeof (plist_name),
		"%s/" PACKAGE_PREFIX ".%s.plist", (char *) l->data, ld->name);

      plist = read_plist (plist_name);
      if (plist == NULL)
	continue;

      process_library_file (c, plist, ld);
      CFRelease (plist);
    }

  return false;
}

gboolean
add_library_to_list (uint32_t *id, NksLibraryDesc *ld, GList **list)
{
  if (ld->name == NULL || ld->gen_key.key == NULL || ld->gen_key.iv == NULL)
    {
      nks_library_desc_free (ld);
      return false;
    }

  *list = g_list_append (*list, ld);

  return false;
}

int
nks_get_libraries (GList **rlist)
{
  Context c;

  c.path_list = create_path_list ();
  c.libs = g_tree_new ((GCompareFunc) &compare_uint32);

  process_content_files (&c);
  g_tree_foreach (c.libs, (GTraverseFunc) &process_library, &c);

  *rlist = NULL;
  g_tree_foreach (c.libs, (GTraverseFunc) &add_library_to_list, rlist);

  g_list_free (c.path_list);
  g_tree_destroy (c.libs);

  return 0;
}
