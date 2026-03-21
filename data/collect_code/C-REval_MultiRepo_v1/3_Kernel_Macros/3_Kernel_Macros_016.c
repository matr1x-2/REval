/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_016 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 24 */
/* Subsystem: sound */
/* Includes
 * #include <linux/io.h>
 * #include <linux/pci.h>
 * #include <linux/time.h>
 * #include <linux/mutex.h>
 * #include <sound/core.h>
 * #include "trident.h"
 */
/* Context-Before
 * 	}
 * }
 * static inline void set_silent_tlb(struct snd_trident *trident, int page)
 * {
 * 	int i;
 * 	page *= UNIT_PAGES;
 * 	for (i = 0; i < UNIT_PAGES; i++, page++)
 * 		__set_tlb_bus(trident, page, trident->tlb.silent_page->addr);
 * }
 * 
 * #endif /* PAGE_SIZE */
 * 
 * /* first and last (aligned) pages of memory block */
 * #define firstpg(blk)	(((struct snd_trident_memblk_arg *)snd_util_memblk_argptr(blk))->first_page)
 * #define lastpg(blk)	(((struct snd_trident_memblk_arg *)snd_util_memblk_argptr(blk))->last_page)
 * 
 * /*
 *  * search empty pages which may contain given size
 *  */
 * static struct snd_util_memblk *
 */
search_empty(struct snd_util_memhdr *hdr, int size)
{
	struct snd_util_memblk *blk;
	int page, psize;
	struct list_head *p;

	psize = get_aligned_page(size + ALIGN_PAGE_SIZE -1);
	page = 0;
	list_for_each(p, &hdr->block) {
		blk = list_entry(p, struct snd_util_memblk, list);
		if (page + psize <= firstpg(blk))
			goto __found_pages;
		page = lastpg(blk) + 1;
	}
	if (page + psize > MAX_ALIGN_PAGES)
		return NULL;

__found_pages:
	/* create a new memory block */
	blk = __snd_util_memblk_new(hdr, psize * ALIGN_PAGE_SIZE, p->prev);
	if (blk == NULL)
		return NULL;
	blk->offset = aligned_page_offset(page); /* set aligned offset */
	firstpg(blk) = page;
	lastpg(blk) = page + psize - 1;
	return blk;
}
/* Context-After
 * /*
 *  * check if the given pointer is valid for pages
 *  */
 * static int is_valid_page(struct snd_trident *trident, unsigned long ptr)
 * {
 * 	if (ptr & ~0x3fffffffUL) {
 * 		dev_err(trident->card->dev, "max memory size is 1GB!!\n");
 * 		return 0;
 * 	}
 * 	if (ptr & (SNDRV_TRIDENT_PAGE_SIZE-1)) {
 * 		dev_err(trident->card->dev, "page is not aligned\n");
 * 		return 0;
 * 	}
 * 	return 1;
 * }
 * 
 * /*
 *  * page allocation for DMA (Scatter-Gather version)
 */
