/**
 * \file ecdsa.h
 *
 * \brief The Elliptic Curve Digital Signature Algorithm (ECDSA).
 *
 * ECDSA is defined in <em>Standards for Efficient Cryptography Group (SECG):
 * SEC1 Elliptic Curve Cryptography</em>.
 * The use of ECDSA for TLS is defined in <em>RFC-4492: Elliptic Curve
 * Cryptography (ECC) Cipher Suites for Transport Layer Security (TLS)</em>.
 *
 */
/*
 *  Copyright (C) 2006-2018, Arm Limited (or its affiliates), All Rights Reserved
 *  SPDX-License-Identifier: GPL-2.0
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  This file is part of Mbed TLS (https://tls.mbed.org)
 */

#ifndef TTLS_ECDSA_H
#define TTLS_ECDSA_H

#include "ecp.h"
#include "md.h"

/*
 * RFC-4492 page 20:
 *
 *	 Ecdsa-Sig-Value ::= SEQUENCE {
 *		 r	   INTEGER,
 *		 s	   INTEGER
 *	 }
 *
 * Size is at most
 *	1 (tag) + 1 (len) + 1 (initial 0) + ECP_MAX_BYTES for each of r and s,
 *	twice that + 1 (tag) + 2 (len) for the sequence
 * (assuming ECP_MAX_BYTES is less than 126 for r and s,
 * and less than 124 (total len <= 255) for the sequence)
 */
#if TTLS_ECP_MAX_BYTES > 124
#error "TTLS_ECP_MAX_BYTES bigger than expected, please fix TTLS_ECDSA_MAX_LEN"
#endif
/** The maximal size of an ECDSA signature in Bytes. */
#define TTLS_ECDSA_MAX_LEN  (3 + 2 * (3 + TTLS_ECP_MAX_BYTES))

/**
 * \brief		   The ECDSA context structure.
 */
typedef ttls_ecp_keypair ttls_ecdsa_context;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief		   This function computes the ECDSA signature of a
 *				  previously-hashed message.
 *
 * \note			The deterministic version is usually preferred.
 *
 * \param grp	   The ECP group.
 * \param r		 The first output integer.
 * \param s		 The second output integer.
 * \param d		 The private signing key.
 * \param buf	   The message hash.
 * \param blen	  The length of \p buf.
 * \param f_rng	 The RNG function.
 * \param p_rng	 The RNG parameter.
 *
 * \note			If the bitlength of the message hash is larger than the
 *				  bitlength of the group order, then the hash is truncated
 *				  as defined in <em>Standards for Efficient Cryptography Group
 *				  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *				  4.1.3, step 5.
 *
 * \return		  \c 0 on success, or an \c TTLS_ERR_ECP_XXX
 *				  or \c TTLS_MPI_XXX error code on failure.
 *
 * \see			 ecp.h
 */
int ttls_ecdsa_sign(ttls_ecp_group *grp, ttls_mpi *r, ttls_mpi *s,
				const ttls_mpi *d, const unsigned char *buf, size_t blen,
				int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);

#if defined(TTLS_ECDSA_DETERMINISTIC)
/**
 * \brief		   This function computes the ECDSA signature of a
 *				  previously-hashed message, deterministic version.
 *				  For more information, see <em>RFC-6979: Deterministic
 *				  Usage of the Digital Signature Algorithm (DSA) and Elliptic
 *				  Curve Digital Signature Algorithm (ECDSA)</em>.
 *
 * \param grp	   The ECP group.
 * \param r		 The first output integer.
 * \param s		 The second output integer.
 * \param d		 The private signing key.
 * \param buf	   The message hash.
 * \param blen	  The length of \p buf.
 * \param md_alg	The MD algorithm used to hash the message.
 *
 * \note			If the bitlength of the message hash is larger than the
 *				  bitlength of the group order, then the hash is truncated as
 *				  defined in <em>Standards for Efficient Cryptography Group
 *				  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *				  4.1.3, step 5.
 *
 * \return		  \c 0 on success,
 *				  or an \c TTLS_ERR_ECP_XXX or \c TTLS_MPI_XXX
 *				  error code on failure.
 *
 * \see			 ecp.h
 */
int ttls_ecdsa_sign_det(ttls_ecp_group *grp, ttls_mpi *r, ttls_mpi *s,
					const ttls_mpi *d, const unsigned char *buf, size_t blen,
					ttls_md_type_t md_alg);
