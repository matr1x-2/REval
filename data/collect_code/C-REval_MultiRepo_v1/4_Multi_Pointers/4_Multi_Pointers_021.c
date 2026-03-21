/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_021 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 33 */
/* Subsystem: kernel */
/* Includes
 * #include <crypto/acompress.h>
 * #include <linux/module.h>
 * #include <linux/file.h>
 * #include <linux/delay.h>
 * #include <linux/bitops.h>
 * #include <linux/device.h>
 * #include <linux/bio.h>
 * #include <linux/blkdev.h>
 * #include <linux/swap.h>
 * #include <linux/swapops.h>
 * #include <linux/pm.h>
 * #include <linux/slab.h>
 * #include <linux/vmalloc.h>
 * #include <linux/cpumask.h>
 * #include <linux/atomic.h>
 * #include <linux/kthread.h>
 * #include <linux/crc32.h>
 * #include <linux/ktime.h>
 * #include "power.h"
 */
/* Context-Before
 * 	sector_t image;
 * 	unsigned int flags;	/* Flags to pass to the "boot" kernel */
 * 	char	orig_sig[10];
 * 	char	sig[10];
 * } __packed;
 * 
 * static struct swsusp_header *swsusp_header;
 * 
 * /*
 *  * The following functions are used for tracing the allocated swap pages, so
 *  * that they can be freed in case of an error.
 *  */
 * struct swsusp_extent {
 * 	struct rb_node node;
 * 	unsigned long start;
 * 	unsigned long end;
 * };
 * 
 * static struct rb_root swsusp_extents = RB_ROOT;
 */
static int swsusp_extents_insert(unsigned long swap_offset)
{
	struct rb_node **new = &(swsusp_extents.rb_node);
	struct rb_node *parent = NULL;
	struct swsusp_extent *ext;

	/* Figure out where to put the new node */
	while (*new) {
		ext = rb_entry(*new, struct swsusp_extent, node);
		parent = *new;
		if (swap_offset < ext->start) {
			/* Try to merge */
			if (swap_offset == ext->start - 1) {
				ext->start--;
				return 0;
			}
			new = &((*new)->rb_left);
		} else if (swap_offset > ext->end) {
			/* Try to merge */
			if (swap_offset == ext->end + 1) {
				ext->end++;
				return 0;
			}
			new = &((*new)->rb_right);
		} else {
			/* It already is in the tree */
			return -EINVAL;
		}
	}
	/* Add the new node and rebalance the tree. */
	ext = kzalloc_obj(struct swsusp_extent);
	if (!ext)
		return -ENOMEM;

	ext->start = swap_offset;
	ext->end = swap_offset;
	rb_link_node(&ext->node, parent, new);
	rb_insert_color(&ext->node, &swsusp_extents);
	return 0;
}
/* Context-After
 * sector_t alloc_swapdev_block(int swap)
 * {
 * 	unsigned long offset;
 * 
 * 	/*
 * 	 * Allocate a swap page and register that it has been allocated, so that
 * 	 * it can be freed in case of an error.
 * 	 */
 * 	offset = swp_offset(swap_alloc_hibernation_slot(swap));
 * 	if (offset) {
 * 		if (swsusp_extents_insert(offset))
 * 			swap_free_hibernation_slot(swp_entry(swap, offset));
 * 		else
 * 			return swapdev_block(swap, offset);
 * 	}
 * 	return 0;
 * }
 * 
 * void free_all_swap_pages(int swap)
 */
