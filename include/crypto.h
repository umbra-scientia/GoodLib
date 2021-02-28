/* Copyright © 2020, Mykos Hudson-Crisp <micklionheart@gmail.com>
* All rights reserved. */
#pragma once

#include "blake2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char PublicKey[24];
typedef unsigned char PrivateKey[24];
typedef unsigned char SecretKey[24];
typedef unsigned char MessageDigest[24];
typedef unsigned char Signature[52248];
typedef blake2b_state HashState;
typedef HashState* HashStatePtr;

void crypto_seed(const void* src, int len);
void crypto_random(void* dst, int len);

void crypto_keygen(PublicKey pub, const PrivateKey priv);
void crypto_sign(Signature dst, const PublicKey pub, const MessageDigest msg, const PrivateKey key);
int crypto_verify(const Signature sig, const PublicKey dst, const MessageDigest msg);

void crypto_hash(MessageDigest dst, const void* src, int len);
void crypto_hmac(MessageDigest dst, const void* src, int len, const PrivateKey key);
void crypto_cipher(void* msg, int len, MessageDigest siv, const MessageDigest key);
int crypto_decipher(void* msg, int len, const MessageDigest siv, const MessageDigest key);
HashStatePtr crypto_hash_stream(HashStatePtr stream, const void* data, int len);
void crypto_hash_finish(HashStatePtr stream, MessageDigest out);

#ifdef __cplusplus
};
#endif
