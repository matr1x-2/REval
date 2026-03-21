/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_045 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 35 */
/* NLOC: 142 */
/* Subsystem: fs */
/* Includes
 * #include <linux/module.h>
 * #include <linux/init.h>
 * #include <linux/fs.h>
 * #include <linux/fs_context.h>
 * #include <linux/sched/mm.h>
 * #include <linux/statfs.h>
 * #include <linux/kthread.h>
 * #include <linux/parser.h>
 * #include <linux/mount.h>
 * #include <linux/seq_file.h>
 * #include <linux/proc_fs.h>
 * #include <linux/random.h>
 * #include <linux/exportfs.h>
 * #include <linux/blkdev.h>
 * #include <linux/quotaops.h>
 * #include <linux/f2fs_fs.h>
 * #include <linux/sysfs.h>
 * #include <linux/quota.h>
 * #include <linux/unicode.h>
 * #include <linux/part_stat.h>
 */
/* Context-Before
 * 		/* fix in-memory information all the time */
 * 		raw_super->segment_count = cpu_to_le32((main_end_blkaddr -
 * 				segment0_blkaddr) >> log_blocks_per_seg);
 * 
 * 		if (f2fs_readonly(sb) || f2fs_hw_is_readonly(sbi)) {
 * 			set_sbi_flag(sbi, SBI_NEED_SB_WRITE);
 * 			res = "internally";
 * 		} else {
 * 			err = __f2fs_commit_super(sbi, folio, index, false);
 * 			res = err ? "failed" : "done";
 * 		}
 * 		f2fs_info(sbi, "Fix alignment : %s, start(%u) end(%llu) block(%u)",
 * 			  res, main_blkaddr, seg_end_blkaddr,
 * 			  segment_count_main << log_blocks_per_seg);
 * 		if (err)
 * 			return true;
 * 	}
 * 	return false;
 * }
 */
static int sanity_check_raw_super(struct f2fs_sb_info *sbi,
					struct folio *folio, pgoff_t index)
{
	block_t segment_count, segs_per_sec, secs_per_zone, segment_count_main;
	block_t total_sections, blocks_per_seg;
	struct f2fs_super_block *raw_super = F2FS_SUPER_BLOCK(folio, index);
	size_t crc_offset = 0;
	__u32 crc = 0;

	if (le32_to_cpu(raw_super->magic) != F2FS_SUPER_MAGIC) {
		f2fs_info(sbi, "Magic Mismatch, valid(0x%x) - read(0x%x)",
			  F2FS_SUPER_MAGIC, le32_to_cpu(raw_super->magic));
		return -EINVAL;
	}

	/* Check checksum_offset and crc in superblock */
	if (__F2FS_HAS_FEATURE(raw_super, F2FS_FEATURE_SB_CHKSUM)) {
		crc_offset = le32_to_cpu(raw_super->checksum_offset);
		if (crc_offset !=
			offsetof(struct f2fs_super_block, crc)) {
			f2fs_info(sbi, "Invalid SB checksum offset: %zu",
				  crc_offset);
			return -EFSCORRUPTED;
		}
		crc = le32_to_cpu(raw_super->crc);
		if (crc != f2fs_crc32(raw_super, crc_offset)) {
			f2fs_info(sbi, "Invalid SB checksum value: %u", crc);
			return -EFSCORRUPTED;
		}
	}

	/* only support block_size equals to PAGE_SIZE */
	if (le32_to_cpu(raw_super->log_blocksize) != F2FS_BLKSIZE_BITS) {
		f2fs_info(sbi, "Invalid log_blocksize (%u), supports only %u",
			  le32_to_cpu(raw_super->log_blocksize),
			  F2FS_BLKSIZE_BITS);
		return -EFSCORRUPTED;
	}

	/* check log blocks per segment */
	if (le32_to_cpu(raw_super->log_blocks_per_seg) != 9) {
		f2fs_info(sbi, "Invalid log blocks per segment (%u)",
			  le32_to_cpu(raw_super->log_blocks_per_seg));
		return -EFSCORRUPTED;
	}

	/* Currently, support 512/1024/2048/4096/16K bytes sector size */
	if (le32_to_cpu(raw_super->log_sectorsize) >
				F2FS_MAX_LOG_SECTOR_SIZE ||
		le32_to_cpu(raw_super->log_sectorsize) <
				F2FS_MIN_LOG_SECTOR_SIZE) {
		f2fs_info(sbi, "Invalid log sectorsize (%u)",
			  le32_to_cpu(raw_super->log_sectorsize));
		return -EFSCORRUPTED;
	}
	if (le32_to_cpu(raw_super->log_sectors_per_block) +
		le32_to_cpu(raw_super->log_sectorsize) !=
			F2FS_MAX_LOG_SECTOR_SIZE) {
		f2fs_info(sbi, "Invalid log sectors per block(%u) log sectorsize(%u)",
			  le32_to_cpu(raw_super->log_sectors_per_block),
			  le32_to_cpu(raw_super->log_sectorsize));
		return -EFSCORRUPTED;
	}

	segment_count = le32_to_cpu(raw_super->segment_count);
	segment_count_main = le32_to_cpu(raw_super->segment_count_main);
	segs_per_sec = le32_to_cpu(raw_super->segs_per_sec);
	secs_per_zone = le32_to_cpu(raw_super->secs_per_zone);
	total_sections = le32_to_cpu(raw_super->section_count);

	/* blocks_per_seg should be 512, given the above check */
	blocks_per_seg = BIT(le32_to_cpu(raw_super->log_blocks_per_seg));

