// Copyright © 2019, Mykos Hudson-Crisp. All rights reserved.
#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "blake2.h"
#include "blake2-impl.h"

#define ZERO_MEM(X) secure_zero_memory(X, sizeof(X))

template<int lambda, int eta, int omega> struct HashSign {
	typedef unsigned char u8;
	typedef unsigned int u32;

	static const u32 hash_bytes = lambda / 8;
	static const u32 merkle_leaf_nodes = 1u << eta;
	static const u32 merkle_path_nodes = eta;
	static const u32 merkle_path_bytes = hash_bytes * merkle_path_nodes;
	static const u32 winternitz_iterations = 1u << omega;
	static const u32 winternitz_token_slices = lambda / omega;
	static const u32 winternitz_token_bytes = hash_bytes * winternitz_token_slices * 2;
	static const u32 hypertree_levels = lambda / eta;
	static const u32 signature_bytes = (hash_bytes + merkle_path_bytes)
		+ (hypertree_levels - 1) * (winternitz_token_bytes + merkle_path_bytes);
		
	typedef u8 private_key[hash_bytes];
	typedef u8 public_key[hash_bytes];
	typedef u8 signature[signature_bytes];

	static void F(u8* dst, const u8* src) {
		blake2s_state ctx;
		blake2s_init(&ctx, hash_bytes);
		blake2s_update(&ctx, src, hash_bytes);
		blake2s_final(&ctx, dst, hash_bytes);
	}
	static void G1(u8* dst, const u8* src1, const u8* src2) {
		blake2b_state ctx;
		blake2b_init(&ctx, hash_bytes);
		blake2b_update(&ctx, src1, hash_bytes);
		blake2b_update(&ctx, src2, hash_bytes);
		blake2b_final(&ctx, dst, hash_bytes);
	}
	static void G2(u8* dst, const u8* src1, const u8* src2, u32 src3) {
		blake2b_state ctx;
		blake2b_init(&ctx, hash_bytes * 2);
		blake2b_update(&ctx, src1, hash_bytes);
		blake2b_update(&ctx, src2, hash_bytes);
		blake2b_update(&ctx, &src3, 4);
		blake2b_final(&ctx, dst, hash_bytes * 2);
	}
	static void H(u8* dst, const u8* data, u32 length) {
		blake2b_state ctx;
		blake2b_init(&ctx, hash_bytes);
		blake2b_update(&ctx, data, length);
		blake2b_final(&ctx, dst, hash_bytes);
	}

	static void hash_mask(u8* mask, const u8* message, u32 bits) {
		u32 bytes = (bits + 7) / 8;
		memset(mask, 0, hash_bytes);
		memcpy(mask + hash_bytes - bytes, message + hash_bytes - bytes, bytes);
		mask[hash_bytes - (bits / 8) - 1] &= 0xFFu << (8 - (bits % 8));
	}
	static void hash_tag(u8* mask, u32 tag, u32 bits) {	
		u32 temp, cover = (1u << bits) - 1;
		memcpy(&temp, mask, 4);
		temp &= ~cover;
		temp |= tag & cover;
		memcpy(mask, &temp, 4);
	}
	static u32 hash_extract(const u8* mask, u32 offset, u32 bits) {
		u32 byte_offset = offset / 8;
		u32 bit_offset = offset % 8;
		u32 bytes = hash_bytes - byte_offset;
		if (bytes > 4) bytes = 4;
		u32 slice = 0;
		memcpy(&slice, mask + byte_offset, bytes);
		slice >>= bit_offset;
		slice &= (1u << bits) - 1;
		return slice;
	}

	static void merkle_keygen(u8* root, const u8* leaves) {
		u8 temp[hash_bytes * merkle_leaf_nodes];
		memcpy(temp, leaves, sizeof(temp));
		for(u32 i=merkle_leaf_nodes/2;i>0;i/=2) {
			for(u32 j=0;j<i;j++) {
				G1(temp + hash_bytes*j, temp + hash_bytes*(j*2+0), temp + hash_bytes*(j*2+1));
			}
		}
		memcpy(root, temp, hash_bytes);
		ZERO_MEM(temp);
	}
	static void merkle_sign(u8* root, u8* path, const u8* leaves, u32 index) {	
		u8 temp[hash_bytes * merkle_leaf_nodes];
		memcpy(temp, leaves, sizeof(temp));
		for(u32 i=merkle_leaf_nodes/2;i>0;i/=2) {
			memcpy(path, temp + (index ^ 1) * hash_bytes, hash_bytes);
			path += hash_bytes;
			index >>= 1;
			for(u32 j=0;j<i;j++) {
				G1(temp + hash_bytes*j, temp + hash_bytes*(j*2+0), temp + hash_bytes*(j*2+1));
			}
		}
		memcpy(root, temp, hash_bytes);
		ZERO_MEM(temp);
	}
	static void merkle_verify(u8* root, const u8* path, const u8* leaf, u32 index) {
		u8 temp[hash_bytes];
		memcpy(temp, leaf, hash_bytes);
		for(u32 i=0;i<merkle_path_nodes;i++) {	
			if (index & 1) {
				G1(temp, path, temp);
			} else {
				G1(temp, temp, path);
			}
			index >>= 1;
			path += hash_bytes;
		}
		memcpy(root, temp, hash_bytes);
		ZERO_MEM(temp);
	}

	static void winternitz_keygen(u8* leaves, const u8* secret, const u8* mask) {
#if defined(_OPENMP)
#pragma omp parallel for
#endif
		for(u32 leaf=0;leaf<merkle_leaf_nodes;leaf++) {
			u8 index[hash_bytes];
			u8 token[winternitz_token_bytes];
			memcpy(index, mask, hash_bytes);
			hash_tag(index, leaf, eta);
			for(u32 slice=0;slice<winternitz_token_slices;slice++) {
				u8* work = token + slice*hash_bytes*2;
				G2(work, secret, index, slice);
				for(u32 i=0;i<winternitz_iterations;i++) {
					F(work, work);
					F(work + hash_bytes, work + hash_bytes);
				}
			}
			H(leaves + leaf*hash_bytes, token, winternitz_token_bytes);
		}
	}
	static void winternitz_sign(u8* token_, const u8* secret, const u8* mask, const u8* root, u32 leaf) {
		u8 index[hash_bytes];
		memcpy(index, mask, hash_bytes);
		hash_tag(index, leaf, eta);
#if defined(_OPENMP)
#pragma omp parallel for
#endif
		for(u32 slice=0;slice<winternitz_token_slices;slice++) {
			u32 chunk = hash_extract(root, slice * omega, omega);
			u8* token = token_ + slice*hash_bytes*2;
			G2(token, secret, index, slice);
			for(u32 i=0;i<chunk;i++) {
				F(token, token);
			}
			token += hash_bytes;
			for(u32 i=0;i<winternitz_iterations-chunk;i++) {
				F(token, token);
			}
		}
	}
	static void winternitz_verify(u8* leaf, const u8* token, const u8* root) {
		u8 temp[winternitz_token_bytes];
		memcpy(temp, token, winternitz_token_bytes);
#if defined(_OPENMP)
#pragma omp parallel for
#endif
		for(u32 slice=0;slice<winternitz_token_slices;slice++) {
			u8* work = temp + slice*hash_bytes*2;
			u32 chunk = hash_extract(root, slice * omega, omega);
			for(u32 i=0;i<winternitz_iterations-chunk;i++) {
				F(work, work);
			}
			work += hash_bytes;
			for(u32 i=0;i<chunk;i++) {
				F(work, work);
			}
		}
		H(leaf, temp, winternitz_token_bytes);
	}

	static void lamport_keygen(u8* leaves, const u8* secret, const u8* mask) {
		u8 index[hash_bytes];
		memcpy(index, mask, hash_bytes);
		for(u32 i=0;i<merkle_leaf_nodes;i++) {
			hash_tag(index, i, eta);
			G1(leaves, secret, index);
			F(leaves, leaves);
			leaves += hash_bytes;
		}
	}
	static void lamport_sign(u8* token, const u8* secret, const u8* message) {
		G1(token, secret, message);
	}
	static void lamport_verify(u8* leaf, const u8* token) {
		F(leaf, token);
	}

	static void keygen(public_key pub, const private_key priv) {
		u8 blank[hash_bytes];
		u8 leaves[merkle_leaf_nodes * hash_bytes];
		memset(blank, 0, hash_bytes);
		winternitz_keygen(leaves, priv, blank);
		merkle_keygen(pub, leaves);
		ZERO_MEM(leaves);
	}

	static void sign_digest(u8* signature, const private_key priv, const u8* message) {
		u8 secret[hash_bytes], mask[hash_bytes], root[hash_bytes];
		u8 leaves[merkle_leaf_nodes * hash_bytes];
		u32 level = hypertree_levels - 1;
		memcpy(secret, priv, hash_bytes);
		secret[0] = priv[0] + level;
		u32 index = hash_extract(message, 0, eta);
		hash_mask(mask, message, level * eta);
		lamport_keygen(leaves, secret, mask);

		lamport_sign(signature, secret, message);
		signature += hash_bytes;
		merkle_sign(root, signature, leaves, index);	
		signature += merkle_path_bytes;

		while (level--) {
			secret[0]--;
			index = hash_extract(message, level * eta, eta);
			hash_mask(mask, message, level * eta);

			winternitz_keygen(leaves, secret, mask);
			winternitz_sign(signature, secret, mask, root, index);
			signature += winternitz_token_bytes;

			merkle_sign(root, signature, leaves, index);
			signature += merkle_path_bytes;
		}
		ZERO_MEM(secret);
		ZERO_MEM(leaves);
	}

	static void verify_digest(u8* out, const u8* signature, const u8* message) {
		u8 leaf[hash_bytes], root[hash_bytes];

		lamport_verify(leaf, signature);
		signature += hash_bytes;

		u32 level = hypertree_levels - 1;
		u32 index = hash_extract(message, 0, eta);
		merkle_verify(root, signature, leaf, index);
		signature += merkle_path_bytes;
	
		while (level--) {
			winternitz_verify(leaf, signature, root);
			signature += winternitz_token_bytes;

			index = hash_extract(message, level * eta, eta);
			merkle_verify(root, signature, leaf, index);
			signature += merkle_path_bytes;
		}
		
		memcpy(out, root, hash_bytes);
	}

	static bool verify(const u8* signature, const public_key pub, const void* message, size_t length) {
		u8 digest[hash_bytes], root[hash_bytes];
		H(digest, (u8*)message, length);
		verify_digest(root, signature, digest);
		return memcmp(pub, root, hash_bytes) == 0;
	}

	static void sign(u8* signature, const private_key priv, const void* message, size_t length) {
		u8 digest[hash_bytes];
		H(digest, (u8*)message, length);
		sign_digest(signature, priv, digest);
	}
};

#undef ZERO_MEM
