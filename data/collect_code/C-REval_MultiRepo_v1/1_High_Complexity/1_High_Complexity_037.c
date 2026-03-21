/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_037 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 31 */
/* NLOC: 130 */
/* Subsystem: fs */
/* Includes
 * #include <linux/sched.h>
 * #include <linux/slab.h>
 * #include <linux/blkdev.h>
 * #include <linux/list_sort.h>
 * #include <linux/iversion.h>
 * #include "misc.h"
 * #include "ctree.h"
 * #include "tree-log.h"
 * #include "disk-io.h"
 * #include "locking.h"
 * #include "backref.h"
 * #include "compression.h"
 * #include "qgroup.h"
 * #include "block-group.h"
 * #include "space-info.h"
 * #include "inode-item.h"
 * #include "fs.h"
 * #include "accessors.h"
 * #include "extent-tree.h"
 * #include "root-tree.h"
 */
/* Context-Before
 * 		range_start = range_end + 1;
 * 	}
 * 	ret = 0;
 * out:
 * 	btrfs_release_path(wc->subvol_path);
 * 	iput(&dir->vfs_inode);
 * 	return ret;
 * }
 * 
 * /*
 *  * the process_func used to replay items from the log tree.  This
 *  * gets called in two different stages.  The first stage just looks
 *  * for inodes and makes sure they are all copied into the subvolume.
 *  *
 *  * The second stage copies all the other item types from the log into
 *  * the subvolume.  The two stage approach is slower, but gets rid of
 *  * lots of complexity around inodes referencing other inodes that exist
 *  * only in the log (references come from either directory items or inode
 *  * back refs).
 *  */
 */
static int replay_one_buffer(struct extent_buffer *eb,
			     struct walk_control *wc, u64 gen, int level)
{
	int nritems;
	struct btrfs_tree_parent_check check = {
		.transid = gen,
		.level = level
	};
	struct btrfs_root *root = wc->root;
	struct btrfs_trans_handle *trans = wc->trans;
	int ret;

	if (level != 0)
		return 0;

	/*
	 * Set to NULL since it was not yet read and in case we abort log replay
	 * on error, we have no valid log tree leaf to dump.
	 */
	wc->log_leaf = NULL;
	ret = btrfs_read_extent_buffer(eb, &check);
	if (ret) {
		btrfs_abort_log_replay(wc, ret,
		       "failed to read log tree leaf %llu for root %llu",
				       eb->start, btrfs_root_id(root));
		return ret;
	}

	ASSERT(wc->subvol_path == NULL);
	wc->subvol_path = btrfs_alloc_path();
	if (!wc->subvol_path) {
		btrfs_abort_log_replay(wc, -ENOMEM, "failed to allocate path");
		return -ENOMEM;
	}

	wc->log_leaf = eb;

