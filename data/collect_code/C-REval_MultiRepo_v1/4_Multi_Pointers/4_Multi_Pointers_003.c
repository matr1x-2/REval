/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_003 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 37 */
/* Subsystem: block */
/* Includes
 * #include <linux/blkdev.h>
 * #include <linux/fs.h>
 * #include <linux/slab.h>
 * #include "check.h"
 */
/* Context-Before
 * 	}
 * 
 * 	*subpart = new_subpart;
 * 	return 0;
 * fail:
 * 	kfree(new_subpart);
 * 	return ret;
 * }
 * 
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
 */
static int parse_parts(struct cmdline_parts **parts, char *bdevdef)
{
	int ret = -EINVAL;
	char *next;
	struct cmdline_subpart **next_subpart;
	struct cmdline_parts *newparts;

	*parts = NULL;

	newparts = kzalloc_obj(struct cmdline_parts);
	if (!newparts)
		return -ENOMEM;

	next = strsep(&bdevdef, ":");
	if (!next) {
		pr_warn("cmdline partition has no block device.");
		goto fail;
	}

	strscpy(newparts->name, next, sizeof(newparts->name));
	newparts->nr_subparts = 0;

	next_subpart = &newparts->subpart;

	while ((next = strsep(&bdevdef, ","))) {
		ret = parse_subpart(next_subpart, next);
		if (ret)
			goto fail;

		newparts->nr_subparts++;
		next_subpart = &(*next_subpart)->next_subpart;
	}

	if (!newparts->subpart) {
		pr_warn("cmdline partition has no valid partition.");
		ret = -EINVAL;
		goto fail;
	}

	*parts = newparts;

	return 0;
fail:
	free_subpart(newparts);
	kfree(newparts);
	return ret;
}
/* Context-After
 * static void cmdline_parts_free(struct cmdline_parts **parts)
 * {
 * 	struct cmdline_parts *next_parts;
 * 
 * 	while (*parts) {
 * 		next_parts = (*parts)->next_parts;
 * 		free_subpart(*parts);
 * 		kfree(*parts);
 * 		*parts = next_parts;
 * 	}
 * }
 * 
 * static int cmdline_parts_parse(struct cmdline_parts **parts,
 * 		const char *cmdline)
 * {
 * 	int ret;
 * 	char *buf;
 * 	char *pbuf;
 * 	char *next;
 */
