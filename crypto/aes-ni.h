#ifndef AES_NI_H_
#define AES_NI_H_
#include <wmmintrin.h>  //for intrinsics for AES-NI
#include <stdint.h>

#define XOR64(x,y) *(uint64_t*)(x) = *(uint64_t *)(x) ^ *(uint64_t *)(y)

void aes128_load_key(__m128i *key_schedule, int8_t *enc_key);
void aes128_dec(__m128i *key_schedule, int8_t *cipherText,int8_t *plainText);

#endif
