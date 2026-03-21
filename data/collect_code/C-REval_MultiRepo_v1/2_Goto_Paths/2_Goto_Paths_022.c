/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_022 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 22 */
/* NLOC: 80 */
/* Subsystem: security */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/moduleparam.h>
 * #include <linux/ratelimit.h>
 * #include <linux/file.h>
 * #include <linux/crypto.h>
 * #include <linux/scatterlist.h>
 * #include <linux/err.h>
 * #include <linux/slab.h>
 * #include <crypto/hash.h>
 * #include "ima.h"
 */
/* Context-Before
 * 	if (algo < 0 || algo >= HASH_ALGO__LAST)
 * 		algo = ima_hash_algo;
 * 
 * 	if (algo == ima_hash_algo)
 * 		return tfm;
 * 
 * 	for (i = 0; i < NR_BANKS(ima_tpm_chip) + ima_extra_slots; i++)
 * 		if (ima_algo_array[i].tfm && ima_algo_array[i].algo == algo)
 * 			return ima_algo_array[i].tfm;
 * 
 * 	tfm = crypto_alloc_shash(hash_algo_name[algo], 0, 0);
 * 	if (IS_ERR(tfm)) {
 * 		rc = PTR_ERR(tfm);
 * 		pr_err("Can not allocate %s (reason: %d)\n",
 * 		       hash_algo_name[algo], rc);
 * 	}
 * 	return tfm;
 * }
 */
int __init ima_init_crypto(void)
{
	enum hash_algo algo;
	long rc;
	int i;

	rc = ima_init_ima_crypto();
	if (rc)
		return rc;

	ima_sha1_idx = -1;
	ima_hash_algo_idx = -1;

	for (i = 0; i < NR_BANKS(ima_tpm_chip); i++) {
		algo = ima_tpm_chip->allocated_banks[i].crypto_id;
		if (algo == HASH_ALGO_SHA1)
			ima_sha1_idx = i;

		if (algo == ima_hash_algo)
			ima_hash_algo_idx = i;
	}

	if (ima_sha1_idx < 0) {
		ima_sha1_idx = NR_BANKS(ima_tpm_chip) + ima_extra_slots++;
		if (ima_hash_algo == HASH_ALGO_SHA1)
			ima_hash_algo_idx = ima_sha1_idx;
	}

	if (ima_hash_algo_idx < 0)
		ima_hash_algo_idx = NR_BANKS(ima_tpm_chip) + ima_extra_slots++;

	ima_algo_array = kzalloc_objs(*ima_algo_array,
				      NR_BANKS(ima_tpm_chip) + ima_extra_slots);
	if (!ima_algo_array) {
		rc = -ENOMEM;
		goto out;
	}

	for (i = 0; i < NR_BANKS(ima_tpm_chip); i++) {
		algo = ima_tpm_chip->allocated_banks[i].crypto_id;
		ima_algo_array[i].algo = algo;

		/* unknown TPM algorithm */
		if (algo == HASH_ALGO__LAST)
			continue;

		if (algo == ima_hash_algo) {
			ima_algo_array[i].tfm = ima_shash_tfm;
			continue;
		}

		ima_algo_array[i].tfm = ima_alloc_tfm(algo);
		if (IS_ERR(ima_algo_array[i].tfm)) {
			if (algo == HASH_ALGO_SHA1) {
				rc = PTR_ERR(ima_algo_array[i].tfm);
				ima_algo_array[i].tfm = NULL;
				goto out_array;
			}

			ima_algo_array[i].tfm = NULL;
		}
	}

	if (ima_sha1_idx >= NR_BANKS(ima_tpm_chip)) {
		if (ima_hash_algo == HASH_ALGO_SHA1) {
			ima_algo_array[ima_sha1_idx].tfm = ima_shash_tfm;
		} else {
			ima_algo_array[ima_sha1_idx].tfm =
						ima_alloc_tfm(HASH_ALGO_SHA1);
			if (IS_ERR(ima_algo_array[ima_sha1_idx].tfm)) {
				rc = PTR_ERR(ima_algo_array[ima_sha1_idx].tfm);
				goto out_array;
			}
		}

		ima_algo_array[ima_sha1_idx].algo = HASH_ALGO_SHA1;
	}

	if (ima_hash_algo_idx >= NR_BANKS(ima_tpm_chip) &&
	    ima_hash_algo_idx != ima_sha1_idx) {
		ima_algo_array[ima_hash_algo_idx].tfm = ima_shash_tfm;
		ima_algo_array[ima_hash_algo_idx].algo = ima_hash_algo;
	}

	return 0;
out_array:
	for (i = 0; i < NR_BANKS(ima_tpm_chip) + ima_extra_slots; i++) {
		if (!ima_algo_array[i].tfm ||
		    ima_algo_array[i].tfm == ima_shash_tfm)
			continue;

		crypto_free_shash(ima_algo_array[i].tfm);
	}
	kfree(ima_algo_array);
out:
	crypto_free_shash(ima_shash_tfm);
	return rc;
}
/* Context-After
 * static void ima_free_tfm(struct crypto_shash *tfm)
 * {
 * 	int i;
 * 
 * 	if (tfm == ima_shash_tfm)
 * 		return;
 * 
 * 	for (i = 0; i < NR_BANKS(ima_tpm_chip) + ima_extra_slots; i++)
 * 		if (ima_algo_array[i].tfm == tfm)
 * 			return;
 * 
 * 	crypto_free_shash(tfm);
 * }
 * 
 * /**
 *  * ima_alloc_pages() - Allocate contiguous pages.
 *  * @max_size:       Maximum amount of memory to allocate.
 *  * @allocated_size: Returned size of actual allocation.
 *  * @last_warn:      Should the min_size allocation warn or not.
 */
