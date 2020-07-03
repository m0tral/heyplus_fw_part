#ifndef __MIBLE_CRYPTO_H__
#define __MIBLE_CRYPTO_H__

#include "ry_type.h"

/*
 * mible_mix_1 - Algorithm to generate t1 from token. 
 *             t1 is used in key exchange.
 *
 * @param[in]   in   - The input token
 * @param[in]   mmac - Master public address
 * @param[in]   smac - Slave public address
 * @param[in]   pid  - Product ID
 * @param[out]  out  - The generated output token
 */
void mible_mix_1(u8* in, u8* mmac, u8* smac, u8* pid, u8* out);



/*
 * mible_mix_2 - Algorithm to generate t2 from t1. 
 *             t2 is used in key exchange.
 *
 * @param[in]   in   - The input token
 * @param[in]   mmac - Master public address
 * @param[in]   smac - Slave public address
 * @param[in]   pid  - Product ID
 * @param[out]  out  - The generated output token
 */
void mible_mix_2(u8* in, u8* mmac, u8* smac, u8* pid, u8* out);

/*
 * miio_ble_encrypt - Encryption API in mi service
 *
 * @param[in]   in     - The input plain text
 * @param[in]   inLen  - Length of the input text
 * @param[in]   key    - Key used to encrypt
 * @param[in]   keyLen - Length of the key
 * @param[out]  out    - The output cipher text
 */
void mible_internal_encrypt(u8* in, int inLen, u8* key, int keyLen, u8* out);

/*
 * miio_ble_decrypt - Encryption API in mi service
 *
 * @param[in]   in     - The input cipher text
 * @param[in]   inLen  - Length of the input text
 * @param[in]   key    - Key used to encrypt
 * @param[in]   keyLen - Length of the key
 * @param[out]  out    - The output plain text
 */
#define mible_internal_decrypt    mible_internal_encrypt




#endif  /* __MIBLE_CRYPTO_H__ */