#endif /* TTLS_ECDSA_DETERMINISTIC */

/**
 * \brief		   This function verifies the ECDSA signature of a
 *				  previously-hashed message.
 *
 * \param grp	   The ECP group.
 * \param buf	   The message hash.
 * \param blen	  The length of \p buf.
 * \param Q		 The public key to use for verification.
 * \param r		 The first integer of the signature.
 * \param s		 The second integer of the signature.
 *
 * \note			If the bitlength of the message hash is larger than the
 *				  bitlength of the group order, then the hash is truncated as
 *				  defined in <em>Standards for Efficient Cryptography Group
 *				  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *				  4.1.4, step 3.
 *
 * \return		  \c 0 on success,
 *				  #TTLS_ERR_ECP_BAD_INPUT_DATA if signature is invalid,
 *				  or an \c TTLS_ERR_ECP_XXX or \c TTLS_MPI_XXX
 *				  error code on failure for any other reason.
 *
 * \see			 ecp.h
 */
int ttls_ecdsa_verify(ttls_ecp_group *grp,
				  const unsigned char *buf, size_t blen,
				  const ttls_ecp_point *Q, const ttls_mpi *r, const ttls_mpi *s);

/**
 * \brief		   This function computes the ECDSA signature and writes it
 *				  to a buffer, serialized as defined in <em>RFC-4492:
 *				  Elliptic Curve Cryptography (ECC) Cipher Suites for
 *				  Transport Layer Security (TLS)</em>.
 *
 * \warning		 It is not thread-safe to use the same context in
 *				  multiple threads.
 *
 * \note			The deterministic version is used if
 *				  #TTLS_ECDSA_DETERMINISTIC is defined. For more
 *				  information, see <em>RFC-6979: Deterministic Usage
 *				  of the Digital Signature Algorithm (DSA) and Elliptic
 *				  Curve Digital Signature Algorithm (ECDSA)</em>.
 *
 * \param ctx	   The ECDSA context.
 * \param md_alg	The message digest that was used to hash the message.
 * \param hash	  The message hash.
 * \param hlen	  The length of the hash.
 * \param sig	   The buffer that holds the signature.
 * \param slen	  The length of the signature written.
 * \param f_rng	 The RNG function.
 * \param p_rng	 The RNG parameter.
 *
 * \note			The \p sig buffer must be at least twice as large as the
 *				  size of the curve used, plus 9. For example, 73 Bytes if
 *				  a 256-bit curve is used. A buffer length of
 *				  #TTLS_ECDSA_MAX_LEN is always safe.
 *
 * \note			If the bitlength of the message hash is larger than the
 *				  bitlength of the group order, then the hash is truncated as
 *				  defined in <em>Standards for Efficient Cryptography Group
 *				  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *				  4.1.3, step 5.
 *
 * \return		  \c 0 on success,
 *				  or an \c TTLS_ERR_ECP_XXX, \c TTLS_ERR_MPI_XXX or
 *				  \c TTLS_ERR_ASN1_XXX error code on failure.
 *
 * \see			 ecp.h
 */
int ttls_ecdsa_write_signature(ttls_ecdsa_context *ctx, ttls_md_type_t md_alg,
						   const unsigned char *hash, size_t hlen,
						   unsigned char *sig, size_t *slen,
						   int (*f_rng)(void *, unsigned char *, size_t),
						   void *p_rng);

