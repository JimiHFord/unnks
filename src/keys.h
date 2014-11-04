#ifndef NKS_KEYS_H
#define NKS_KEYS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gen_key.h"

int nks_get_0100_key (uint32_t key_index, const uint8_t **key, size_t *length);
int nks_create_0110_key (const NksGeneratingKey *gen_key,
			 void *buffer, size_t len);

#endif
