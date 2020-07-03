/******************************************************************************
  Filename:       crypto.c
  Description:    This file contains the implementation of crypto APIs.
  
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
 

/*
 * MIIO BLE Include
 */

#include "mible_crypto.h"
#include "ry_utils.h"


/*********************************************************************
 * MACROS
 */


/*********************************************************************
 * CONSTANTS
 */


/*********************************************************************
 * TYPEDEFS
 */
struct rc4_state {
    u8  perm[256];
    u8  index1;
    u8  index2;
};


/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */


/*********************************************************************
 * LOCAL VARIABLES
 */


static void swap_bytes(u8 *a, u8 *b)
{
    u8 temp;

    temp = *a;
    *a = *b;
    *b = temp;
}

/*
 * Initialize an RC4 state buffer using the supplied key,
 * which can have arbitrary length.
 */
static void rc4_init(struct rc4_state *const state, const u8 *key, int keylen)
{
    u8 j;
    int i;

    /* Initialize state with identity permutation */
    for (i = 0; i < 256; i++)
        state->perm[i] = (u8)i; 
    state->index1 = 0;
    state->index2 = 0;

    /* Randomize the permutation using key data */
    for (j = i = 0; i < 256; i++) {
        j += state->perm[i] + key[i % keylen]; 
        swap_bytes(&state->perm[i], &state->perm[j]);
    }
}


/*
 * Encrypt some data using the supplied RC4 state buffer.
 * The input and output buffers may be the same buffer.
 * Since RC4 is a stream cypher, this function is used
 * for both encryption and decryption.
 */
static void rc4_crypt(struct rc4_state *const state,
    u8 *inbuf, u8 *outbuf, int buflen)
{
    int i;
    u8 j;

    for (i = 0; i < buflen; i++) {

        /* Update modification indicies */
        state->index1++;
        state->index2 += state->perm[state->index1];

        /* Modify permutation */
        swap_bytes(&state->perm[state->index1],
            &state->perm[state->index2]);

        /* Encrypt/decrypt next byte */
        j = state->perm[state->index1] + state->perm[state->index2];
        outbuf[i] = inbuf[i] ^ state->perm[j];
    }
}

#define TOKEN_LEN  12


/*
 * miio_ble_mix_1 - Algorithm to generate t1 from token. 
 *             t1 is used in key exchange.
 *
 * @param[in]   in   - The input token
 * @param[in]   mmac - Master public address
 * @param[in]   smac - Slave public address
 * @param[in]   pid  - Product ID
 * @param[out]  out  - The generated output token
 */
void mible_mix_1(u8* in, u8* mmac, u8* smac, u8* pid, u8* out)
{
    struct rc4_state state;
    int i;
    
    u8 key[8];
    key[0] = smac[0];
    key[1] = smac[2];
    key[2] = smac[5];
    key[3] = pid[0];
    key[4] = pid[0];
    key[5] = smac[4];
    key[6] = smac[5];
    key[7] = smac[1];

    LOG_DEBUG("key1: ");
    ry_data_dump(key, 8, ' ');

    /* RC4 encrypt */
    rc4_init(&state, key, 8);
    rc4_crypt(&state, in, out, TOKEN_LEN);
}


/*
 * miio_ble_mix_2 - Algorithm to generate t2 from t1. 
 *             t2 is used in key exchange.
 *
 * @param[in]   in   - The input token
 * @param[in]   mmac - Master public address
 * @param[in]   smac - Slave public address
 * @param[in]   pid  - Product ID
 * @param[out]  out  - The generated output token
 */
void mible_mix_2(u8* in, u8* mmac, u8* smac, u8* pid, u8* out)
{
    struct rc4_state state;
    int i;
    
    u8 key[8];
    key[0] = smac[0];
    key[1] = smac[2];
    key[2] = smac[5];
    key[3] = pid[1];
    key[4] = smac[4];
    key[5] = smac[0];
    key[6] = smac[5];
    key[7] = pid[0];

    LOG_DEBUG("key2: ");
    ry_data_dump(key, 8, ' ');

    /* RC4 encrypt */
    rc4_init(&state, key, 8);
    rc4_crypt(&state, in, out, TOKEN_LEN);
}

/*
 * miio_ble_encrypt - Encryption API in mi service
 *
 * @param[in]   in     - The input plain text
 * @param[in]   inLen  - Length of the input text
 * @param[in]   key    - Key used to encrypt
 * @param[in]   keyLen - Length of the key
 * @param[out]  out    - The output cipher text
 */
void mible_internal_encrypt(u8* in, int inLen, u8* key, int keyLen, u8* out)
{
    struct rc4_state state;

    /* RC4 encrypt */
    rc4_init(&state, key, keyLen);
    rc4_crypt(&state, in, out, inLen);
}


