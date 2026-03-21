/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_007 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 40 */
/* Subsystem: mm */
/* Includes
 * #include "vma_internal.h"
 * #include "vma.h"
 */
/* Context-Before
 * 	tlb_finish_mmu(&tlb);
 * 
 * 	vma_prev(&vmi);
 * 	/* Shrink the vma to just the new range */
 * 	return vma_shrink(&vmi, vma, new_start, new_end, vma->vm_pgoff);
 * }
 * 
 * /*
 *  * Establish the stack VMA in an execve'd process, located temporarily at the
 *  * maximum stack address provided by the architecture.
 *  *
 *  * We later relocate this downwards in relocate_vma_down().
 *  *
 *  * This function is almost certainly NOT what you want for anything other than
 *  * early executable initialisation.
 *  *
 *  * On success, returns 0 and sets *vmap to the stack VMA and *top_mem_p to the
 *  * maximum addressable location in the stack (that is capable of storing a
 *  * system word of data).
 *  */
 */
int create_init_stack_vma(struct mm_struct *mm, struct vm_area_struct **vmap,
			  unsigned long *top_mem_p)
{
	unsigned long flags = VM_STACK_FLAGS | VM_STACK_INCOMPLETE_SETUP;
	int err;
	struct vm_area_struct *vma = vm_area_alloc(mm);

	if (!vma)
		return -ENOMEM;

	vma_set_anonymous(vma);

	if (mmap_write_lock_killable(mm)) {
		err = -EINTR;
		goto err_free;
	}

	/*
	 * Need to be called with mmap write lock
	 * held, to avoid race with ksmd.
	 */
	err = ksm_execve(mm);
	if (err)
		goto err_ksm;

	/*
	 * Place the stack at the largest stack address the architecture
	 * supports. Later, we'll move this to an appropriate place. We don't
	 * use STACK_TOP because that can depend on attributes which aren't
	 * configured yet.
	 */
	BUILD_BUG_ON(VM_STACK_FLAGS & VM_STACK_INCOMPLETE_SETUP);
	vma->vm_end = STACK_TOP_MAX;
	vma->vm_start = vma->vm_end - PAGE_SIZE;
	if (pgtable_supports_soft_dirty())
		flags |= VM_SOFTDIRTY;
	vm_flags_init(vma, flags);
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

	err = insert_vm_struct(mm, vma);
	if (err)
		goto err;

	mm->stack_vm = mm->total_vm = 1;
	mmap_write_unlock(mm);
	*vmap = vma;
	*top_mem_p = vma->vm_end - sizeof(void *);
	return 0;

err:
	ksm_exit(mm);
err_ksm:
	mmap_write_unlock(mm);
err_free:
	*vmap = NULL;
	vm_area_free(vma);
	return err;
}
/* Context-After: <empty> */
