#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libs.h"

#include "lib_data.c"

void
nks_library_desc_free (NksLibraryDesc *desc)
{
  g_free (desc->name);
  g_free (desc);
}

static int
compare_libraries (const void *a, const void *b)
{
  return (((NksLibraryDesc *) a)->id - ((NksLibraryDesc *) b)->id);
}

const NksLibraryDesc *
nks_get_library_desc (uint32_t id)
{
  NksLibraryDesc l;
  NksLibraryDesc *lib;

  l.id = id;
  lib = bsearch (&l, libraries, G_N_ELEMENTS (libraries),
		 sizeof (libraries[0]), &compare_libraries);

  if (lib == NULL)
    return lib;

  nks_generating_key_expand (&lib->gen_key);

  return lib;
}
