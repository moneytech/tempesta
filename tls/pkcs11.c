/**
 * \file pkcs11.c
 *
 * \brief Wrapper for PKCS#11 library libpkcs11-helper
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  Copyright (C) 2015-2018 Tempesta Technologies, Inc.
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
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
#include "pkcs11.h"

#if defined(TTLS_PKCS11_C)

#include "md.h"
#include "oid.h"
#include "x509_crt.h"

#include <string.h>

void ttls_pkcs11_init(ttls_pkcs11_context *ctx)
{
	memset(ctx, 0, sizeof(ttls_pkcs11_context));
}

int ttls_pkcs11_x509_cert_bind(ttls_x509_crt *cert, pkcs11h_certificate_t pkcs11_cert)
{
	int ret = 1;
	unsigned char *cert_blob = NULL;
	size_t cert_blob_size = 0;

	if (cert == NULL)
	{
		ret = 2;
		goto cleanup;
	}

	if (pkcs11h_certificate_getCertificateBlob(pkcs11_cert, NULL,
												&cert_blob_size) != CKR_OK)
	{
		ret = 3;
		goto cleanup;
	}

	cert_blob = ttls_calloc(1, cert_blob_size);
	if (NULL == cert_blob)
	{
		ret = 4;
		goto cleanup;
	}

	if (pkcs11h_certificate_getCertificateBlob(pkcs11_cert, cert_blob,
												&cert_blob_size) != CKR_OK)
	{
		ret = 5;
		goto cleanup;
	}

	if (0 != ttls_x509_crt_parse(cert, cert_blob, cert_blob_size))
	{
		ret = 6;
		goto cleanup;
	}

	ret = 0;

cleanup:
	if (NULL != cert_blob)
		ttls_free(cert_blob);

	return ret;
}


int ttls_pkcs11_priv_key_bind(ttls_pkcs11_context *priv_key,
		pkcs11h_certificate_t pkcs11_cert)
{
	int ret = 1;
	ttls_x509_crt cert;

	ttls_x509_crt_init(&cert);

	if (priv_key == NULL)
		goto cleanup;

	if (0 != ttls_pkcs11_x509_cert_bind(&cert, pkcs11_cert))
		goto cleanup;

	priv_key->len = ttls_pk_get_len(&cert.pk);
	priv_key->pkcs11h_cert = pkcs11_cert;

	ret = 0;

cleanup:
	ttls_x509_crt_free(&cert);

	return ret;
}

void ttls_pkcs11_priv_key_free(ttls_pkcs11_context *priv_key)
{
	if (NULL != priv_key)
		pkcs11h_certificate_freeCertificate(priv_key->pkcs11h_cert);
}

int ttls_pkcs11_decrypt(ttls_pkcs11_context *ctx,
					   int mode, size_t *olen,
					   const unsigned char *input,
					   unsigned char *output,
					   size_t output_max_len)
{
	size_t input_len, output_len;

	if (NULL == ctx)
		return(TTLS_ERR_RSA_BAD_INPUT_DATA);

	if (TTLS_RSA_PRIVATE != mode)
		return(TTLS_ERR_RSA_BAD_INPUT_DATA);

	output_len = input_len = ctx->len;

	if (input_len < 16 || input_len > output_max_len)
		return(TTLS_ERR_RSA_BAD_INPUT_DATA);

	/* Determine size of output buffer */
	if (pkcs11h_certificate_decryptAny(ctx->pkcs11h_cert, CKM_RSA_PKCS, input,
			input_len, NULL, &output_len) != CKR_OK)
	{
		return(TTLS_ERR_RSA_BAD_INPUT_DATA);
	}

	if (output_len > output_max_len)
		return(TTLS_ERR_RSA_OUTPUT_TOO_LARGE);

	if (pkcs11h_certificate_decryptAny(ctx->pkcs11h_cert, CKM_RSA_PKCS, input,
			input_len, output, &output_len) != CKR_OK)
	{
		return(TTLS_ERR_RSA_BAD_INPUT_DATA);
	}
	*olen = output_len;
	return 0;
}

int ttls_pkcs11_sign(ttls_pkcs11_context *ctx,
					int mode,
					ttls_md_type_t md_alg,
					unsigned int hashlen,
					const unsigned char *hash,
					unsigned char *sig)
{
	size_t sig_len = 0, asn_len = 0, oid_size = 0;
	unsigned char *p = sig;
	const char *oid;

	if (NULL == ctx)
		return(TTLS_ERR_RSA_BAD_INPUT_DATA);

	if (TTLS_RSA_PRIVATE != mode)
		return(TTLS_ERR_RSA_BAD_INPUT_DATA);

	if (md_alg != TTLS_MD_NONE)
	{
		const ttls_md_info_t *md_info = ttls_md_info_from_type(md_alg);
		if (md_info == NULL)
			return(TTLS_ERR_RSA_BAD_INPUT_DATA);

		if (ttls_oid_get_oid_by_md(md_alg, &oid, &oid_size) != 0)
			return(TTLS_ERR_RSA_BAD_INPUT_DATA);

		hashlen = ttls_md_get_size(md_info);
		asn_len = 10 + oid_size;
	}

	sig_len = ctx->len;
	if (hashlen > sig_len || asn_len > sig_len ||
		hashlen + asn_len > sig_len)
	{
		return(TTLS_ERR_RSA_BAD_INPUT_DATA);
	}

	if (md_alg != TTLS_MD_NONE)
	{
		/*
		 * DigestInfo ::= SEQUENCE {
		 *   digestAlgorithm DigestAlgorithmIdentifier,
		 *   digest Digest }
		 *
		 * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
		 *
		 * Digest ::= OCTET STRING
		 */
		*p++ = TTLS_ASN1_SEQUENCE | TTLS_ASN1_CONSTRUCTED;
		*p++ = (unsigned char) (0x08 + oid_size + hashlen);
		*p++ = TTLS_ASN1_SEQUENCE | TTLS_ASN1_CONSTRUCTED;
		*p++ = (unsigned char) (0x04 + oid_size);
		*p++ = TTLS_ASN1_OID;
		*p++ = oid_size & 0xFF;
		memcpy(p, oid, oid_size);
		p += oid_size;
		*p++ = TTLS_ASN1_NULL;
		*p++ = 0x00;
		*p++ = TTLS_ASN1_OCTET_STRING;
		*p++ = hashlen;
	}

	memcpy(p, hash, hashlen);

	if (pkcs11h_certificate_signAny(ctx->pkcs11h_cert, CKM_RSA_PKCS, sig,
			asn_len + hashlen, sig, &sig_len) != CKR_OK)
	{
		return(TTLS_ERR_RSA_BAD_INPUT_DATA);
	}

	return 0;
}

#endif /* defined(TTLS_PKCS11_C) */