	nritems = btrfs_header_nritems(eb);
	for (wc->log_slot = 0; wc->log_slot < nritems; wc->log_slot++) {
		struct btrfs_inode_item *inode_item = NULL;

		btrfs_item_key_to_cpu(eb, &wc->log_key, wc->log_slot);

		if (wc->log_key.type == BTRFS_INODE_ITEM_KEY) {
			inode_item = btrfs_item_ptr(eb, wc->log_slot,
						    struct btrfs_inode_item);
			/*
			 * An inode with no links is either:
			 *
			 * 1) A tmpfile (O_TMPFILE) that got fsync'ed and never
			 *    got linked before the fsync, skip it, as replaying
			 *    it is pointless since it would be deleted later.
			 *    We skip logging tmpfiles, but it's always possible
			 *    we are replaying a log created with a kernel that
			 *    used to log tmpfiles;
			 *
			 * 2) A non-tmpfile which got its last link deleted
			 *    while holding an open fd on it and later got
			 *    fsynced through that fd. We always log the
			 *    parent inodes when inode->last_unlink_trans is
			 *    set to the current transaction, so ignore all the
			 *    inode items for this inode. We will delete the
			 *    inode when processing the parent directory with
			 *    replay_dir_deletes().
			 */
			if (btrfs_inode_nlink(eb, inode_item) == 0) {
				wc->ignore_cur_inode = true;
				continue;
			} else {
				wc->ignore_cur_inode = false;
			}
		}

		/* Inode keys are done during the first stage. */
		if (wc->log_key.type == BTRFS_INODE_ITEM_KEY &&
		    wc->stage == LOG_WALK_REPLAY_INODES) {
			u32 mode;

			ret = replay_xattr_deletes(wc);
			if (ret)
				break;
			mode = btrfs_inode_mode(eb, inode_item);
			if (S_ISDIR(mode)) {
				ret = replay_dir_deletes(wc, wc->log_key.objectid, false);
				if (ret)
					break;
			}
			ret = overwrite_item(wc);
			if (ret)
				break;

			/*
			 * Before replaying extents, truncate the inode to its
			 * size. We need to do it now and not after log replay
			 * because before an fsync we can have prealloc extents
			 * added beyond the inode's i_size. If we did it after,
			 * through orphan cleanup for example, we would drop
			 * those prealloc extents just after replaying them.
			 */
			if (S_ISREG(mode)) {
				struct btrfs_drop_extents_args drop_args = { 0 };
				struct btrfs_inode *inode;
				u64 from;

				inode = btrfs_iget_logging(wc->log_key.objectid, root);
				if (IS_ERR(inode)) {
					ret = PTR_ERR(inode);
					btrfs_abort_log_replay(wc, ret,
					       "failed to lookup inode %llu root %llu",
							       wc->log_key.objectid,
							       btrfs_root_id(root));
					break;
				}
				from = ALIGN(i_size_read(&inode->vfs_inode),
					     root->fs_info->sectorsize);
				drop_args.start = from;
				drop_args.end = (u64)-1;
				drop_args.drop_cache = true;
				drop_args.path = wc->subvol_path;
				ret = btrfs_drop_extents(trans, root, inode,  &drop_args);
				if (ret) {
					btrfs_abort_log_replay(wc, ret,
		       "failed to drop extents for inode %llu root %llu offset %llu",
							       btrfs_ino(inode),
							       btrfs_root_id(root),
							       from);
				} else {
					inode_sub_bytes(&inode->vfs_inode,
							drop_args.bytes_found);
					/* Update the inode's nbytes. */
					ret = btrfs_update_inode(trans, inode);
					if (ret)
						btrfs_abort_log_replay(wc, ret,
					       "failed to update inode %llu root %llu",
								       btrfs_ino(inode),
								       btrfs_root_id(root));
				}
				iput(&inode->vfs_inode);
				if (ret)
					break;
			}

			ret = link_to_fixup_dir(wc, wc->log_key.objectid);
			if (ret)
				break;
		}

		if (wc->ignore_cur_inode)
			continue;

		if (wc->log_key.type == BTRFS_DIR_INDEX_KEY &&
		    wc->stage == LOG_WALK_REPLAY_DIR_INDEX) {
			ret = replay_one_dir_item(wc);
			if (ret)
				break;
		}

		if (wc->stage < LOG_WALK_REPLAY_ALL)
			continue;

		/* these keys are simply copied */
		if (wc->log_key.type == BTRFS_XATTR_ITEM_KEY) {
			ret = overwrite_item(wc);
			if (ret)
				break;
		} else if (wc->log_key.type == BTRFS_INODE_REF_KEY ||
			   wc->log_key.type == BTRFS_INODE_EXTREF_KEY) {
			ret = add_inode_ref(wc);
			if (ret)
				break;
		} else if (wc->log_key.type == BTRFS_EXTENT_DATA_KEY) {
			ret = replay_one_extent(wc);
			if (ret)
				break;
		}
		/*
		 * We don't log BTRFS_DIR_ITEM_KEY keys anymore, only the
		 * BTRFS_DIR_INDEX_KEY items which we use to derive the
		 * BTRFS_DIR_ITEM_KEY items. If we are replaying a log from an
		 * older kernel with such keys, ignore them.
		 */
	}
	btrfs_free_path(wc->subvol_path);
	wc->subvol_path = NULL;
	return ret;
}
/* Context-After
 * static int clean_log_buffer(struct btrfs_trans_handle *trans,
 * 			    struct extent_buffer *eb)
 * {
 * 	struct btrfs_fs_info *fs_info = eb->fs_info;
 * 	struct btrfs_block_group *bg;
 * 
 * 	btrfs_tree_lock(eb);
 * 	btrfs_clear_buffer_dirty(trans, eb);
 * 	wait_on_extent_buffer_writeback(eb);
 * 	btrfs_tree_unlock(eb);
 * 
 * 	if (trans) {
 * 		int ret;
 * 
 * 		ret = btrfs_pin_reserved_extent(trans, eb);
 * 		if (ret)
 * 			btrfs_abort_transaction(trans, ret);
 * 		return ret;
 * 	}
 */
