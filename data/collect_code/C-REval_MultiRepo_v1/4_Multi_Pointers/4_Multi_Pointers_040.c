/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_040 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 17 */
/* Subsystem: block */
/* Includes
 * #include "bfq-iosched.h"
 */
/* Context-Before
 * 	if (entity == st->last_idle) {
 * 		next = rb_prev(&entity->rb_node);
 * 		st->last_idle = bfq_entity_of(next);
 * 	}
 * 
 * 	bfq_extract(&st->idle, entity);
 * 
 * 	if (bfqq)
 * 		list_del(&bfqq->bfqq_list);
 * }
 * 
 * /**
 *  * bfq_insert - generic tree insertion.
 *  * @root: tree root.
 *  * @entity: entity to insert.
 *  *
 *  * This is used for the idle and the active tree, since they are both
 *  * ordered by finish time.
 *  */
 */
static void bfq_insert(struct rb_root *root, struct bfq_entity *entity)
{
	struct bfq_entity *entry;
	struct rb_node **node = &root->rb_node;
	struct rb_node *parent = NULL;

	while (*node) {
		parent = *node;
		entry = rb_entry(parent, struct bfq_entity, rb_node);

		if (bfq_gt(entry->finish, entity->finish))
			node = &parent->rb_left;
		else
			node = &parent->rb_right;
	}

	rb_link_node(&entity->rb_node, parent, node);
	rb_insert_color(&entity->rb_node, root);

	entity->tree = root;
}
/* Context-After
 * /**
 *  * bfq_update_min - update the min_start field of a entity.
 *  * @entity: the entity to update.
 *  * @node: one of its children.
 *  *
 *  * This function is called when @entity may store an invalid value for
 *  * min_start due to updates to the active tree.  The function  assumes
 *  * that the subtree rooted at @node (which may be its left or its right
 *  * child) has a valid min_start value.
 *  */
 * static void bfq_update_min(struct bfq_entity *entity, struct rb_node *node)
 * {
 * 	struct bfq_entity *child;
 * 
 * 	if (node) {
 * 		child = rb_entry(node, struct bfq_entity, rb_node);
 * 		if (bfq_gt(entity->min_start, child->min_start))
 * 			entity->min_start = child->min_start;
 * 	}
 */
