/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_024 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 60 */
/* Subsystem: crypto */
/* Includes
 * #include <crypto/internal/aead.h>
 * #include <crypto/internal/hash.h>
 * #include <crypto/internal/skcipher.h>
 * #include <crypto/authenc.h>
 * #include <crypto/scatterwalk.h>
 * #include <linux/err.h>
 * #include <linux/init.h>
 * #include <linux/kernel.h>
 * #include <linux/module.h>
 * #include <linux/rtnetlink.h>
 * #include <linux/slab.h>
 * #include <linux/spinlock.h>
 */
/* Context-Before
 * 	return err;
 * }
 * 
 * static void crypto_authenc_exit_tfm(struct crypto_aead *tfm)
 * {
 * 	struct crypto_authenc_ctx *ctx = crypto_aead_ctx(tfm);
 * 
 * 	crypto_free_ahash(ctx->auth);
 * 	crypto_free_skcipher(ctx->enc);
 * }
 * 
 * static void crypto_authenc_free(struct aead_instance *inst)
 * {
 * 	struct authenc_instance_ctx *ctx = aead_instance_ctx(inst);
 * 
 * 	crypto_drop_skcipher(&ctx->enc);
 * 	crypto_drop_ahash(&ctx->auth);
 * 	kfree(inst);
 * }
 */
static int crypto_authenc_create(struct crypto_template *tmpl,
				 struct rtattr **tb)
{
	u32 mask;
	struct aead_instance *inst;
	struct authenc_instance_ctx *ctx;
	struct skcipher_alg_common *enc;
	struct hash_alg_common *auth;
	struct crypto_alg *auth_base;
	int err;

	err = crypto_check_attr_type(tb, CRYPTO_ALG_TYPE_AEAD, &mask);
	if (err)
		return err;

	inst = kzalloc(sizeof(*inst) + sizeof(*ctx), GFP_KERNEL);
	if (!inst)
		return -ENOMEM;
	ctx = aead_instance_ctx(inst);

	err = crypto_grab_ahash(&ctx->auth, aead_crypto_instance(inst),
				crypto_attr_alg_name(tb[1]), 0, mask);
	if (err)
		goto err_free_inst;
	auth = crypto_spawn_ahash_alg(&ctx->auth);
	auth_base = &auth->base;

	err = crypto_grab_skcipher(&ctx->enc, aead_crypto_instance(inst),
				   crypto_attr_alg_name(tb[2]), 0, mask);
	if (err)
		goto err_free_inst;
	enc = crypto_spawn_skcipher_alg_common(&ctx->enc);

	ctx->reqoff = 2 * auth->digestsize;

	err = -ENAMETOOLONG;
	if (snprintf(inst->alg.base.cra_name, CRYPTO_MAX_ALG_NAME,
		     "authenc(%s,%s)", auth_base->cra_name,
		     enc->base.cra_name) >=
	    CRYPTO_MAX_ALG_NAME)
		goto err_free_inst;

	if (snprintf(inst->alg.base.cra_driver_name, CRYPTO_MAX_ALG_NAME,
		     "authenc(%s,%s)", auth_base->cra_driver_name,
		     enc->base.cra_driver_name) >= CRYPTO_MAX_ALG_NAME)
		goto err_free_inst;

	inst->alg.base.cra_priority = enc->base.cra_priority * 10 +
				      auth_base->cra_priority;
	inst->alg.base.cra_blocksize = enc->base.cra_blocksize;
	inst->alg.base.cra_alignmask = enc->base.cra_alignmask;
	inst->alg.base.cra_ctxsize = sizeof(struct crypto_authenc_ctx);

	inst->alg.ivsize = enc->ivsize;
	inst->alg.chunksize = enc->chunksize;
	inst->alg.maxauthsize = auth->digestsize;

	inst->alg.init = crypto_authenc_init_tfm;
	inst->alg.exit = crypto_authenc_exit_tfm;

	inst->alg.setkey = crypto_authenc_setkey;
	inst->alg.encrypt = crypto_authenc_encrypt;
	inst->alg.decrypt = crypto_authenc_decrypt;

	inst->free = crypto_authenc_free;

	err = aead_register_instance(tmpl, inst);
	if (err) {
err_free_inst:
		crypto_authenc_free(inst);
	}
	return err;
}
/* Context-After
 * static struct crypto_template crypto_authenc_tmpl = {
 * 	.name = "authenc",
 * 	.create = crypto_authenc_create,
 * 	.module = THIS_MODULE,
 * };
 * 
 * static int __init crypto_authenc_module_init(void)
 * {
 * 	return crypto_register_template(&crypto_authenc_tmpl);
 * }
 * 
 * static void __exit crypto_authenc_module_exit(void)
 * {
 * 	crypto_unregister_template(&crypto_authenc_tmpl);
 * }
 * 
 * module_init(crypto_authenc_module_init);
 * module_exit(crypto_authenc_module_exit);
 */
