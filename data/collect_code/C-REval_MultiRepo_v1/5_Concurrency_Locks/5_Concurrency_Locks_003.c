/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_003 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 26 */
/* Subsystem: drivers */
/* Includes
 * #include <drm/ttm/ttm_device.h>
 * #include <drm/ttm/ttm_range_manager.h>
 * #include "i915_drv.h"
 * #include "i915_scatterlist.h"
 * #include "i915_ttm_buddy_manager.h"
 * #include "intel_region_ttm.h"
 * #include "gem/i915_gem_region.h"
 * #include "gem/i915_gem_ttm.h" /* For the funcs/ops export only */
 */
/* Context-Before
 * 	ret = i915_ttm_buddy_man_init(bdev, mem_type, false,
 * 				      resource_size(&mem->region),
 * 				      resource_size(&mem->io),
 * 				      mem->min_page_size, PAGE_SIZE);
 * 	if (ret)
 * 		return ret;
 * 
 * 	mem->region_private = ttm_manager_type(bdev, mem_type);
 * 
 * 	return 0;
 * }
 * 
 * /**
 *  * intel_region_ttm_fini - Finalize a TTM region.
 *  * @mem: The memory region
 *  *
 *  * This functions takes down the TTM resource manager associated with the
 *  * memory region, and if it was registered with the TTM device,
 *  * removes that registration.
 *  */
 */
int intel_region_ttm_fini(struct intel_memory_region *mem)
{
	struct ttm_resource_manager *man = mem->region_private;
	int ret = -EBUSY;
	int count;

	/*
	 * Put the region's move fences. This releases requests that
	 * may hold on to contexts and vms that may hold on to buffer
	 * objects placed in this region.
	 */
	if (man)
		ttm_resource_manager_cleanup(man);

	/* Flush objects from region. */
	for (count = 0; count < 10; ++count) {
		i915_gem_flush_free_objects(mem->i915);

		mutex_lock(&mem->objects.lock);
		if (list_empty(&mem->objects.list))
			ret = 0;
		mutex_unlock(&mem->objects.lock);
		if (!ret)
			break;

		msleep(20);
		drain_workqueue(mem->i915->bdev.wq);
	}

	/* If we leaked objects, Don't free the region causing use after free */
	if (ret || !man)
		return ret;

	ret = i915_ttm_buddy_man_fini(&mem->i915->bdev,
				      intel_region_to_ttm_type(mem));
	GEM_WARN_ON(ret);
	mem->region_private = NULL;

	return ret;
}
/* Context-After
 * /**
 *  * intel_region_ttm_resource_to_rsgt -
 *  * Convert an opaque TTM resource manager resource to a refcounted sg_table.
 *  * @mem: The memory region.
 *  * @res: The resource manager resource obtained from the TTM resource manager.
 *  * @page_alignment: Required page alignment for each sg entry. Power of two.
 *  *
 *  * The gem backends typically use sg-tables for operations on the underlying
 *  * io_memory. So provide a way for the backends to translate the
 *  * nodes they are handed from TTM to sg-tables.
 *  *
 *  * Return: A malloced sg_table on success, an error pointer on failure.
 *  */
 * struct i915_refct_sgt *
 * intel_region_ttm_resource_to_rsgt(struct intel_memory_region *mem,
 * 				  struct ttm_resource *res,
 * 				  u32 page_alignment)
 * {
 * 	if (mem->is_range_manager) {
 */
