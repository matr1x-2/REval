/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_038 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 10 */
/* NLOC: 62 */
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
 * 			rb_node = &parent->rb_left;
 * 		else if (cie->cie_pointer >= cie_tmp->cie_pointer)
 * 			rb_node = &parent->rb_right;
 * 		else
 * 			WARN_ON(1);
 * 	}
 * 
 * 	rb_link_node(&cie->node, parent, rb_node);
 * 	rb_insert_color(&cie->node, &cie_root);
 * 
 * #ifdef CONFIG_MODULES
 * 	if (mod != NULL)
 * 		list_add_tail(&cie->link, &mod->arch.cie_list);
 * #endif
 * 
 * 	spin_unlock_irqrestore(&dwarf_cie_lock, flags);
 * 
 * 	return 0;
 * }
 */
static int dwarf_parse_fde(void *entry, u32 entry_type,
			   void *start, unsigned long len,
			   unsigned char *end, struct module *mod)
{
	struct rb_node **rb_node = &fde_root.rb_node;
	struct rb_node *parent = *rb_node;
	struct dwarf_fde *fde;
	struct dwarf_cie *cie;
	unsigned long flags;
	int count;
	void *p = start;

	fde = kzalloc_obj(*fde);
	if (!fde)
		return -ENOMEM;

	fde->length = len;

	/*
	 * In a .eh_frame section the CIE pointer is the
	 * delta between the address within the FDE
	 */
	fde->cie_pointer = (unsigned long)(p - entry_type - 4);

	cie = dwarf_lookup_cie(fde->cie_pointer);
	fde->cie = cie;

	if (cie->encoding)
		count = dwarf_read_encoded_value(p, &fde->initial_location,
						 cie->encoding);
	else
		count = dwarf_read_addr(p, &fde->initial_location);

	p += count;

	if (cie->encoding)
		count = dwarf_read_encoded_value(p, &fde->address_range,
						 cie->encoding & 0x0f);
	else
		count = dwarf_read_addr(p, &fde->address_range);

	p += count;

	if (fde->cie->flags & DWARF_CIE_Z_AUGMENTATION) {
		unsigned int length;
		count = dwarf_read_uleb128(p, &length);
		p += count + length;
	}

	/* Call frame instructions. */
	fde->instructions = p;
	fde->end = end;

	/* Add to list. */
	spin_lock_irqsave(&dwarf_fde_lock, flags);

	while (*rb_node) {
		struct dwarf_fde *fde_tmp;
		unsigned long tmp_start, tmp_end;
		unsigned long start, end;

		fde_tmp = rb_entry(*rb_node, struct dwarf_fde, node);

		start = fde->initial_location;
		end = fde->initial_location + fde->address_range;

		tmp_start = fde_tmp->initial_location;
		tmp_end = fde_tmp->initial_location + fde_tmp->address_range;

		parent = *rb_node;

		if (start < tmp_start)
			rb_node = &parent->rb_left;
		else if (start >= tmp_end)
			rb_node = &parent->rb_right;
		else
			WARN_ON(1);
	}

	rb_link_node(&fde->node, parent, rb_node);
	rb_insert_color(&fde->node, &fde_root);

#ifdef CONFIG_MODULES
	if (mod != NULL)
		list_add_tail(&fde->link, &mod->arch.fde_list);
#endif

	spin_unlock_irqrestore(&dwarf_fde_lock, flags);

	return 0;
}
/* Context-After
 * static void dwarf_unwinder_dump(struct task_struct *task,
 * 				struct pt_regs *regs,
 * 				unsigned long *sp,
 * 				const struct stacktrace_ops *ops,
 * 				void *data)
 * {
 * 	struct dwarf_frame *frame, *_frame;
 * 	unsigned long return_addr;
 * 
 * 	_frame = NULL;
 * 	return_addr = 0;
 * 
 * 	while (1) {
 * 		frame = dwarf_unwind_stack(return_addr, _frame);
 * 
 * 		if (_frame)
 * 			dwarf_free_frame(_frame);
 * 
 * 		_frame = frame;
 */
