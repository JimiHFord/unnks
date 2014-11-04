#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "gen_key.h"
#include "util.h"

static bool
hex_string_to_binary (const char *str, void **rdata, size_t *rlen)
{
  size_t len;
  size_t n;
  unsigned int u;
  uint8_t *data = NULL;

  len = strlen (str);
  if ((len % 2) != 0)
    goto err;

  len /= 2;

  data = g_malloc (len);

  for (n = 0; n < len; n++)
    {
      if (sscanf (str + 2 * n, "%02x", &u) != 1)
	goto err;

      data[n] = u;
    }

  *rdata = data;
  *rlen = len;

  return true;

err:
  g_free (data);
  return false;
}

bool
nks_generating_key_set_iv_str (NksGeneratingKey *gk, const char *iv)
{
  void *data;
  size_t len;

  if (!hex_string_to_binary (iv, &data, &len))
    return false;

  if (len != 16)
    {
      g_free (data);
      return false;
    }

  memcpy (gk->iv, data, len);
  gk->iv_len = len;

  return true;
}

bool
nks_generating_key_set_key_str (NksGeneratingKey *gk, const char *key)
{
  void *data;
  size_t len;

  if (!hex_string_to_binary (key, &data, &len))
    return false;

  if (len != 16 && len != 24 && len != 32)
    {
      g_free (data);
      return false;
    }

  memcpy (gk->key, data, len);
  gk->key_len = len;

  return true;
}

void
nks_generating_key_expand (NksGeneratingKey *gk)
{
  uint32_t seed;
  size_t n;

  if (gk->key_len == 4)
    {
      gk->key_len = 32;
      seed = read_u32_be_mem (gk->key);

      for (n = 0; n < gk->key_len; n++)
	gk->key[n] = rand_ms (&seed) & 0xff;
    }

  if (gk->iv_len == 4)
    {
      gk->iv_len = 16;
      seed = read_u32_be_mem (gk->iv);

      for (n = 0; n < gk->iv_len; n++)
	gk->iv[n] = rand_ms (&seed) & 0xff;
    }
}