#if defined(TTLS_ECDSA_DETERMINISTIC)
#if ! defined(TTLS_DEPRECATED_REMOVED)
#if defined(TTLS_DEPRECATED_WARNING)
#define TTLS_DEPRECATED	__attribute__((deprecated))
#else
#define TTLS_DEPRECATED
#endif
/**
 * \brief   This function computes an ECDSA signature and writes it to a buffer,
 *		  serialized as defined in <em>RFC-4492: Elliptic Curve Cryptography
 *		  (ECC) Cipher Suites for Transport Layer Security (TLS)</em>.
 *
 *		  The deterministic version is defined in <em>RFC-6979:
 *		  Deterministic Usage of the Digital Signature Algorithm (DSA) and
 *		  Elliptic Curve Digital Signature Algorithm (ECDSA)</em>.
 *
 * \warning		 It is not thread-safe to use the same context in
 *				  multiple threads.

 *
 * \deprecated	  Superseded by ttls_ecdsa_write_signature() in 2.0.0
 *
 * \param ctx	   The ECDSA context.
 * \param hash	  The Message hash.
 * \param hlen	  The length of the hash.
 * \param sig	   The buffer that holds the signature.
 * \param slen	  The length of the signature written.
 * \param md_alg	The MD algorithm used to hash the message.
 *
 * \note			The \p sig buffer must be at least twice as large as the
 *				  size of the curve used, plus 9. For example, 73 Bytes if a
 *				  256-bit curve is used. A buffer length of
 *				  #TTLS_ECDSA_MAX_LEN is always safe.
 *
 * \note			If the bitlength of the message hash is larger than the
 *				  bitlength of the group order, then the hash is truncated as
 *				  defined in <em>Standards for Efficient Cryptography Group
 *				  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *				  4.1.3, step 5.
 *
 * \return		  \c 0 on success,
 *				  or an \c TTLS_ERR_ECP_XXX, \c TTLS_ERR_MPI_XXX or
 *				  \c TTLS_ERR_ASN1_XXX error code on failure.
 *
 * \see			 ecp.h
 */
int ttls_ecdsa_write_signature_det(ttls_ecdsa_context *ctx,
							   const unsigned char *hash, size_t hlen,
							   unsigned char *sig, size_t *slen,
							   ttls_md_type_t md_alg) TTLS_DEPRECATED;
#undef TTLS_DEPRECATED
#endif /* TTLS_DEPRECATED_REMOVED */
#endif /* TTLS_ECDSA_DETERMINISTIC */

/**
 * \brief		   This function reads and verifies an ECDSA signature.
 *
 * \param ctx	   The ECDSA context.
 * \param hash	  The message hash.
 * \param hlen	  The size of the hash.
 * \param sig	   The signature to read and verify.
 * \param slen	  The size of \p sig.
 *
 * \note			If the bitlength of the message hash is larger than the
 *				  bitlength of the group order, then the hash is truncated as
 *				  defined in <em>Standards for Efficient Cryptography Group
 *				  (SECG): SEC1 Elliptic Curve Cryptography</em>, section
 *				  4.1.4, step 3.
 *
 * \return		  \c 0 on success,
 *				  #TTLS_ERR_ECP_BAD_INPUT_DATA if signature is invalid,
 *				  #TTLS_ERR_ECP_SIG_LEN_MISMATCH if the signature is
 *				  valid but its actual length is less than \p siglen,
 *				  or an \c TTLS_ERR_ECP_XXX or \c TTLS_ERR_MPI_XXX
 *				  error code on failure for any other reason.
 *
 * \see			 ecp.h
 */
int ttls_ecdsa_read_signature(ttls_ecdsa_context *ctx,
						  const unsigned char *hash, size_t hlen,
						  const unsigned char *sig, size_t slen);

/**
 * \brief		  This function generates an ECDSA keypair on the given curve.
 *
 * \param ctx	  The ECDSA context to store the keypair in.
 * \param gid	  The elliptic curve to use. One of the various
 *				 \c TTLS_ECP_DP_XXX macros depending on configuration.
 * \param f_rng	The RNG function.
 * \param p_rng	The RNG parameter.
 *
 * \return		 \c 0 on success, or an \c TTLS_ERR_ECP_XXX code on
 *				 failure.
 *
 * \see			ecp.h
 */
int ttls_ecdsa_genkey(ttls_ecdsa_context *ctx, ttls_ecp_group_id gid,
				  int (*f_rng)(void *, unsigned char *, size_t), void *p_rng);

/**
 * \brief		   This function sets an ECDSA context from an EC key pair.
 *
 * \param ctx	   The ECDSA context to set.
 * \param key	   The EC key to use.
 *
 * \return		  \c 0 on success, or an \c TTLS_ERR_ECP_XXX code on
 *				  failure.
 *
 * \see			 ecp.h
 */
int ttls_ecdsa_from_keypair(ttls_ecdsa_context *ctx, const ttls_ecp_keypair *key);

/**
 * \brief		   This function initializes an ECDSA context.
 *
 * \param ctx	   The ECDSA context to initialize.
 */
void ttls_ecdsa_init(ttls_ecdsa_context *ctx);

/**
 * \brief		   This function frees an ECDSA context.
 *
 * \param ctx	   The ECDSA context to free.
 */
void ttls_ecdsa_free(ttls_ecdsa_context *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ecdsa.h */
