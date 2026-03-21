/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_024 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 9 */
/* NLOC: 51 */
/* Subsystem: block */
/* Includes
 * #include <linux/blkdev.h>
 * #include <linux/fs.h>
 * #include <linux/slab.h>
 * #include "check.h"
 */
/* Context-Before
 * /* partition flags */
 * #define PF_RDONLY                   0x01 /* Device is read only */
 * #define PF_POWERUP_LOCK             0x02 /* Always locked after reset */
 * 
 * struct cmdline_subpart {
 * 	char name[BDEVNAME_SIZE]; /* partition name, such as 'rootfs' */
 * 	sector_t from;
 * 	sector_t size;
 * 	int flags;
 * 	struct cmdline_subpart *next_subpart;
 * };
 * 
 * struct cmdline_parts {
 * 	char name[BDEVNAME_SIZE]; /* block device, such as 'mmcblk0' */
 * 	unsigned int nr_subparts;
 * 	struct cmdline_subpart *subpart;
 * 	struct cmdline_parts *next_parts;
 * };
 */
static int parse_subpart(struct cmdline_subpart **subpart, char *partdef)
{
	int ret = 0;
	struct cmdline_subpart *new_subpart;

	*subpart = NULL;

	new_subpart = kzalloc_obj(struct cmdline_subpart);
	if (!new_subpart)
		return -ENOMEM;

	if (*partdef == '-') {
		new_subpart->size = (sector_t)(~0ULL);
		partdef++;
	} else {
		new_subpart->size = (sector_t)memparse(partdef, &partdef);
		if (new_subpart->size < (sector_t)PAGE_SIZE) {
			pr_warn("cmdline partition size is invalid.");
			ret = -EINVAL;
			goto fail;
		}
	}

	if (*partdef == '@') {
		partdef++;
		new_subpart->from = (sector_t)memparse(partdef, &partdef);
	} else {
		new_subpart->from = (sector_t)(~0ULL);
	}

	if (*partdef == '(') {
		partdef++;
		char *next = strsep(&partdef, ")");

		if (!next) {
			pr_warn("cmdline partition format is invalid.");
			ret = -EINVAL;
			goto fail;
		}

		strscpy(new_subpart->name, next, sizeof(new_subpart->name));
	} else
		new_subpart->name[0] = '\0';

	new_subpart->flags = 0;

	if (!strncmp(partdef, "ro", 2)) {
		new_subpart->flags |= PF_RDONLY;
		partdef += 2;
	}

	if (!strncmp(partdef, "lk", 2)) {
		new_subpart->flags |= PF_POWERUP_LOCK;
		partdef += 2;
	}

	*subpart = new_subpart;
	return 0;
fail:
	kfree(new_subpart);
	return ret;
}
/* Context-After
 * static void free_subpart(struct cmdline_parts *parts)
 * {
 * 	struct cmdline_subpart *subpart;
 * 
 * 	while (parts->subpart) {
 * 		subpart = parts->subpart;
 * 		parts->subpart = subpart->next_subpart;
 * 		kfree(subpart);
 * 	}
 * }
 * 
 * static int parse_parts(struct cmdline_parts **parts, char *bdevdef)
 * {
 * 	int ret = -EINVAL;
 * 	char *next;
 * 	struct cmdline_subpart **next_subpart;
 * 	struct cmdline_parts *newparts;
 * 
 * 	*parts = NULL;
 */
