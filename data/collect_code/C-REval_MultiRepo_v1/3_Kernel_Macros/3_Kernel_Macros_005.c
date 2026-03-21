/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_005 */
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
 * 			kobject_put(&filter->kobj);
 * 			damon_sysfs_scheme_filters_rm_dirs(filters);
 * 			return err;
 * 		}
 * 
 * 		filters_arr[i] = filter;
 * 		filters->nr++;
 * 	}
 * 	return 0;
 * }
 * 
 * static ssize_t nr_filters_show(struct kobject *kobj,
 * 		struct kobj_attribute *attr, char *buf)
 * {
 * 	struct damon_sysfs_scheme_filters *filters = container_of(kobj,
 * 			struct damon_sysfs_scheme_filters, kobj);
 * 
 * 	return sysfs_emit(buf, "%d\n", filters->nr);
 * }
 */
static ssize_t nr_filters_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct damon_sysfs_scheme_filters *filters;
	int nr, err = kstrtoint(buf, 0, &nr);

	if (err)
		return err;
	if (nr < 0)
		return -EINVAL;

	filters = container_of(kobj, struct damon_sysfs_scheme_filters, kobj);

	if (!mutex_trylock(&damon_sysfs_lock))
		return -EBUSY;
	err = damon_sysfs_scheme_filters_add_dirs(filters, nr);
	mutex_unlock(&damon_sysfs_lock);
	if (err)
		return err;

	return count;
}
/* Context-After
 * static void damon_sysfs_scheme_filters_release(struct kobject *kobj)
 * {
 * 	kfree(container_of(kobj, struct damon_sysfs_scheme_filters, kobj));
 * }
 * 
 * static struct kobj_attribute damon_sysfs_scheme_filters_nr_attr =
 * 		__ATTR_RW_MODE(nr_filters, 0600);
 * 
 * static struct attribute *damon_sysfs_scheme_filters_attrs[] = {
 * 	&damon_sysfs_scheme_filters_nr_attr.attr,
 * 	NULL,
 * };
 * ATTRIBUTE_GROUPS(damon_sysfs_scheme_filters);
 * 
 * static const struct kobj_type damon_sysfs_scheme_filters_ktype = {
 * 	.release = damon_sysfs_scheme_filters_release,
 * 	.sysfs_ops = &kobj_sysfs_ops,
 * 	.default_groups = damon_sysfs_scheme_filters_groups,
 * };
 */
