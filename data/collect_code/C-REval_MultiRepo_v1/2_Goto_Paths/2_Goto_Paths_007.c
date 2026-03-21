/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_007 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 9 */
/* NLOC: 38 */
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
 * 	if (ret < 0)
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
 * 	memzero_explicit(lt_key, sizeof(lt_key));
 * 	return ret;
 * }
 */
static int blk_crypto_ioctl_prepare_key(struct blk_crypto_profile *profile,
					void __user *argp)
{
	struct blk_crypto_prepare_key_arg arg;
	u8 lt_key[BLK_CRYPTO_MAX_HW_WRAPPED_KEY_SIZE];
	u8 eph_key[BLK_CRYPTO_MAX_HW_WRAPPED_KEY_SIZE];
	int ret;

	if (copy_from_user(&arg, argp, sizeof(arg)))
		return -EFAULT;

	if (memchr_inv(arg.reserved, 0, sizeof(arg.reserved)))
		return -EINVAL;

	if (arg.lt_key_size > sizeof(lt_key))
		return -EINVAL;

	if (copy_from_user(lt_key, u64_to_user_ptr(arg.lt_key_ptr),
			   arg.lt_key_size)) {
		ret = -EFAULT;
		goto out;
	}
	ret = blk_crypto_prepare_key(profile, lt_key, arg.lt_key_size, eph_key);
	if (ret < 0)
		goto out;
	if (ret > arg.eph_key_size) {
		ret = -EOVERFLOW;
		goto out;
	}
	arg.eph_key_size = ret;
	if (copy_to_user(u64_to_user_ptr(arg.eph_key_ptr), eph_key,
			 arg.eph_key_size) ||
	    copy_to_user(argp, &arg, sizeof(arg))) {
		ret = -EFAULT;
		goto out;
	}
	ret = 0;

out:
	memzero_explicit(lt_key, sizeof(lt_key));
	memzero_explicit(eph_key, sizeof(eph_key));
	return ret;
}
/* Context-After
 * int blk_crypto_ioctl(struct block_device *bdev, unsigned int cmd,
 * 		     void __user *argp)
 * {
 * 	struct blk_crypto_profile *profile =
 * 		bdev_get_queue(bdev)->crypto_profile;
 * 
 * 	if (!profile)
 * 		return -EOPNOTSUPP;
 * 
 * 	switch (cmd) {
 * 	case BLKCRYPTOIMPORTKEY:
 * 		return blk_crypto_ioctl_import_key(profile, argp);
 * 	case BLKCRYPTOGENERATEKEY:
 * 		return blk_crypto_ioctl_generate_key(profile, argp);
 * 	case BLKCRYPTOPREPAREKEY:
 * 		return blk_crypto_ioctl_prepare_key(profile, argp);
 * 	default:
 * 		return -ENOTTY;
 * 	}
 */
