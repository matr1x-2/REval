/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_016 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 10 */
/* NLOC: 46 */
/* Subsystem: kernel */
/* Includes
 * #include <linux/btf.h>
 * #include <linux/kernel.h>
 * #include <linux/slab.h>
 * #include "trace_btf.h"
 */
/* Context-Before
 * 		return (const struct btf_param *)(func_proto + 1);
 * 	else
 * 		return NULL;
 * }
 * 
 * #define BTF_ANON_STACK_MAX	16
 * 
 * struct btf_anon_stack {
 * 	u32 tid;
 * 	u32 offset;
 * };
 * 
 * /*
 *  * Find a member of data structure/union by name and return it.
 *  * Return NULL if not found, or -EINVAL if parameter is invalid.
 *  * If the member is an member of anonymous union/structure, the offset
 *  * of that anonymous union/structure is stored into @anon_offset. Caller
 *  * can calculate the correct offset from the root data structure by
 *  * adding anon_offset to the member's offset.
 *  */
 */
const struct btf_member *btf_find_struct_member(struct btf *btf,
						const struct btf_type *type,
						const char *member_name,
						u32 *anon_offset)
{
	struct btf_anon_stack *anon_stack;
	const struct btf_member *member;
	u32 tid, cur_offset = 0;
	const char *name;
	int i, top = 0;

	anon_stack = kzalloc_objs(*anon_stack, BTF_ANON_STACK_MAX);
	if (!anon_stack)
		return ERR_PTR(-ENOMEM);

retry:
	if (!btf_type_is_struct(type)) {
		member = ERR_PTR(-EINVAL);
		goto out;
	}

	for_each_member(i, type, member) {
		if (!member->name_off) {
			/* Anonymous union/struct: push it for later use */
			if (btf_type_skip_modifiers(btf, member->type, &tid) &&
			    top < BTF_ANON_STACK_MAX) {
				anon_stack[top].tid = tid;
				anon_stack[top++].offset =
					cur_offset + member->offset;
			}
		} else {
			name = btf_name_by_offset(btf, member->name_off);
			if (name && !strcmp(member_name, name)) {
				if (anon_offset)
					*anon_offset = cur_offset;
				goto out;
			}
		}
	}
	if (top > 0) {
		/* Pop from the anonymous stack and retry */
		tid = anon_stack[--top].tid;
		cur_offset = anon_stack[top].offset;
		type = btf_type_by_id(btf, tid);
		goto retry;
	}
	member = NULL;

out:
	kfree(anon_stack);
	return member;
}
/* Context-After: <empty> */
