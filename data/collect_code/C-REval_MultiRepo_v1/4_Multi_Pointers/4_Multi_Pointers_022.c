/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_022 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 35 */
/* Subsystem: arch */
/* Includes
 * #include <linux/list.h>
 * #include <linux/pci.h>
 * #include <linux/rbtree.h>
 * #include <linux/slab.h>
 * #include <linux/spinlock.h>
 * #include <linux/atomic.h>
 * #include <linux/debugfs.h>
 * #include <asm/pci-bridge.h>
 * #include <asm/ppc-pci.h>
 */
/* Context-Before
 * static void eeh_addr_cache_print(struct pci_io_addr_cache *cache)
 * {
 * 	struct rb_node *n;
 * 	int cnt = 0;
 * 
 * 	n = rb_first(&cache->rb_root);
 * 	while (n) {
 * 		struct pci_io_addr_range *piar;
 * 		piar = rb_entry(n, struct pci_io_addr_range, rb_node);
 * 		pr_info("PCI: %s addr range %d [%pap-%pap]: %s\n",
 * 		       (piar->flags & IORESOURCE_IO) ? "i/o" : "mem", cnt,
 * 		       &piar->addr_lo, &piar->addr_hi, pci_name(piar->pcidev));
 * 		cnt++;
 * 		n = rb_next(n);
 * 	}
 * }
 * #endif
 * 
 * /* Insert address range into the rb tree. */
 * static struct pci_io_addr_range *
 */
eeh_addr_cache_insert(struct pci_dev *dev, resource_size_t alo,
		      resource_size_t ahi, unsigned long flags)
{
	struct rb_node **p = &pci_io_addr_cache_root.rb_root.rb_node;
	struct rb_node *parent = NULL;
	struct pci_io_addr_range *piar;

	/* Walk tree, find a place to insert into tree */
	while (*p) {
		parent = *p;
		piar = rb_entry(parent, struct pci_io_addr_range, rb_node);
		if (ahi < piar->addr_lo) {
			p = &parent->rb_left;
		} else if (alo > piar->addr_hi) {
			p = &parent->rb_right;
		} else {
			if (dev != piar->pcidev ||
			    alo != piar->addr_lo || ahi != piar->addr_hi) {
				pr_warn("PIAR: overlapping address range\n");
			}
			return piar;
		}
	}
	piar = kzalloc_obj(struct pci_io_addr_range, GFP_ATOMIC);
	if (!piar)
		return NULL;

	piar->addr_lo = alo;
	piar->addr_hi = ahi;
	piar->edev = pci_dev_to_eeh_dev(dev);
	piar->pcidev = dev;
	piar->flags = flags;

	eeh_edev_dbg(piar->edev, "PIAR: insert range=[%pap:%pap]\n",
		 &alo, &ahi);

	rb_link_node(&piar->rb_node, parent, p);
	rb_insert_color(&piar->rb_node, &pci_io_addr_cache_root.rb_root);

	return piar;
}
/* Context-After
 * static void __eeh_addr_cache_insert_dev(struct pci_dev *dev)
 * {
 * 	struct eeh_dev *edev;
 * 	int i;
 * 
 * 	edev = pci_dev_to_eeh_dev(dev);
 * 	if (!edev) {
 * 		pr_warn("PCI: no EEH dev found for %s\n",
 * 			pci_name(dev));
 * 		return;
 * 	}
 * 
 * 	/* Skip any devices for which EEH is not enabled. */
 * 	if (!edev->pe) {
 * 		dev_dbg(&dev->dev, "EEH: Skip building address cache\n");
 * 		return;
 * 	}
 * 
 * 	/*
 */
