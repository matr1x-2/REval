/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_024 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 16 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/slab.h>
 * #include <linux/module.h>
 * #include <linux/list.h>
 * #include <linux/fs.h>
 * #include <linux/mtd/blktrans.h>
 * #include <linux/mtd/mtd.h>
 * #include <linux/blkdev.h>
 * #include <linux/blk-mq.h>
 * #include <linux/blkpg.h>
 * #include <linux/spinlock.h>
 * #include <linux/hdreg.h>
 * #include <linux/mutex.h>
 * #include <linux/uaccess.h>
 * #include "mtdcore.h"
 */
/* Context-Before
 * 	ret = __get_mtd_device(dev->mtd);
 * 	if (ret)
 * 		goto error_release;
 * 	dev->writable = mode & BLK_OPEN_WRITE;
 * 
 * unlock:
 * 	dev->open++;
 * 	mutex_unlock(&dev->lock);
 * 	return ret;
 * 
 * error_release:
 * 	if (dev->tr->release)
 * 		dev->tr->release(dev);
 * error_put:
 * 	module_put(dev->tr->owner);
 * 	mutex_unlock(&dev->lock);
 * 	blktrans_dev_put(dev);
 * 	return ret;
 * }
 */
static void blktrans_release(struct gendisk *disk)
{
	struct mtd_blktrans_dev *dev = disk->private_data;

	mutex_lock(&dev->lock);

	if (--dev->open)
		goto unlock;

	module_put(dev->tr->owner);

	if (dev->mtd) {
		if (dev->tr->release)
			dev->tr->release(dev);
		__put_mtd_device(dev->mtd);
	}
unlock:
	mutex_unlock(&dev->lock);
	blktrans_dev_put(dev);
}
/* Context-After
 * static int blktrans_getgeo(struct gendisk *disk, struct hd_geometry *geo)
 * {
 * 	struct mtd_blktrans_dev *dev = disk->private_data;
 * 	int ret = -ENXIO;
 * 
 * 	mutex_lock(&dev->lock);
 * 
 * 	if (!dev->mtd)
 * 		goto unlock;
 * 
 * 	ret = dev->tr->getgeo ? dev->tr->getgeo(dev, geo) : -ENOTTY;
 * unlock:
 * 	mutex_unlock(&dev->lock);
 * 	return ret;
 * }
 * 
 * static const struct block_device_operations mtd_block_ops = {
 * 	.owner		= THIS_MODULE,
 * 	.open		= blktrans_open,
 */
