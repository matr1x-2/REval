/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_012 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 22 */
/* Subsystem: crypto */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/export.h>
 * #include <linux/slab.h>
 * #include <linux/err.h>
 * #include <linux/asn1.h>
 * #include <crypto/hash.h>
 * #include <crypto/hash_info.h>
 * #include <crypto/public_key.h>
 * #include "pkcs7_parser.h"
 */
/* Context-Before
 * 			sig->m_size = sinfo->authattrs_len;
 * 			ret = 0;
 * 		} else {
 * 			ret = crypto_shash_digest(desc, sig->m,
 * 						  sinfo->authattrs_len,
 * 						  sig->m);
 * 			if (ret < 0)
 * 				goto error;
 * 		}
 * 		pr_devel("AADigest = [%*ph]\n", 8, sig->m);
 * 	}
 * 
 * error:
 * 	kfree(desc);
 * error_no_desc:
 * 	crypto_free_shash(tfm);
 * 	kleave(" = %d", ret);
 * 	return ret;
 * }
 */
int pkcs7_get_digest(struct pkcs7_message *pkcs7, const u8 **buf, u32 *len,
		     enum hash_algo *hash_algo)
{
	struct pkcs7_signed_info *sinfo = pkcs7->signed_infos;
	int i, ret;

	/*
	 * This function doesn't support messages with more than one signature.
	 */
	if (sinfo == NULL || sinfo->next != NULL)
		return -EBADMSG;

	ret = pkcs7_digest(pkcs7, sinfo);
	if (ret)
		return ret;
	if (!sinfo->sig->m_free) {
		pr_notice_once("%s: No digest available\n", __func__);
		return -EINVAL; /* TODO: MLDSA doesn't necessarily calculate an
				 * intermediate digest. */
	}

	*buf = sinfo->sig->m;
	*len = sinfo->sig->m_size;

	i = match_string(hash_algo_name, HASH_ALGO__LAST,
			 sinfo->sig->hash_algo);
	if (i >= 0)
		*hash_algo = i;

	return 0;
}
/* Context-After
 * /*
 *  * Find the key (X.509 certificate) to use to verify a PKCS#7 message.  PKCS#7
 *  * uses the issuer's name and the issuing certificate serial number for
 *  * matching purposes.  These must match the certificate issuer's name (not
 *  * subject's name) and the certificate serial number [RFC 2315 6.7].
 *  */
 * static int pkcs7_find_key(struct pkcs7_message *pkcs7,
 * 			  struct pkcs7_signed_info *sinfo)
 * {
 * 	struct x509_certificate *x509;
 * 	unsigned certix = 1;
 * 
 * 	kenter("%u", sinfo->index);
 * 
 * 	for (x509 = pkcs7->certs; x509; x509 = x509->next, certix++) {
 * 		/* I'm _assuming_ that the generator of the PKCS#7 message will
 * 		 * encode the fields from the X.509 cert in the same way in the
 * 		 * PKCS#7 message - but I can't be 100% sure of that.  It's
 * 		 * possible this will need element-by-element comparison.
 */
