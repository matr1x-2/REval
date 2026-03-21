/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_001 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 30 */
/* Subsystem: kernel */
/* Includes
 * #include <linux/hash.h>
 * #include <linux/bpf.h>
 * #include <linux/filter.h>
 * #include <linux/static_call.h>
 */
/* Context-Before
 * 		/* Prepare the dispatcher in d->rw_image. Then use
 * 		 * bpf_arch_text_copy to update d->image, which is RO+X.
 * 		 */
 * 		if (bpf_dispatcher_prepare(d, new, tmp))
 * 			return;
 * 		if (IS_ERR(bpf_arch_text_copy(new, tmp, PAGE_SIZE / 2)))
 * 			return;
 * 	}
 * 
 * 	__BPF_DISPATCHER_UPDATE(d, new ?: (void *)&bpf_dispatcher_nop_func);
 * 
 * 	/* Make sure all the callers executing the previous/old half of the
 * 	 * image leave it, so following update call can modify it safely.
 * 	 */
 * 	synchronize_rcu();
 * 
 * 	if (new)
 * 		d->image_off = noff;
 * }
 */
void bpf_dispatcher_change_prog(struct bpf_dispatcher *d, struct bpf_prog *from,
				struct bpf_prog *to)
{
	bool changed = false;
	int prev_num_progs;

	if (from == to)
		return;

	mutex_lock(&d->mutex);
	if (!d->image) {
		d->image = bpf_prog_pack_alloc(PAGE_SIZE, bpf_jit_fill_hole_with_zero);
		if (!d->image)
			goto out;
		d->rw_image = bpf_jit_alloc_exec(PAGE_SIZE);
		if (!d->rw_image) {
			bpf_prog_pack_free(d->image, PAGE_SIZE);
			d->image = NULL;
			goto out;
		}
		bpf_image_ksym_init(d->image, PAGE_SIZE, &d->ksym);
		bpf_image_ksym_add(&d->ksym);
	}

	prev_num_progs = d->num_progs;
	changed |= bpf_dispatcher_remove_prog(d, from);
	changed |= bpf_dispatcher_add_prog(d, to);

	if (!changed)
		goto out;

	bpf_dispatcher_update(d, prev_num_progs);
out:
	mutex_unlock(&d->mutex);
}
/* Context-After: <empty> */
