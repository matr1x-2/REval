/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_009 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 15 */
/* NLOC: 44 */
/* Subsystem: security */
/* Includes
 * #include <linux/init.h>
 * #include <linux/list.h>
 * #include <linux/kernel_read_file.h>
 * #include <linux/fs.h>
 * #include <linux/security.h>
 * #include <linux/magic.h>
 * #include <linux/parser.h>
 * #include <linux/slab.h>
 * #include <linux/rculist.h>
 * #include <linux/seq_file.h>
 * #include <linux/ima.h>
 * #include "ima.h"
 */
/* Context-Before
 *  * @idmap: idmap of the mount the inode was found from
 *  * @inode: pointer to an inode for which the policy decision is being made
 *  * @cred: pointer to a credentials structure for which the policy decision is
 *  *        being made
 *  * @prop: LSM properties of the task to be validated
 *  * @func: IMA hook identifier
 *  * @mask: requested action (MAY_READ | MAY_WRITE | MAY_APPEND | MAY_EXEC)
 *  * @flags: IMA actions to consider (e.g. IMA_MEASURE | IMA_APPRAISE)
 *  * @pcr: set the pcr to extend
 *  * @template_desc: the template that should be used for this rule
 *  * @func_data: func specific data, may be NULL
 *  * @allowed_algos: allowlist of hash algorithms for the IMA xattr
 *  *
 *  * Measure decision based on func/mask/fsmagic and LSM(subj/obj/type)
 *  * conditions.
 *  *
 *  * Since the IMA policy may be updated multiple times we need to lock the
 *  * list when walking it.  Reads are many orders of magnitude more numerous
 *  * than writes so ima_match_policy() is classical RCU candidate.
 *  */
 */
int ima_match_policy(struct mnt_idmap *idmap, struct inode *inode,
		     const struct cred *cred, struct lsm_prop *prop,
		     enum ima_hooks func, int mask, int flags, int *pcr,
		     struct ima_template_desc **template_desc,
		     const char *func_data, unsigned int *allowed_algos)
{
	struct ima_rule_entry *entry;
	int action = 0, actmask = flags | (flags << 1);
	struct list_head *ima_rules_tmp;

	if (template_desc && !*template_desc)
		*template_desc = ima_template_desc_current();

	rcu_read_lock();
	ima_rules_tmp = rcu_dereference(ima_rules);
	list_for_each_entry_rcu(entry, ima_rules_tmp, list) {

		if (!(entry->action & actmask))
			continue;

		if (!ima_match_rules(entry, idmap, inode, cred, prop,
				     func, mask, func_data))
			continue;

		action |= entry->flags & IMA_NONACTION_FLAGS;

		action |= entry->action & IMA_DO_MASK;
		if (entry->action & IMA_APPRAISE) {
			action |= get_subaction(entry, func);
			action &= ~IMA_HASH;
			if (ima_fail_unverifiable_sigs)
				action |= IMA_FAIL_UNVERIFIABLE_SIGS;

			if (allowed_algos &&
			    entry->flags & IMA_VALIDATE_ALGOS)
				*allowed_algos = entry->allowed_algos;
		}

		if (entry->action & IMA_DO_MASK)
			actmask &= ~(entry->action | entry->action << 1);
		else
			actmask &= ~(entry->action | entry->action >> 1);

		if ((pcr) && (entry->flags & IMA_PCR))
			*pcr = entry->pcr;

		if (template_desc && entry->template)
			*template_desc = entry->template;

		if (!actmask)
			break;
	}
	rcu_read_unlock();

	return action;
}
/* Context-After
 * /**
 *  * ima_update_policy_flags() - Update global IMA variables
 *  *
 *  * Update ima_policy_flag and ima_setxattr_allowed_hash_algorithms
 *  * based on the currently loaded policy.
 *  *
 *  * With ima_policy_flag, the decision to short circuit out of a function
 *  * or not call the function in the first place can be made earlier.
 *  *
 *  * With ima_setxattr_allowed_hash_algorithms, the policy can restrict the
 *  * set of hash algorithms accepted when updating the security.ima xattr of
 *  * a file.
 *  *
 *  * Context: called after a policy update and at system initialization.
 *  */
 * void ima_update_policy_flags(void)
 * {
 * 	struct ima_rule_entry *entry;
 * 	int new_policy_flag = 0;
 */