	if (segment_count > F2FS_MAX_SEGMENT ||
				segment_count < F2FS_MIN_SEGMENTS) {
		f2fs_info(sbi, "Invalid segment count (%u)", segment_count);
		return -EFSCORRUPTED;
	}

	if (total_sections > segment_count_main || total_sections < 1 ||
			segs_per_sec > segment_count || !segs_per_sec) {
		f2fs_info(sbi, "Invalid segment/section count (%u, %u x %u)",
			  segment_count, total_sections, segs_per_sec);
		return -EFSCORRUPTED;
	}

	if (segment_count_main != total_sections * segs_per_sec) {
		f2fs_info(sbi, "Invalid segment/section count (%u != %u * %u)",
			  segment_count_main, total_sections, segs_per_sec);
		return -EFSCORRUPTED;
	}

	if ((segment_count / segs_per_sec) < total_sections) {
		f2fs_info(sbi, "Small segment_count (%u < %u * %u)",
			  segment_count, segs_per_sec, total_sections);
		return -EFSCORRUPTED;
	}

	if (segment_count > (le64_to_cpu(raw_super->block_count) >> 9)) {
		f2fs_info(sbi, "Wrong segment_count / block_count (%u > %llu)",
			  segment_count, le64_to_cpu(raw_super->block_count));
		return -EFSCORRUPTED;
	}

	if (RDEV(0).path[0]) {
		block_t dev_seg_count = le32_to_cpu(RDEV(0).total_segments);
		int i = 1;

		while (i < MAX_DEVICES && RDEV(i).path[0]) {
			dev_seg_count += le32_to_cpu(RDEV(i).total_segments);
			i++;
		}
		if (segment_count != dev_seg_count) {
			f2fs_info(sbi, "Segment count (%u) mismatch with total segments from devices (%u)",
					segment_count, dev_seg_count);
			return -EFSCORRUPTED;
		}
	} else {
		if (__F2FS_HAS_FEATURE(raw_super, F2FS_FEATURE_BLKZONED) &&
					!bdev_is_zoned(sbi->sb->s_bdev)) {
			f2fs_info(sbi, "Zoned block device path is missing");
			return -EFSCORRUPTED;
		}
	}

	if (secs_per_zone > total_sections || !secs_per_zone) {
		f2fs_info(sbi, "Wrong secs_per_zone / total_sections (%u, %u)",
			  secs_per_zone, total_sections);
		return -EFSCORRUPTED;
	}
	if (le32_to_cpu(raw_super->extension_count) > F2FS_MAX_EXTENSION ||
			raw_super->hot_ext_count > F2FS_MAX_EXTENSION ||
			(le32_to_cpu(raw_super->extension_count) +
			raw_super->hot_ext_count) > F2FS_MAX_EXTENSION) {
		f2fs_info(sbi, "Corrupted extension count (%u + %u > %u)",
			  le32_to_cpu(raw_super->extension_count),
			  raw_super->hot_ext_count,
			  F2FS_MAX_EXTENSION);
		return -EFSCORRUPTED;
	}

	if (le32_to_cpu(raw_super->cp_payload) >=
				(blocks_per_seg - F2FS_CP_PACKS -
				NR_CURSEG_PERSIST_TYPE)) {
		f2fs_info(sbi, "Insane cp_payload (%u >= %u)",
			  le32_to_cpu(raw_super->cp_payload),
			  blocks_per_seg - F2FS_CP_PACKS -
			  NR_CURSEG_PERSIST_TYPE);
		return -EFSCORRUPTED;
	}

	/* check reserved ino info */
	if (le32_to_cpu(raw_super->node_ino) != 1 ||
		le32_to_cpu(raw_super->meta_ino) != 2 ||
		le32_to_cpu(raw_super->root_ino) != 3) {
		f2fs_info(sbi, "Invalid Fs Meta Ino: node(%u) meta(%u) root(%u)",
			  le32_to_cpu(raw_super->node_ino),
			  le32_to_cpu(raw_super->meta_ino),
			  le32_to_cpu(raw_super->root_ino));
		return -EFSCORRUPTED;
	}

	/* check CP/SIT/NAT/SSA/MAIN_AREA area boundary */
	if (sanity_check_area_boundary(sbi, folio, index))
		return -EFSCORRUPTED;

	return 0;
}
/* Context-After
 * int f2fs_sanity_check_ckpt(struct f2fs_sb_info *sbi)
 * {
 * 	unsigned int total, fsmeta;
 * 	struct f2fs_super_block *raw_super = F2FS_RAW_SUPER(sbi);
 * 	struct f2fs_checkpoint *ckpt = F2FS_CKPT(sbi);
 * 	unsigned int ovp_segments, reserved_segments;
 * 	unsigned int main_segs, blocks_per_seg;
 * 	unsigned int sit_segs, nat_segs;
 * 	unsigned int sit_bitmap_size, nat_bitmap_size;
 * 	unsigned int log_blocks_per_seg;
 * 	unsigned int segment_count_main;
 * 	unsigned int cp_pack_start_sum, cp_payload;
 * 	block_t user_block_count, valid_user_blocks;
 * 	block_t avail_node_count, valid_node_count;
 * 	unsigned int nat_blocks, nat_bits_bytes, nat_bits_blocks;
 * 	unsigned int sit_blk_cnt;
 * 	int i, j;
 * 
 * 	total = le32_to_cpu(raw_super->segment_count);
 */
