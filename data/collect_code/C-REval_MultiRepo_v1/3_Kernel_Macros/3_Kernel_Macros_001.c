/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_001 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 11 */
/* NLOC: 39 */
/* Subsystem: fs */
/* Includes
 * #include <linux/export.h>
 * #include <linux/fs.h>
 * #include <linux/filelock.h>
 * #include <linux/mm.h>
 * #include <linux/backing-dev.h>
 * #include <linux/hash.h>
 * #include <linux/swap.h>
 * #include <linux/security.h>
 * #include <linux/cdev.h>
 * #include <linux/memblock.h>
 * #include <linux/fsnotify.h>
 * #include <linux/fsverity.h>
 * #include <linux/mount.h>
 * #include <linux/posix_acl.h>
 * #include <linux/buffer_head.h> /* for inode_has_buffers */
 * #include <linux/ratelimit.h>
 * #include <linux/list_lru.h>
 * #include <linux/iversion.h>
 * #include <linux/rw_hint.h>
 * #include <linux/seq_file.h>
 */
/* Context-Before
 * 	spin_unlock(&inode_hash_lock);
 * }
 * EXPORT_SYMBOL(__insert_inode_hash);
 * 
 * /**
 *  *	__remove_inode_hash - remove an inode from the hash
 *  *	@inode: inode to unhash
 *  *
 *  *	Remove an inode from the superblock.
 *  */
 * void __remove_inode_hash(struct inode *inode)
 * {
 * 	spin_lock(&inode_hash_lock);
 * 	spin_lock(&inode->i_lock);
 * 	hlist_del_init_rcu(&inode->i_hash);
 * 	spin_unlock(&inode->i_lock);
 * 	spin_unlock(&inode_hash_lock);
 * }
 * EXPORT_SYMBOL(__remove_inode_hash);
 */
void dump_mapping(const struct address_space *mapping)
{
	struct inode *host;
	const struct address_space_operations *a_ops;
	struct hlist_node *dentry_first;
	struct dentry *dentry_ptr;
	struct dentry dentry;
	char fname[64] = {};
	unsigned long ino;

	/*
	 * If mapping is an invalid pointer, we don't want to crash
	 * accessing it, so probe everything depending on it carefully.
	 */
	if (get_kernel_nofault(host, &mapping->host) ||
	    get_kernel_nofault(a_ops, &mapping->a_ops)) {
		pr_warn("invalid mapping:%px\n", mapping);
		return;
	}

	if (!host) {
		pr_warn("aops:%ps\n", a_ops);
		return;
	}

	if (get_kernel_nofault(dentry_first, &host->i_dentry.first) ||
	    get_kernel_nofault(ino, &host->i_ino)) {
		pr_warn("aops:%ps invalid inode:%px\n", a_ops, host);
		return;
	}

	if (!dentry_first) {
		pr_warn("aops:%ps ino:%lx\n", a_ops, ino);
		return;
	}

	dentry_ptr = container_of(dentry_first, struct dentry, d_u.d_alias);
	if (get_kernel_nofault(dentry, dentry_ptr) ||
	    !dentry.d_parent || !dentry.d_name.name) {
		pr_warn("aops:%ps ino:%lx invalid dentry:%px\n",
				a_ops, ino, dentry_ptr);
		return;
	}

	if (strncpy_from_kernel_nofault(fname, dentry.d_name.name, 63) < 0)
		strscpy(fname, "<invalid>");
	/*
	 * Even if strncpy_from_kernel_nofault() succeeded,
	 * the fname could be unreliable
	 */
	pr_warn("aops:%ps ino:%lx dentry name(?):\"%s\"\n",
		a_ops, ino, fname);
}
/* Context-After
 * void clear_inode(struct inode *inode)
 * {
 * 	/*
 * 	 * Only IS_VERITY() inodes can have verity info, so start by checking
 * 	 * for IS_VERITY() (which is faster than retrieving the pointer to the
 * 	 * verity info).  This minimizes overhead for non-verity inodes.
 * 	 */
 * 	if (IS_ENABLED(CONFIG_FS_VERITY) && IS_VERITY(inode))
 * 		fsverity_cleanup_inode(inode);
 * 
 * 	/*
 * 	 * We have to cycle the i_pages lock here because reclaim can be in the
 * 	 * process of removing the last page (in __filemap_remove_folio())
 * 	 * and we must not free the mapping under it.
 * 	 */
 * 	xa_lock_irq(&inode->i_data.i_pages);
 * 	BUG_ON(inode->i_data.nrpages);
 * 	/*
 * 	 * Almost always, mapping_empty(&inode->i_data) here; but there are
 */
