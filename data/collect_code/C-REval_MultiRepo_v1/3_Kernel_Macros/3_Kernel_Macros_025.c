/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_025 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 18 */
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
 * 	dev->msi.data->__domains[domid].domain = domain;
 * 
 * 	if (msi_domain_prepare_irqs(domain, dev, hwsize, &bundle->alloc_info)) {
 * 		dev->msi.data->__domains[domid].domain = NULL;
 * 		irq_domain_remove(domain);
 * 		return false;
 * 	}
 * 
 * 	/* @bundle and @fwnode_alloced are now in use. Prevent cleanup */
 * 	retain_and_null_ptr(bundle);
 * 	retain_and_null_ptr(fwnode_alloced);
 * 	return true;
 * }
 * 
 * /**
 *  * msi_remove_device_irq_domain - Free a device MSI interrupt domain
 *  * @dev:	Pointer to the device
 *  * @domid:	Domain id
 *  */
 */
void msi_remove_device_irq_domain(struct device *dev, unsigned int domid)
{
	struct fwnode_handle *fwnode = NULL;
	struct msi_domain_info *info;
	struct irq_domain *domain;

	guard(msi_descs_lock)(dev);
	domain = msi_get_device_domain(dev, domid);
	if (!domain || !irq_domain_is_msi_device(domain))
		return;

	dev->msi.data->__domains[domid].domain = NULL;
	info = domain->host_data;

	info->ops->msi_teardown(domain, info->alloc_data);

	if (irq_domain_is_msi_device(domain))
		fwnode = domain->fwnode;
	irq_domain_remove(domain);
	irq_domain_free_fwnode(fwnode);
	kfree(container_of(info, struct msi_domain_template, info));
}
/* Context-After
 * /**
 *  * msi_match_device_irq_domain - Match a device irq domain against a bus token
 *  * @dev:	Pointer to the device
 *  * @domid:	Domain id
 *  * @bus_token:	Bus token to match against the domain bus token
 *  *
 *  * Return: True if device domain exists and bus tokens match.
 *  */
 * bool msi_match_device_irq_domain(struct device *dev, unsigned int domid,
 * 				 enum irq_domain_bus_token bus_token)
 * {
 * 	struct msi_domain_info *info;
 * 	struct irq_domain *domain;
 * 
 * 	guard(msi_descs_lock)(dev);
 * 	domain = msi_get_device_domain(dev, domid);
 * 	if (domain && irq_domain_is_msi_device(domain)) {
 * 		info = domain->host_data;
 * 		return info->bus_token == bus_token;
 */
