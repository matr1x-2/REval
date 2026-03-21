/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_016 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 29 */
/* Subsystem: lib */
/* Includes
 * #include <linux/export.h>
 * #include <linux/list_sort.h>
 * #include <linux/ref_tracker.h>
 * #include <linux/slab.h>
 * #include <linux/stacktrace.h>
 * #include <linux/stackdepot.h>
 * #include <linux/seq_file.h>
 * #include <linux/xarray.h>
 * #include <linux/debugfs.h>
 */
/* Context-Before
 * 	list_for_each_entry_safe(tracker, n, &dir->quarantine, head) {
 * 		list_del(&tracker->head);
 * 		kfree(tracker);
 * 		dir->quarantine_avail++;
 * 	}
 * 	if (!list_empty(&dir->list)) {
 * 		ref_tracker_dir_print_locked(dir, 16);
 * 		leak = true;
 * 		list_for_each_entry_safe(tracker, n, &dir->list, head) {
 * 			list_del(&tracker->head);
 * 			kfree(tracker);
 * 		}
 * 	}
 * 	spin_unlock_irqrestore(&dir->lock, flags);
 * 	WARN_ON_ONCE(leak);
 * 	WARN_ON_ONCE(refcount_read(&dir->untracked) != 1);
 * 	WARN_ON_ONCE(refcount_read(&dir->no_tracker) != 1);
 * }
 * EXPORT_SYMBOL(ref_tracker_dir_exit);
 */
int ref_tracker_alloc(struct ref_tracker_dir *dir,
		      struct ref_tracker **trackerp,
		      gfp_t gfp)
{
	unsigned long entries[REF_TRACKER_STACK_ENTRIES];
	struct ref_tracker *tracker;
	unsigned int nr_entries;
	gfp_t gfp_mask = gfp | __GFP_NOWARN;
	unsigned long flags;

	WARN_ON_ONCE(dir->dead);

	if (!trackerp) {
		refcount_inc(&dir->no_tracker);
		return 0;
	}
	if (gfp & __GFP_DIRECT_RECLAIM)
		gfp_mask |= __GFP_NOFAIL;
	*trackerp = tracker = kzalloc_obj(*tracker, gfp_mask);
	if (unlikely(!tracker)) {
		pr_err_once("memory allocation failure, unreliable refcount tracker.\n");
		refcount_inc(&dir->untracked);
		return -ENOMEM;
	}
	nr_entries = stack_trace_save(entries, ARRAY_SIZE(entries), 1);
	tracker->alloc_stack_handle = stack_depot_save(entries, nr_entries, gfp);

	spin_lock_irqsave(&dir->lock, flags);
	list_add(&tracker->head, &dir->list);
	spin_unlock_irqrestore(&dir->lock, flags);
	return 0;
}
/* Context-After
 * EXPORT_SYMBOL_GPL(ref_tracker_alloc);
 * 
 * int ref_tracker_free(struct ref_tracker_dir *dir,
 * 		     struct ref_tracker **trackerp)
 * {
 * 	unsigned long entries[REF_TRACKER_STACK_ENTRIES];
 * 	depot_stack_handle_t stack_handle;
 * 	struct ref_tracker *tracker;
 * 	unsigned int nr_entries;
 * 	unsigned long flags;
 * 
 * 	WARN_ON_ONCE(dir->dead);
 * 
 * 	if (!trackerp) {
 * 		refcount_dec(&dir->no_tracker);
 * 		return 0;
 * 	}
 * 	tracker = *trackerp;
 * 	if (!tracker) {
 * 		refcount_dec(&dir->untracked);
 */
