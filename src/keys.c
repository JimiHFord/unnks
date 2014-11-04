#include <gcrypt.h>
#include <glib.h>
#include <stdlib.h>

#include "keys.h"
#include "util.h"

static uint8_t  nks_0100_keys[32][16];
static uint8_t *nks_0110_base_key;

static void
generate_0100_keys (void)
{
  uint32_t seed = UINT32_C (0x6ee38fe0);
  int key, n;

  for (key = 0; key < 32; key++)
    {
      for (n = 0; n < 16; n++)
	nks_0100_keys[key][n] = rand_ms (&seed) & 0xff;
    }
}

int
nks_get_0100_key (uint32_t key_index, const uint8_t **key, size_t *length)
{
  void *ret_key = NULL;
  size_t ret_length = 0;

  if (key_index >= 0x20)
    return -ENOKEY;

  if (nks_0100_keys[0][0] == 0)
    generate_0100_keys ();

  ret_key    = nks_0100_keys[key_index];
  ret_length = 0x10;

  if (key != NULL)
    *key = ret_key;

  if (length != NULL)
    *length = ret_length;

  return 0;
}

static void
generate_0110_base_key (void)
{
  int n;
  uint32_t seed;
  
  if (nks_0110_base_key != NULL)
    return;

  nks_0110_base_key = g_malloc (0x10000);
  seed = UINT32_C (0x608da0a2);

  for (n = 0; n < 0x10000; n++)
    nks_0110_base_key[n] = rand_ms (&seed) & 0xff;
}

static int
initialise_gcrypt (void)
{
  if (gcry_control (GCRYCTL_INITIALIZATION_FINISHED_P))
    return 0;

  if (!gcry_check_version (GCRYPT_VERSION))
    {
      fprintf (stderr, "Error: Incompatible gcrypt version.\n");
      return -ENOTSUP;
    }

  gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
  gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);

  return 0;
}

static void
increment_counter (uint8_t *num, size_t len)
{
  size_t n;

  for (n = len - 1; n > 0; n--)
    {
      if (++num[n] != 0)
	break;
    }
}

int
nks_create_0110_key (const NksGeneratingKey *gk, void *buffer, size_t len)
{
  gcry_cipher_hd_t cipher;
  uint8_t ctr[16];
  uint8_t *bkp;
  uint8_t *bp;
  size_t n, m;
  int algo;
  int r;

  if (gk->key == NULL)
    return -EINVAL;

  if (gk->key_len != 16 && gk->key_len != 24 && gk->key_len != 32)
    return -EINVAL;

  if (gk->iv == NULL || gk->iv_len != 16)
    return -EINVAL;

  if (buffer == NULL || len < 16 || (len & 15) != 0)
    return -EINVAL;

  r = initialise_gcrypt ();
  if (r != 0)
    return r;

  if (nks_0110_base_key == NULL)
    generate_0110_base_key ();

  switch (gk->key_len)
    {
    case 16: algo = GCRY_CIPHER_AES128; break;
    case 24: algo = GCRY_CIPHER_AES192; break;
    case 32: algo = GCRY_CIPHER_AES256; break;
    default: abort (); break;
    }

  if (gcry_cipher_open (&cipher, algo, GCRY_CIPHER_MODE_ECB, 0) != 0)
    return -ENOTSUP;

  if (gcry_cipher_setkey (cipher, gk->key, gk->key_len) != 0)
    {
      r = -ENOTSUP;
      goto err;
    }

  memcpy (ctr, gk->iv, 16);

  bp = buffer;
  bkp = nks_0110_base_key;

  for (n = 0; 16 * n < len; n++)
    {
      if (gcry_cipher_encrypt (cipher, bp, 16, ctr, 16) != 0)
	{
	  r = -ENOTSUP;
	  goto err;
	}

      for (m = 0; m < 16; m++)
	bp[m] ^= bkp[m];

      increment_counter (ctr, 16);

      bp += 16;
      bkp += 16;
    }

  gcry_cipher_close (cipher);

  return 0;

err:
  gcry_cipher_close (cipher);
  return r;
}
