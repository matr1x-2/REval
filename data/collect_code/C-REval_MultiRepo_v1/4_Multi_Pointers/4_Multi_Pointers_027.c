/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_027 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 18 */
/* Subsystem: mm */
/* Includes
 * #include <linux/slab.h>
 * #include <linux/numa.h>
 * #include "sysfs-common.h"
 */
/* Context-Before
 * }
 * 
 * static int damon_sysfs_scheme_set_watermarks(struct damon_sysfs_scheme *scheme)
 * {
 * 	struct damon_sysfs_watermarks *watermarks =
 * 		damon_sysfs_watermarks_alloc(DAMOS_WMARK_NONE, 0, 0, 0, 0);
 * 	int err;
 * 
 * 	if (!watermarks)
 * 		return -ENOMEM;
 * 	err = kobject_init_and_add(&watermarks->kobj,
 * 			&damon_sysfs_watermarks_ktype, &scheme->kobj,
 * 			"watermarks");
 * 	if (err)
 * 		kobject_put(&watermarks->kobj);
 * 	else
 * 		scheme->watermarks = watermarks;
 * 	return err;
 * }
 */
static int damon_sysfs_scheme_set_filters(struct damon_sysfs_scheme *scheme,
		enum damos_sysfs_filter_handle_layer layer, const char *name,
		struct damon_sysfs_scheme_filters **filters_ptr)
{
	struct damon_sysfs_scheme_filters *filters =
		damon_sysfs_scheme_filters_alloc(layer);
	int err;

	if (!filters)
		return -ENOMEM;
	err = kobject_init_and_add(&filters->kobj,
			&damon_sysfs_scheme_filters_ktype, &scheme->kobj,
			"%s", name);
	if (err)
		kobject_put(&filters->kobj);
	else
		*filters_ptr = filters;
	return err;
}
/* Context-After
 * static int damos_sysfs_set_filter_dirs(struct damon_sysfs_scheme *scheme)
 * {
 * 	int err;
 * 
 * 	err = damon_sysfs_scheme_set_filters(scheme,
 * 			DAMOS_SYSFS_FILTER_HANDLE_LAYER_BOTH, "filters",
 * 			&scheme->filters);
 * 	if (err)
 * 		return err;
 * 	err = damon_sysfs_scheme_set_filters(scheme,
 * 			DAMOS_SYSFS_FILTER_HANDLE_LAYER_CORE, "core_filters",
 * 			&scheme->core_filters);
 * 	if (err)
 * 		goto put_filters_out;
 * 	err = damon_sysfs_scheme_set_filters(scheme,
 * 			DAMOS_SYSFS_FILTER_HANDLE_LAYER_OPS, "ops_filters",
 * 			&scheme->ops_filters);
 * 	if (err)
 * 		goto put_core_filters_out;
 */
