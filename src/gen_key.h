#ifndef NKS_GEN_KEY_H
#define NKS_GEN_KEY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
  uint8_t key[32];
  uint8_t key_len;
  uint8_t iv[16];
  uint8_t iv_len;
} NksGeneratingKey;

bool nks_generating_key_set_key_str (NksGeneratingKey *gk, const char *key);
bool nks_generating_key_set_iv_str (NksGeneratingKey *gk, const char *iv);
void nks_generating_key_expand (NksGeneratingKey *gk);

#endif
