#include <glib.h>
#include <stdbool.h>
#include <stdio.h>

#include "libs.h"

static void
print_hex (const void *data, size_t len)
{
  size_t n;

  for (n = 0; n < len; n++)
    printf ("%02x", ((uint8_t *) data)[n]);
}

static gboolean
print_library_info (NksLibraryDesc *ld)
{
  printf ("%08"PRIx32" ", ld->id);
  print_hex (ld->gen_key.key, ld->gen_key.key_len);
  printf (" ");
  print_hex (ld->gen_key.iv, ld->gen_key.iv_len);
  printf (" %s\n", ld->name);
  return false;
}

int
main (void)
{
  GList *libs;

  if (nks_get_libraries (&libs) != 0)
    return 1;

  g_list_foreach (libs, (GFunc) print_library_info, NULL);

  return 0;
}
