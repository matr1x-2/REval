/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_049 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 5 */
/* NLOC: 30 */
/* Subsystem: arch */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/io.h>
 * #include <linux/list.h>
 * #include <linux/mempool.h>
 * #include <linux/mm.h>
 * #include <linux/elf.h>
 * #include <linux/ftrace.h>
 * #include <linux/module.h>
 * #include <linux/slab.h>
 * #include <asm/dwarf.h>
 * #include <asm/unwinder.h>
 * #include <asm/sections.h>
 * #include <linux/unaligned.h>
 * #include <asm/stacktrace.h>
 */
/* Context-Before
 * 	list_for_each_entry_safe(fde, ftmp, &mod->arch.fde_list, link) {
 * 		list_del(&fde->link);
 * 		rb_erase(&fde->node, &fde_root);
 * 		kfree(fde);
 * 	}
 * 
 * 	spin_unlock_irqrestore(&dwarf_fde_lock, flags);
 * }
 * #endif /* CONFIG_MODULES */
 * 
 * /**
 *  *	dwarf_unwinder_init - initialise the dwarf unwinder
 *  *
 *  *	Build the data structures describing the .dwarf_frame section to
 *  *	make it easier to lookup CIE and FDE entries. Because the
 *  *	.eh_frame section is packed as tightly as possible it is not
 *  *	easy to lookup the FDE for a given PC, so we build a list of FDE
 *  *	and CIE entries that make it easier.
 *  */
 */
static int __init dwarf_unwinder_init(void)
{
	int err = -ENOMEM;

	dwarf_frame_cachep = kmem_cache_create("dwarf_frames",
			sizeof(struct dwarf_frame), 0,
			SLAB_PANIC | SLAB_HWCACHE_ALIGN, NULL);

	dwarf_reg_cachep = kmem_cache_create("dwarf_regs",
			sizeof(struct dwarf_reg), 0,
			SLAB_PANIC | SLAB_HWCACHE_ALIGN, NULL);

	dwarf_frame_pool = mempool_create_slab_pool(DWARF_FRAME_MIN_REQ,
						    dwarf_frame_cachep);
	if (!dwarf_frame_pool)
		goto out;

	dwarf_reg_pool = mempool_create_slab_pool(DWARF_REG_MIN_REQ,
						  dwarf_reg_cachep);
	if (!dwarf_reg_pool)
		goto out;

	err = dwarf_parse_section(__start_eh_frame, __stop_eh_frame, NULL);
	if (err)
		goto out;

	err = unwinder_register(&dwarf_unwinder);
	if (err)
		goto out;

	dwarf_unwinder_ready = 1;

	return 0;

out:
	printk(KERN_ERR "Failed to initialise DWARF unwinder: %d\n", err);
	dwarf_unwinder_cleanup();
	return err;
}
/* Context-After
 * early_initcall(dwarf_unwinder_init);
 */
