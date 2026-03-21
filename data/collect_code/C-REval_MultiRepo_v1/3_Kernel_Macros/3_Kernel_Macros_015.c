/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_015 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 5 */
/* NLOC: 18 */
/* Subsystem: mm */
/* Includes
 * #include <linux/slab.h>
 * #include <linux/numa.h>
 * #include "sysfs-common.h"
 */
/* Context-Before
 * 			kobject_put(&dest->kobj);
 * 			damos_sysfs_dests_rm_dirs(dests);
 * 			return err;
 * 		}
 * 
 * 		dests_arr[i] = dest;
 * 		dests->nr++;
 * 	}
 * 	return 0;
 * }
 * 
 * static ssize_t nr_dests_show(struct kobject *kobj,
 * 		struct kobj_attribute *attr, char *buf)
 * {
 * 	struct damos_sysfs_dests *dests = container_of(kobj,
 * 			struct damos_sysfs_dests, kobj);
 * 
 * 	return sysfs_emit(buf, "%d\n", dests->nr);
 * }
 */
static ssize_t nr_dests_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct damos_sysfs_dests *dests;
	int nr, err = kstrtoint(buf, 0, &nr);

	if (err)
		return err;
	if (nr < 0)
		return -EINVAL;

	dests = container_of(kobj, struct damos_sysfs_dests, kobj);

	if (!mutex_trylock(&damon_sysfs_lock))
		return -EBUSY;
	err = damos_sysfs_dests_add_dirs(dests, nr);
	mutex_unlock(&damon_sysfs_lock);
	if (err)
		return err;

	return count;
}
/* Context-After
 * static void damos_sysfs_dests_release(struct kobject *kobj)
 * {
 * 	kfree(container_of(kobj, struct damos_sysfs_dests, kobj));
 * }
 * 
 * static struct kobj_attribute damos_sysfs_dests_nr_attr =
 * 		__ATTR_RW_MODE(nr_dests, 0600);
 * 
 * static struct attribute *damos_sysfs_dests_attrs[] = {
 * 	&damos_sysfs_dests_nr_attr.attr,
 * 	NULL,
 * };
 * ATTRIBUTE_GROUPS(damos_sysfs_dests);
 * 
 * static const struct kobj_type damos_sysfs_dests_ktype = {
 * 	.release = damos_sysfs_dests_release,
 * 	.sysfs_ops = &kobj_sysfs_ops,
 * 	.default_groups = damos_sysfs_dests_groups,
 * };
 */
