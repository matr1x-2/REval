/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_020 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 29 */
/* Subsystem: block */
/* Includes
 * #include <linux/bio.h>
 * #include <linux/blkdev.h>
 * #include <linux/blk-crypto-profile.h>
 * #include <linux/module.h>
 * #include <linux/ratelimit.h>
 * #include <linux/slab.h>
 * #include "blk-crypto-internal.h"
 */
/* Context-Before
 * 		goto out;
 * 	if (ret > arg.lt_key_size) {
 * 		ret = -EOVERFLOW;
 * 		goto out;
 * 	}
 * 	arg.lt_key_size = ret;
 * 	if (copy_to_user(u64_to_user_ptr(arg.lt_key_ptr), lt_key,
 * 			 arg.lt_key_size) ||
 * 	    copy_to_user(argp, &arg, sizeof(arg))) {
 * 		ret = -EFAULT;
 * 		goto out;
 * 	}
 * 	ret = 0;
 * 
 * out:
 * 	memzero_explicit(raw_key, sizeof(raw_key));
 * 	memzero_explicit(lt_key, sizeof(lt_key));
 * 	return ret;
 * }
 */
static int blk_crypto_ioctl_generate_key(struct blk_crypto_profile *profile,
					 void __user *argp)
{
	struct blk_crypto_generate_key_arg arg;
	u8 lt_key[BLK_CRYPTO_MAX_HW_WRAPPED_KEY_SIZE];
	int ret;

	if (copy_from_user(&arg, argp, sizeof(arg)))
		return -EFAULT;

	if (memchr_inv(arg.reserved, 0, sizeof(arg.reserved)))
		return -EINVAL;

	ret = blk_crypto_generate_key(profile, lt_key);
	if (ret < 0)
		goto out;
	if (ret > arg.lt_key_size) {
		ret = -EOVERFLOW;
		goto out;
	}
	arg.lt_key_size = ret;
	if (copy_to_user(u64_to_user_ptr(arg.lt_key_ptr), lt_key,
			 arg.lt_key_size) ||
	    copy_to_user(argp, &arg, sizeof(arg))) {
		ret = -EFAULT;
		goto out;
	}
	ret = 0;

out:
	memzero_explicit(lt_key, sizeof(lt_key));
	return ret;
}
/* Context-After
 * static int blk_crypto_ioctl_prepare_key(struct blk_crypto_profile *profile,
 * 					void __user *argp)
 * {
 * 	struct blk_crypto_prepare_key_arg arg;
 * 	u8 lt_key[BLK_CRYPTO_MAX_HW_WRAPPED_KEY_SIZE];
 * 	u8 eph_key[BLK_CRYPTO_MAX_HW_WRAPPED_KEY_SIZE];
 * 	int ret;
 * 
 * 	if (copy_from_user(&arg, argp, sizeof(arg)))
 * 		return -EFAULT;
 * 
 * 	if (memchr_inv(arg.reserved, 0, sizeof(arg.reserved)))
 * 		return -EINVAL;
 * 
 * 	if (arg.lt_key_size > sizeof(lt_key))
 * 		return -EINVAL;
 * 
 * 	if (copy_from_user(lt_key, u64_to_user_ptr(arg.lt_key_ptr),
 * 			   arg.lt_key_size)) {
 */
