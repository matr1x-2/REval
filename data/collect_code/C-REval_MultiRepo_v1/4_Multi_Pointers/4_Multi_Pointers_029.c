/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_029 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 36 */
/* Subsystem: security */
/* Includes
 * #include <linux/types.h>
 * #include <linux/module_signature.h>
 * #include <keys/asymmetric-type.h>
 * #include <crypto/pkcs7.h>
 * #include "ima.h"
 */
/* Context-Before
 * 	enum hash_algo hash_algo;
 * 
 * 	/* This digest will go in the 'd-modsig' field of the IMA template. */
 * 	const u8 *digest;
 * 	u32 digest_size;
 * 
 * 	/*
 * 	 * This is what will go to the measurement list if the template requires
 * 	 * storing the signature.
 * 	 */
 * 	int raw_pkcs7_len;
 * 	u8 raw_pkcs7[] __counted_by(raw_pkcs7_len);
 * };
 * 
 * /*
 *  * ima_read_modsig - Read modsig from buf.
 *  *
 *  * Return: 0 on success, error code otherwise.
 *  */
 */
int ima_read_modsig(enum ima_hooks func, const void *buf, loff_t buf_len,
		    struct modsig **modsig)
{
	const size_t marker_len = strlen(MODULE_SIG_STRING);
	const struct module_signature *sig;
	struct modsig *hdr;
	size_t sig_len;
	const void *p;
	int rc;

	if (buf_len <= marker_len + sizeof(*sig))
		return -ENOENT;

	p = buf + buf_len - marker_len;
	if (memcmp(p, MODULE_SIG_STRING, marker_len))
		return -ENOENT;

	buf_len -= marker_len;
	sig = (const struct module_signature *)(p - sizeof(*sig));

	rc = mod_check_sig(sig, buf_len, func_tokens[func]);
	if (rc)
		return rc;

	sig_len = be32_to_cpu(sig->sig_len);
	buf_len -= sig_len + sizeof(*sig);

	/* Allocate sig_len additional bytes to hold the raw PKCS#7 data. */
	hdr = kzalloc_flex(*hdr, raw_pkcs7, sig_len);
	if (!hdr)
		return -ENOMEM;

	hdr->raw_pkcs7_len = sig_len;
	hdr->pkcs7_msg = pkcs7_parse_message(buf + buf_len, sig_len);
	if (IS_ERR(hdr->pkcs7_msg)) {
		rc = PTR_ERR(hdr->pkcs7_msg);
		kfree(hdr);
		return rc;
	}

	memcpy(hdr->raw_pkcs7, buf + buf_len, sig_len);

	/* We don't know the hash algorithm yet. */
	hdr->hash_algo = HASH_ALGO__LAST;

	*modsig = hdr;

	return 0;
}
/* Context-After
 * /**
 *  * ima_collect_modsig - Calculate the file hash without the appended signature.
 *  * @modsig: parsed module signature
 *  * @buf: data to verify the signature on
 *  * @size: data size
 *  *
 *  * Since the modsig is part of the file contents, the hash used in its signature
 *  * isn't the same one ordinarily calculated by IMA. Therefore PKCS7 code
 *  * calculates a separate one for signature verification.
 *  */
 * void ima_collect_modsig(struct modsig *modsig, const void *buf, loff_t size)
 * {
 * 	int rc;
 * 
 * 	/*
 * 	 * Provide the file contents (minus the appended sig) so that the PKCS7
 * 	 * code can calculate the file hash.
 * 	 */
 * 	size -= modsig->raw_pkcs7_len + strlen(MODULE_SIG_STRING) +
 */
