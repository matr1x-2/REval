/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_014 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 28 */
/* Subsystem: kernel */
/* Includes
 * #include <linux/bpf.h>
 * #include <linux/btf.h>
 * #include <linux/err.h>
 * #include <linux/slab.h>
 * #include <linux/spinlock.h>
 * #include <linux/vmalloc.h>
 * #include <net/ipv6.h>
 * #include <uapi/linux/btf.h>
 * #include <linux/btf_ids.h>
 * #include <asm/rqspinlock.h>
 * #include <linux/bpf_mem_alloc.h>
 */
/* Context-Before
 * 	bpf_map_init_from_attr(&trie->map, attr);
 * 	trie->data_size = attr->key_size -
 * 			  offsetof(struct bpf_lpm_trie_key_u8, data);
 * 	trie->max_prefixlen = trie->data_size * 8;
 * 
 * 	raw_res_spin_lock_init(&trie->lock);
 * 
 * 	/* Allocate intermediate and leaf nodes from the same allocator */
 * 	leaf_size = sizeof(struct lpm_trie_node) + trie->data_size +
 * 		    trie->map.value_size;
 * 	err = bpf_mem_alloc_init(&trie->ma, leaf_size, false);
 * 	if (err)
 * 		goto free_out;
 * 	return &trie->map;
 * 
 * free_out:
 * 	bpf_map_area_free(trie);
 * 	return ERR_PTR(err);
 * }
 */
static void trie_free(struct bpf_map *map)
{
	struct lpm_trie *trie = container_of(map, struct lpm_trie, map);
	struct lpm_trie_node __rcu **slot;
	struct lpm_trie_node *node;

	/* Always start at the root and walk down to a node that has no
	 * children. Then free that node, nullify its reference in the parent
	 * and start over.
	 */

	for (;;) {
		slot = &trie->root;

		for (;;) {
			node = rcu_dereference_protected(*slot, 1);
			if (!node)
				goto out;

			if (rcu_access_pointer(node->child[0])) {
				slot = &node->child[0];
				continue;
			}

			if (rcu_access_pointer(node->child[1])) {
				slot = &node->child[1];
				continue;
			}

			/* No bpf program may access the map, so freeing the
			 * node without waiting for the extra RCU GP.
			 */
			bpf_mem_cache_raw_free(node);
			RCU_INIT_POINTER(*slot, NULL);
			break;
		}
	}

out:
	bpf_mem_alloc_destroy(&trie->ma);
	bpf_map_area_free(trie);
}
/* Context-After
 * static int trie_get_next_key(struct bpf_map *map, void *_key, void *_next_key)
 * {
 * 	struct lpm_trie_node *node, *next_node = NULL, *parent, *search_root;
 * 	struct lpm_trie *trie = container_of(map, struct lpm_trie, map);
 * 	struct bpf_lpm_trie_key_u8 *key = _key, *next_key = _next_key;
 * 	struct lpm_trie_node **node_stack = NULL;
 * 	int err = 0, stack_ptr = -1;
 * 	unsigned int next_bit;
 * 	size_t matchlen = 0;
 * 
 * 	/* The get_next_key follows postorder. For the 4 node example in
 * 	 * the top of this file, the trie_get_next_key() returns the following
 * 	 * one after another:
 * 	 *   192.168.0.0/24
 * 	 *   192.168.1.0/24
 * 	 *   192.168.128.0/24
 * 	 *   192.168.0.0/16
 * 	 *
 * 	 * The idea is to return more specific keys before less specific ones.
 */
