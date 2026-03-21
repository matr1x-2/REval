/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_007 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 44 */
/* Subsystem: fs */
/* Includes
 * #include <linux/buffer_head.h>
 * #include <linux/slab.h>
 * #include <linux/blkdev.h>
 * #include <linux/backing-dev.h>
 * #include <linux/log2.h>
 * #include <linux/crc32.h>
 * #include "nilfs.h"
 * #include "segment.h"
 * #include "alloc.h"
 * #include "cpfile.h"
 * #include "sufile.h"
 * #include "dat.h"
 * #include "segbuf.h"
 */
/* Context-Before
 * 	n = nilfs->ns_cptree.rb_node;
 * 	while (n) {
 * 		root = rb_entry(n, struct nilfs_root, rb_node);
 * 
 * 		if (cno < root->cno) {
 * 			n = n->rb_left;
 * 		} else if (cno > root->cno) {
 * 			n = n->rb_right;
 * 		} else {
 * 			refcount_inc(&root->count);
 * 			spin_unlock(&nilfs->ns_cptree_lock);
 * 			return root;
 * 		}
 * 	}
 * 	spin_unlock(&nilfs->ns_cptree_lock);
 * 
 * 	return NULL;
 * }
 * 
 * struct nilfs_root *
 */
nilfs_find_or_create_root(struct the_nilfs *nilfs, __u64 cno)
{
	struct rb_node **p, *parent;
	struct nilfs_root *root, *new;
	int err;

	root = nilfs_lookup_root(nilfs, cno);
	if (root)
		return root;

	new = kzalloc_obj(*root);
	if (!new)
		return NULL;

	spin_lock(&nilfs->ns_cptree_lock);

	p = &nilfs->ns_cptree.rb_node;
	parent = NULL;

	while (*p) {
		parent = *p;
		root = rb_entry(parent, struct nilfs_root, rb_node);

		if (cno < root->cno) {
			p = &(*p)->rb_left;
		} else if (cno > root->cno) {
			p = &(*p)->rb_right;
		} else {
			refcount_inc(&root->count);
			spin_unlock(&nilfs->ns_cptree_lock);
			kfree(new);
			return root;
		}
	}

	new->cno = cno;
	new->ifile = NULL;
	new->nilfs = nilfs;
	refcount_set(&new->count, 1);
	atomic64_set(&new->inodes_count, 0);
	atomic64_set(&new->blocks_count, 0);

	rb_link_node(&new->rb_node, parent, p);
	rb_insert_color(&new->rb_node, &nilfs->ns_cptree);

	spin_unlock(&nilfs->ns_cptree_lock);

	err = nilfs_sysfs_create_snapshot_group(new);
	if (err) {
		kfree(new);
		new = NULL;
	}

	return new;
}
/* Context-After
 * void nilfs_put_root(struct nilfs_root *root)
 * {
 * 	struct the_nilfs *nilfs = root->nilfs;
 * 
 * 	if (refcount_dec_and_lock(&root->count, &nilfs->ns_cptree_lock)) {
 * 		rb_erase(&root->rb_node, &nilfs->ns_cptree);
 * 		spin_unlock(&nilfs->ns_cptree_lock);
 * 
 * 		nilfs_sysfs_delete_snapshot_group(root);
 * 		iput(root->ifile);
 * 
 * 		kfree(root);
 * 	}
 * }
 */
