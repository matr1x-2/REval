/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_035 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 5 */
/* NLOC: 31 */
/* Subsystem: kernel */
/* Includes
 * #include <linux/device.h>
 * #include <linux/irq.h>
 * #include <linux/irqdomain.h>
 * #include <linux/msi.h>
 * #include <linux/mutex.h>
 * #include <linux/pci.h>
 * #include <linux/slab.h>
 * #include <linux/seq_file.h>
 * #include <linux/sysfs.h>
 * #include <linux/types.h>
 * #include <linux/xarray.h>
 * #include "internals.h"
 */
/* Context-Before
 * 		return NULL;
 * 
 * 	desc->dev = dev;
 * 	desc->nvec_used = nvec;
 * 	if (affinity) {
 * 		desc->affinity = kmemdup_array(affinity, nvec, sizeof(*desc->affinity), GFP_KERNEL);
 * 		if (!desc->affinity) {
 * 			kfree(desc);
 * 			return NULL;
 * 		}
 * 	}
 * 	return desc;
 * }
 * 
 * static void msi_free_desc(struct msi_desc *desc)
 * {
 * 	kfree(desc->affinity);
 * 	kfree(desc);
 * }
 */
static int msi_insert_desc(struct device *dev, struct msi_desc *desc,
			   unsigned int domid, unsigned int index)
{
	struct msi_device_data *md = dev->msi.data;
	struct xarray *xa = &md->__domains[domid].store;
	unsigned int hwsize;
	int ret;

	hwsize = msi_domain_get_hwsize(dev, domid);

	if (index == MSI_ANY_INDEX) {
		struct xa_limit limit = { .min = 0, .max = hwsize - 1 };
		unsigned int index;

		/* Let the xarray allocate a free index within the limit */
		ret = xa_alloc(xa, &index, desc, limit, GFP_KERNEL);
		if (ret)
			goto fail;

		desc->msi_index = index;
		return 0;
	} else {
		if (index >= hwsize) {
			ret = -ERANGE;
			goto fail;
		}

		desc->msi_index = index;
		ret = xa_insert(xa, index, desc, GFP_KERNEL);
		if (ret)
			goto fail;
		return 0;
	}
fail:
	msi_free_desc(desc);
	return ret;
}
/* Context-After
 * /**
 *  * msi_domain_insert_msi_desc - Allocate and initialize a MSI descriptor and
 *  *				insert it at @init_desc->msi_index
 *  *
 *  * @dev:	Pointer to the device for which the descriptor is allocated
 *  * @domid:	The id of the interrupt domain to which the desriptor is added
 *  * @init_desc:	Pointer to an MSI descriptor to initialize the new descriptor
 *  *
 *  * Return: 0 on success or an appropriate failure code.
 *  */
 * int msi_domain_insert_msi_desc(struct device *dev, unsigned int domid,
 * 			       struct msi_desc *init_desc)
 * {
 * 	struct msi_desc *desc;
 * 
 * 	lockdep_assert_held(&dev->msi.data->mutex);
 * 
 * 	desc = msi_alloc_desc(dev, init_desc->nvec_used, init_desc->affinity);
 * 	if (!desc)
 */
