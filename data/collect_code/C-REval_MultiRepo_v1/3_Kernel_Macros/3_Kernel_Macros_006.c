/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_006 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 16 */
/* Subsystem: block */
/* Includes
 * #include "bfq-iosched.h"
 */
/* Context-Before
 * 	if (next_in_service) {
 * 		bool new_budget_triggers_change =
 * 			bfq_update_parent_budget(next_in_service);
 * 
 * 		parent_sched_may_change = !sd->next_in_service ||
 * 			new_budget_triggers_change;
 * 	}
 * 
 * 	sd->next_in_service = next_in_service;
 * 
 * 	return parent_sched_may_change;
 * }
 * 
 * #ifdef CONFIG_BFQ_GROUP_IOSCHED
 * 
 * /*
 *  * Returns true if this budget changes may let next_in_service->parent
 *  * become the next_in_service entity for its parent entity.
 *  */
 */
static bool bfq_update_parent_budget(struct bfq_entity *next_in_service)
{
	struct bfq_entity *bfqg_entity;
	struct bfq_group *bfqg;
	struct bfq_sched_data *group_sd;
	bool ret = false;

	group_sd = next_in_service->sched_data;

	bfqg = container_of(group_sd, struct bfq_group, sched_data);
	/*
	 * bfq_group's my_entity field is not NULL only if the group
	 * is not the root group. We must not touch the root entity
	 * as it must never become an in-service entity.
	 */
	bfqg_entity = bfqg->my_entity;
	if (bfqg_entity) {
		if (bfqg_entity->budget > next_in_service->budget)
			ret = true;
		bfqg_entity->budget = next_in_service->budget;
	}

	return ret;
}
/* Context-After
 * /*
 *  * This function tells whether entity stops being a candidate for next
 *  * service, according to the restrictive definition of the field
 *  * next_in_service. In particular, this function is invoked for an
 *  * entity that is about to be set in service.
 *  *
 *  * If entity is a queue, then the entity is no longer a candidate for
 *  * next service according to the that definition, because entity is
 *  * about to become the in-service queue. This function then returns
 *  * true if entity is a queue.
 *  *
 *  * In contrast, entity could still be a candidate for next service if
 *  * it is not a queue, and has more than one active child. In fact,
 *  * even if one of its children is about to be set in service, other
 *  * active children may still be the next to serve, for the parent
 *  * entity, even according to the above definition. As a consequence, a
 *  * non-queue entity is not a candidate for next-service only if it has
 *  * only one active child. And only if this condition holds, then this
 *  * function returns true for a non-queue entity.
 */
