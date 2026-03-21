/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_009 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 17 */
/* NLOC: 73 */
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
 * 	return tfm;
 * }
 * 
 * static void ima_free_atfm(struct crypto_ahash *tfm)
 * {
 * 	if (tfm != ima_ahash_tfm)
 * 		crypto_free_ahash(tfm);
 * }
 * 
 * static inline int ahash_wait(int err, struct crypto_wait *wait)
 * {
 * 
 * 	err = crypto_wait_req(err, wait);
 * 
 * 	if (err)
 * 		pr_crit_ratelimited("ahash calculation failed: err: %d\n", err);
 * 
 * 	return err;
 * }
 */
static int ima_calc_file_hash_atfm(struct file *file,
				   struct ima_digest_data *hash,
				   struct crypto_ahash *tfm)
{
	loff_t i_size, offset;
	char *rbuf[2] = { NULL, };
	int rc, rbuf_len, active = 0, ahash_rc = 0;
	struct ahash_request *req;
	struct scatterlist sg[1];
	struct crypto_wait wait;
	size_t rbuf_size[2];

	hash->length = crypto_ahash_digestsize(tfm);

	req = ahash_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	crypto_init_wait(&wait);
	ahash_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG |
				   CRYPTO_TFM_REQ_MAY_SLEEP,
				   crypto_req_done, &wait);

	rc = ahash_wait(crypto_ahash_init(req), &wait);
	if (rc)
		goto out1;

	i_size = i_size_read(file_inode(file));

	if (i_size == 0)
		goto out2;

	/*
	 * Try to allocate maximum size of memory.
	 * Fail if even a single page cannot be allocated.
	 */
	rbuf[0] = ima_alloc_pages(i_size, &rbuf_size[0], 1);
	if (!rbuf[0]) {
		rc = -ENOMEM;
		goto out1;
	}

	/* Only allocate one buffer if that is enough. */
	if (i_size > rbuf_size[0]) {
		/*
		 * Try to allocate secondary buffer. If that fails fallback to
		 * using single buffering. Use previous memory allocation size
		 * as baseline for possible allocation size.
		 */
		rbuf[1] = ima_alloc_pages(i_size - rbuf_size[0],
					  &rbuf_size[1], 0);
	}

	for (offset = 0; offset < i_size; offset += rbuf_len) {
		if (!rbuf[1] && offset) {
			/* Not using two buffers, and it is not the first
			 * read/request, wait for the completion of the
			 * previous ahash_update() request.
			 */
			rc = ahash_wait(ahash_rc, &wait);
			if (rc)
				goto out3;
		}
		/* read buffer */
		rbuf_len = min_t(loff_t, i_size - offset, rbuf_size[active]);
		rc = integrity_kernel_read(file, offset, rbuf[active],
					   rbuf_len);
		if (rc != rbuf_len) {
			if (rc >= 0)
				rc = -EINVAL;
			/*
			 * Forward current rc, do not overwrite with return value
			 * from ahash_wait()
			 */
			ahash_wait(ahash_rc, &wait);
			goto out3;
		}

		if (rbuf[1] && offset) {
			/* Using two buffers, and it is not the first
			 * read/request, wait for the completion of the
			 * previous ahash_update() request.
			 */
			rc = ahash_wait(ahash_rc, &wait);
			if (rc)
				goto out3;
		}

		sg_init_one(&sg[0], rbuf[active], rbuf_len);
		ahash_request_set_crypt(req, sg, NULL, rbuf_len);

		ahash_rc = crypto_ahash_update(req);

		if (rbuf[1])
			active = !active; /* swap buffers, if we use two */
	}
	/* wait for the last update request to complete */
	rc = ahash_wait(ahash_rc, &wait);
out3:
	ima_free_pages(rbuf[0], rbuf_size[0]);
	ima_free_pages(rbuf[1], rbuf_size[1]);
out2:
	if (!rc) {
		ahash_request_set_crypt(req, NULL, hash->digest, 0);
		rc = ahash_wait(crypto_ahash_final(req), &wait);
	}
out1:
	ahash_request_free(req);
	return rc;
}
/* Context-After
 * static int ima_calc_file_ahash(struct file *file, struct ima_digest_data *hash)
 * {
 * 	struct crypto_ahash *tfm;
 * 	int rc;
 * 
 * 	tfm = ima_alloc_atfm(hash->algo);
 * 	if (IS_ERR(tfm))
 * 		return PTR_ERR(tfm);
 * 
 * 	rc = ima_calc_file_hash_atfm(file, hash, tfm);
 * 
 * 	ima_free_atfm(tfm);
 * 
 * 	return rc;
 * }
 * 
 * static int ima_calc_file_hash_tfm(struct file *file,
 * 				  struct ima_digest_data *hash,
 * 				  struct crypto_shash *tfm)
 */
