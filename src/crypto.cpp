/* Copyright © 2020, Mykos Hudson-Crisp <micklionheart@gmail.com>
* All rights reserved. */
#include "crypto.h"
#include "blake2.h"
#include <malloc.h>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

#include "HashSign.hpp"
typedef HashSign<192,6,6> Hash192_Key;

char pool[64];
int nonce = 0;

void crypto_seed(const void* src, int len) {
	blake2b(pool, sizeof(pool), src, len, pool, sizeof(pool));
}
void crypto_random(void* dst, int len) {
	blake2xb(dst, len, &nonce, sizeof(nonce), pool, sizeof(pool));
	nonce++;
}
void crypto_hash(MessageDigest dst, const void* src, int len) {
	blake2b(dst, sizeof(MessageDigest), src, len, 0, 0);
}
void* crypto_hash_stream(void* ctx, const void* src, int len) {
	if (!ctx) {
		ctx = malloc(sizeof(blake2b_state));
		blake2b_init((blake2b_state*)ctx, sizeof(MessageDigest));
	}
	if (src && len) blake2b_update((blake2b_state*)ctx, src, len);
	return ctx;
}
void crypto_hash_finish(void* ctx, MessageDigest dst) {
	blake2b_final((blake2b_state*)ctx, dst, sizeof(MessageDigest));
	free(ctx);
}
void crypto_hmac(MessageDigest dst, const void* src, int len, const PrivateKey key) {
	blake2b(dst, sizeof(MessageDigest), src, len, key, sizeof(PrivateKey));
}
void crypto_cipher(void* msg, int len, MessageDigest siv, const MessageDigest key) {
	blake2b(siv, sizeof(MessageDigest), msg, len, key, sizeof(MessageDigest));
	blake2xb_xor(msg, len, siv, sizeof(MessageDigest), key, sizeof(MessageDigest));
}
int crypto_decipher(void* msg, int len, const MessageDigest siv, const MessageDigest key) {
	MessageDigest civ;
	blake2xb_xor(msg, len, siv, sizeof(MessageDigest), key, sizeof(MessageDigest));
	blake2b(civ, sizeof(MessageDigest), msg, len, key, sizeof(MessageDigest));
	volatile int total = 0;
	for(int i=0;i<sizeof(MessageDigest);i++) {
		total |= siv[i] ^ civ[i];
	}
	if (total) memset(msg, 0, len);
	return !total;
}

void crypto_keygen(PublicKey dst, const PrivateKey src) {
	Hash192_Key::keygen(dst, src);
}
void crypto_sign(Signature dst, const PublicKey pub, const MessageDigest msg, const PrivateKey key) {
	MessageDigest challenge;
	blake2s(challenge, sizeof(MessageDigest), msg, sizeof(MessageDigest), pub, sizeof(MessageDigest));
	Hash192_Key::sign_digest(dst, key, challenge);
}
int crypto_verify(const Signature sig, const PublicKey pub, const MessageDigest msg) {
	MessageDigest challenge;
	blake2s(challenge, sizeof(MessageDigest), msg, sizeof(MessageDigest), pub, sizeof(MessageDigest));
	PublicKey signer;
	Hash192_Key::verify_digest(signer, sig, challenge);
	return memcmp(signer, pub, sizeof(PublicKey)) ? 0 : 1;
}
