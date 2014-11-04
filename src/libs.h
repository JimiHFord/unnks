#ifndef NKS_LIBS_H
#define NKS_LIBS_H

#include <glib.h>
#include <inttypes.h>
#include <stdbool.h>

#include "keys.h"

typedef struct
{
  uint32_t 	   id;
  char    	  *name;
  NksGeneratingKey gen_key;
} NksLibraryDesc;

void nks_library_desc_free (NksLibraryDesc *desc);
int nks_get_libraries (GList **list);
const NksLibraryDesc *nks_get_library_desc (uint32_t id);

#endif
