/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_019 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 9 */
/* NLOC: 39 */
/* Subsystem: arch */
/* Includes
 * #include <asm/cpacf.h>
 * #include <asm/pkey.h>
 * #include <crypto/engine.h>
 * #include <crypto/hash.h>
 * #include <crypto/internal/hash.h>
 * #include <crypto/sha2.h>
 * #include <linux/atomic.h>
 * #include <linux/cpufeature.h>
 * #include <linux/delay.h>
 * #include <linux/miscdevice.h>
 * #include <linux/module.h>
 * #include <linux/spinlock.h>
 */
/* Context-Before
 * static void s390_phmac_exit(void)
 * {
 * 	struct phmac_alg *phmac;
 * 	int i;
 * 
 * 	if (phmac_crypto_engine) {
 * 		crypto_engine_stop(phmac_crypto_engine);
 * 		crypto_engine_exit(phmac_crypto_engine);
 * 	}
 * 
 * 	for (i = ARRAY_SIZE(phmac_algs) - 1; i >= 0; i--) {
 * 		phmac = &phmac_algs[i];
 * 		if (phmac->registered)
 * 			crypto_engine_unregister_ahash(&phmac->alg);
 * 	}
 * 
 * 	misc_deregister(&phmac_dev);
 * }
 */
static int __init s390_phmac_init(void)
{
	struct phmac_alg *phmac;
	int i, rc;

	/* for selftest cpacf klmd subfunction is needed */
	if (!cpacf_query_func(CPACF_KLMD, CPACF_KLMD_SHA_256))
		return -ENODEV;
	if (!cpacf_query_func(CPACF_KLMD, CPACF_KLMD_SHA_512))
		return -ENODEV;

	/* register a simple phmac pseudo misc device */
	rc = misc_register(&phmac_dev);
	if (rc)
		return rc;

	/* with this pseudo device alloc and start a crypto engine */
	phmac_crypto_engine =
		crypto_engine_alloc_init_and_set(phmac_dev.this_device,
						 true, false, MAX_QLEN);
	if (!phmac_crypto_engine) {
		rc = -ENOMEM;
		goto out_err;
	}
	rc = crypto_engine_start(phmac_crypto_engine);
	if (rc) {
		crypto_engine_exit(phmac_crypto_engine);
		phmac_crypto_engine = NULL;
		goto out_err;
	}

	for (i = 0; i < ARRAY_SIZE(phmac_algs); i++) {
		phmac = &phmac_algs[i];
		if (!cpacf_query_func(CPACF_KMAC, phmac->fc))
			continue;
		rc = crypto_engine_register_ahash(&phmac->alg);
		if (rc)
			goto out_err;
		phmac->registered = true;
		pr_debug("%s registered\n", phmac->alg.base.halg.base.cra_name);
	}

	return 0;

out_err:
	s390_phmac_exit();
	return rc;
}
/* Context-After
 * module_init(s390_phmac_init);
 * module_exit(s390_phmac_exit);
 * 
 * MODULE_ALIAS_CRYPTO("phmac(sha224)");
 * MODULE_ALIAS_CRYPTO("phmac(sha256)");
 * MODULE_ALIAS_CRYPTO("phmac(sha384)");
 * MODULE_ALIAS_CRYPTO("phmac(sha512)");
 * 
 * MODULE_DESCRIPTION("S390 HMAC driver for protected keys");
 * MODULE_LICENSE("GPL");
 */
