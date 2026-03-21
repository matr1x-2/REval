/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_009 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 57 */
/* NLOC: 123 */
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
 * 			break;
 * 		}
 * 	}
 * 
 * 	return matched;
 * }
 * 
 * /**
 *  * ima_match_rules - determine whether an inode matches the policy rule.
 *  * @rule: a pointer to a rule
 *  * @idmap: idmap of the mount the inode was found from
 *  * @inode: a pointer to an inode
 *  * @cred: a pointer to a credentials structure for user validation
 *  * @prop: LSM properties of the task to be validated
 *  * @func: LIM hook identifier
 *  * @mask: requested action (MAY_READ | MAY_WRITE | MAY_APPEND | MAY_EXEC)
 *  * @func_data: func specific data, may be NULL
 *  *
 *  * Returns true on rule match, false on failure.
 *  */
 */
static bool ima_match_rules(struct ima_rule_entry *rule,
			    struct mnt_idmap *idmap,
			    struct inode *inode, const struct cred *cred,
			    struct lsm_prop *prop, enum ima_hooks func, int mask,
			    const char *func_data)
{
	int i;
	bool result = false;
	struct ima_rule_entry *lsm_rule = rule;
	bool rule_reinitialized = false;

	if ((rule->flags & IMA_FUNC) &&
	    (rule->func != func && func != POST_SETATTR))
		return false;

	switch (func) {
	case KEY_CHECK:
	case CRITICAL_DATA:
		return ((rule->func == func) &&
			ima_match_rule_data(rule, func_data, cred));
	default:
		break;
	}

	if ((rule->flags & IMA_MASK) &&
	    (rule->mask != mask && func != POST_SETATTR))
		return false;
	if ((rule->flags & IMA_INMASK) &&
	    (!(rule->mask & mask) && func != POST_SETATTR))
		return false;
	if ((rule->flags & IMA_FSMAGIC)
	    && rule->fsmagic != inode->i_sb->s_magic)
		return false;
	if ((rule->flags & IMA_FSNAME)
	    && strcmp(rule->fsname, inode->i_sb->s_type->name))
		return false;
	if (rule->flags & IMA_FS_SUBTYPE) {
		if (!inode->i_sb->s_subtype)
			return false;
		if (strcmp(rule->fs_subtype, inode->i_sb->s_subtype))
			return false;
	}
	if ((rule->flags & IMA_FSUUID) &&
	    !uuid_equal(&rule->fsuuid, &inode->i_sb->s_uuid))
		return false;
	if ((rule->flags & IMA_UID) && !rule->uid_op(cred->uid, rule->uid))
		return false;
	if (rule->flags & IMA_EUID) {
		if (has_capability_noaudit(current, CAP_SETUID)) {
			if (!rule->uid_op(cred->euid, rule->uid)
			    && !rule->uid_op(cred->suid, rule->uid)
			    && !rule->uid_op(cred->uid, rule->uid))
				return false;
		} else if (!rule->uid_op(cred->euid, rule->uid))
			return false;
	}
	if ((rule->flags & IMA_GID) && !rule->gid_op(cred->gid, rule->gid))
		return false;
	if (rule->flags & IMA_EGID) {
		if (has_capability_noaudit(current, CAP_SETGID)) {
			if (!rule->gid_op(cred->egid, rule->gid)
			    && !rule->gid_op(cred->sgid, rule->gid)
			    && !rule->gid_op(cred->gid, rule->gid))
				return false;
		} else if (!rule->gid_op(cred->egid, rule->gid))
			return false;
	}
	if ((rule->flags & IMA_FOWNER) &&
	    !rule->fowner_op(i_uid_into_vfsuid(idmap, inode),
			     rule->fowner))
		return false;
	if ((rule->flags & IMA_FGROUP) &&
	    !rule->fgroup_op(i_gid_into_vfsgid(idmap, inode),
			     rule->fgroup))
		return false;
	for (i = 0; i < MAX_LSM_RULES; i++) {
		int rc = 0;
		struct lsm_prop inode_prop = { };

		if (!lsm_rule->lsm[i].rule) {
			if (!lsm_rule->lsm[i].args_p)
				continue;
			else
				return false;
		}

retry:
		switch (i) {
		case LSM_OBJ_USER:
		case LSM_OBJ_ROLE:
		case LSM_OBJ_TYPE:
			security_inode_getlsmprop(inode, &inode_prop);
			rc = ima_filter_rule_match(&inode_prop,
						   lsm_rule->lsm[i].type,
						   Audit_equal,
						   lsm_rule->lsm[i].rule);
			break;
		case LSM_SUBJ_USER:
		case LSM_SUBJ_ROLE:
		case LSM_SUBJ_TYPE:
			rc = ima_filter_rule_match(prop, lsm_rule->lsm[i].type,
						   Audit_equal,
						   lsm_rule->lsm[i].rule);
			break;
		default:
			break;
		}

		if (rc == -ESTALE && !rule_reinitialized) {
			lsm_rule = ima_lsm_copy_rule(rule, GFP_ATOMIC);
			if (lsm_rule) {
				rule_reinitialized = true;
				goto retry;
			}
		}
		if (rc <= 0) {
			result = false;
			goto out;
		}
	}
	result = true;

out:
	if (rule_reinitialized) {
		for (i = 0; i < MAX_LSM_RULES; i++)
			ima_filter_rule_free(lsm_rule->lsm[i].rule);
		kfree(lsm_rule);
	}
	return result;
}
/* Context-After
 * /*
 *  * In addition to knowing that we need to appraise the file in general,
 *  * we need to differentiate between calling hooks, for hook specific rules.
 *  */
 * static int get_subaction(struct ima_rule_entry *rule, enum ima_hooks func)
 * {
 * 	if (!(rule->flags & IMA_FUNC))
 * 		return IMA_FILE_APPRAISE;
 * 
 * 	switch (func) {
 * 	case MMAP_CHECK:
 * 	case MMAP_CHECK_REQPROT:
 * 		return IMA_MMAP_APPRAISE;
 * 	case BPRM_CHECK:
 * 		return IMA_BPRM_APPRAISE;
 * 	case CREDS_CHECK:
 * 		return IMA_CREDS_APPRAISE;
 * 	case FILE_CHECK:
 * 	case POST_SETATTR:
 */
