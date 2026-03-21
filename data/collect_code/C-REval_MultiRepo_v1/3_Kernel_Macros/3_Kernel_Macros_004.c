/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_004 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
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
 * 	}
 * 
 * 	if (trie->data_size >= i + 1) {
 * 		prefixlen += 8 - fls(node->data[i] ^ key->data[i]);
 * 
 * 		if (prefixlen >= limit)
 * 			return limit;
 * 	}
 * 
 * 	return prefixlen;
 * }
 * 
 * static size_t longest_prefix_match(const struct lpm_trie *trie,
 * 				   const struct lpm_trie_node *node,
 * 				   const struct bpf_lpm_trie_key_u8 *key)
 * {
 * 	return __longest_prefix_match(trie, node, key);
 * }
 * 
 * /* Called from syscall or from eBPF program */
 */
static void *trie_lookup_elem(struct bpf_map *map, void *_key)
{
	struct lpm_trie *trie = container_of(map, struct lpm_trie, map);
	struct lpm_trie_node *node, *found = NULL;
	struct bpf_lpm_trie_key_u8 *key = _key;

	if (key->prefixlen > trie->max_prefixlen)
		return NULL;

	/* Start walking the trie from the root node ... */

	for (node = rcu_dereference_check(trie->root, rcu_read_lock_bh_held());
	     node;) {
		unsigned int next_bit;
		size_t matchlen;

		/* Determine the longest prefix of @node that matches @key.
		 * If it's the maximum possible prefix for this trie, we have
		 * an exact match and can return it directly.
		 */
		matchlen = __longest_prefix_match(trie, node, key);
		if (matchlen == trie->max_prefixlen) {
			found = node;
			break;
		}

		/* If the number of bits that match is smaller than the prefix
		 * length of @node, bail out and return the node we have seen
		 * last in the traversal (ie, the parent).
		 */
		if (matchlen < node->prefixlen)
			break;

		/* Consider this node as return candidate unless it is an
		 * artificially added intermediate one.
		 */
		if (!(node->flags & LPM_TREE_NODE_FLAG_IM))
			found = node;

		/* If the node match is fully satisfied, let's see if we can
		 * become more specific. Determine the next bit in the key and
		 * traverse down.
		 */
		next_bit = extract_bit(key->data, node->prefixlen);
		node = rcu_dereference_check(node->child[next_bit],
					     rcu_read_lock_bh_held());
	}

	if (!found)
		return NULL;

	return found->data + trie->data_size;
}
/* Context-After
 * static struct lpm_trie_node *lpm_trie_node_alloc(struct lpm_trie *trie,
 * 						 const void *value)
 * {
 * 	struct lpm_trie_node *node;
 * 
 * 	node = bpf_mem_cache_alloc(&trie->ma);
 * 
 * 	if (!node)
 * 		return NULL;
 * 
 * 	node->flags = 0;
 * 
 * 	if (value)
 * 		memcpy(node->data + trie->data_size, value,
 * 		       trie->map.value_size);
 * 
 * 	return node;
 * }
 */
