/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_014 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 17 */
/* NLOC: 62 */
/* Subsystem: lib */
/* Includes
 * #include <linux/cpu.h>
 * #include <linux/debugobjects.h>
 * #include <linux/debugfs.h>
 * #include <linux/hash.h>
 * #include <linux/kmemleak.h>
 * #include <linux/sched.h>
 * #include <linux/sched/loadavg.h>
 * #include <linux/sched/task_stack.h>
 * #include <linux/seq_file.h>
 * #include <linux/slab.h>
 * #include <linux/static_key.h>
 */
/* Context-Before
 * 	}
 * 	res = 0;
 * out:
 * 	raw_spin_unlock_irqrestore(&db->lock, flags);
 * 	if (res)
 * 		debug_objects_enabled = false;
 * 	return res;
 * }
 * 
 * static __initconst const struct debug_obj_descr descr_type_test = {
 * 	.name			= "selftest",
 * 	.is_static_object	= is_static_object,
 * 	.fixup_init		= fixup_init,
 * 	.fixup_activate		= fixup_activate,
 * 	.fixup_destroy		= fixup_destroy,
 * 	.fixup_free		= fixup_free,
 * };
 * 
 * static __initdata struct self_test obj = { .static_init = 0 };
 */
static bool __init debug_objects_selftest(void)
{
	int fixups, oldfixups, warnings, oldwarnings;
	unsigned long flags;

	local_irq_save(flags);

	fixups = oldfixups = debug_objects_fixups;
	warnings = oldwarnings = debug_objects_warnings;
	descr_test = &descr_type_test;

	debug_object_init(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_INIT, fixups, warnings))
		goto out;
	debug_object_activate(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_ACTIVE, fixups, warnings))
		goto out;
	debug_object_activate(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_ACTIVE, ++fixups, ++warnings))
		goto out;
	debug_object_deactivate(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_INACTIVE, fixups, warnings))
		goto out;
	debug_object_destroy(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_DESTROYED, fixups, warnings))
		goto out;
	debug_object_init(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_DESTROYED, fixups, ++warnings))
		goto out;
	debug_object_activate(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_DESTROYED, fixups, ++warnings))
		goto out;
	debug_object_deactivate(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_DESTROYED, fixups, ++warnings))
		goto out;
	debug_object_free(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_NONE, fixups, warnings))
		goto out;

	obj.static_init = 1;
	debug_object_activate(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_ACTIVE, fixups, warnings))
		goto out;
	debug_object_init(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_INIT, ++fixups, ++warnings))
		goto out;
	debug_object_free(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_NONE, fixups, warnings))
		goto out;

#ifdef CONFIG_DEBUG_OBJECTS_FREE
	debug_object_init(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_INIT, fixups, warnings))
		goto out;
	debug_object_activate(&obj, &descr_type_test);
	if (check_results(&obj, ODEBUG_STATE_ACTIVE, fixups, warnings))
		goto out;
	__debug_check_no_obj_freed(&obj, sizeof(obj));
	if (check_results(&obj, ODEBUG_STATE_NONE, ++fixups, ++warnings))
		goto out;
#endif
	pr_info("selftest passed\n");

out:
	debug_objects_fixups = oldfixups;
	debug_objects_warnings = oldwarnings;
	descr_test = NULL;

	local_irq_restore(flags);
	return debug_objects_enabled;
}
/* Context-After
 * #else
 * static inline bool debug_objects_selftest(void) { return true; }
 * #endif
 * 
 * /*
 *  * Called during early boot to initialize the hash buckets and link
 *  * the static object pool objects into the poll list. After this call
 *  * the object tracker is fully operational.
 *  */
 * void __init debug_objects_early_init(void)
 * {
 * 	int i;
 * 
 * 	for (i = 0; i < ODEBUG_HASH_SIZE; i++)
 * 		raw_spin_lock_init(&obj_hash[i].lock);
 * 
 * 	/* Keep early boot simple and add everything to the boot list */
 * 	for (i = 0; i < ODEBUG_POOL_SIZE; i++)
 * 		hlist_add_head(&obj_static_pool[i].node, &pool_boot);
 * }
 */
