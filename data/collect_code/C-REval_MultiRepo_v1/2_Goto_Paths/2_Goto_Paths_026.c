/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_026 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 35 */
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
 * /*
 *  * fixup_free is called when:
 *  * - an active object is freed
 *  */
 * static bool __init fixup_free(void *addr, enum debug_obj_state state)
 * {
 * 	struct self_test *obj = addr;
 * 
 * 	switch (state) {
 * 	case ODEBUG_STATE_ACTIVE:
 * 		debug_object_deactivate(obj, &descr_type_test);
 * 		debug_object_free(obj, &descr_type_test);
 * 		return true;
 * 	default:
 * 		return false;
 * 	}
 * }
 * 
 * static int __init
 */
check_results(void *addr, enum debug_obj_state state, int fixups, int warnings)
{
	struct debug_bucket *db;
	struct debug_obj *obj;
	unsigned long flags;
	int res = -EINVAL;

	db = get_bucket((unsigned long) addr);

	raw_spin_lock_irqsave(&db->lock, flags);

	obj = lookup_object(addr, db);
	if (!obj && state != ODEBUG_STATE_NONE) {
		WARN(1, KERN_ERR "ODEBUG: selftest object not found\n");
		goto out;
	}
	if (obj && obj->state != state) {
		WARN(1, KERN_ERR "ODEBUG: selftest wrong state: %d != %d\n",
		       obj->state, state);
		goto out;
	}
	if (fixups != debug_objects_fixups) {
		WARN(1, KERN_ERR "ODEBUG: selftest fixups failed %d != %d\n",
		       fixups, debug_objects_fixups);
		goto out;
	}
	if (warnings != debug_objects_warnings) {
		WARN(1, KERN_ERR "ODEBUG: selftest warnings failed %d != %d\n",
		       warnings, debug_objects_warnings);
		goto out;
	}
	res = 0;
out:
	raw_spin_unlock_irqrestore(&db->lock, flags);
	if (res)
		debug_objects_enabled = false;
	return res;
}
/* Context-After
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
 * 
 * static bool __init debug_objects_selftest(void)
 * {
 * 	int fixups, oldfixups, warnings, oldwarnings;
 * 	unsigned long flags;
 * 
 * 	local_irq_save(flags);
 * 
 * 	fixups = oldfixups = debug_objects_fixups;
 */
