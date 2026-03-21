/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_024 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 60 */
/* NLOC: 193 */
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
 * 		seq_printf(m, "%s%s", i ? "|" : "", opt_list->items[i]);
 * }
 * 
 * static void ima_policy_show_appraise_algos(struct seq_file *m,
 * 					   unsigned int allowed_hashes)
 * {
 * 	int idx, list_size = 0;
 * 
 * 	for (idx = 0; idx < HASH_ALGO__LAST; idx++) {
 * 		if (!(allowed_hashes & (1U << idx)))
 * 			continue;
 * 
 * 		/* only add commas if the list contains multiple entries */
 * 		if (list_size++)
 * 			seq_puts(m, ",");
 * 
 * 		seq_puts(m, hash_algo_name[idx]);
 * 	}
 * }
 */
int ima_policy_show(struct seq_file *m, void *v)
{
	struct ima_rule_entry *entry = v;
	int i;
	char tbuf[64] = {0,};
	int offset = 0;

	rcu_read_lock();

	/* Do not print rules with inactive LSM labels */
	for (i = 0; i < MAX_LSM_RULES; i++) {
		if (entry->lsm[i].args_p && !entry->lsm[i].rule) {
			rcu_read_unlock();
			return 0;
		}
	}

	if (entry->action & MEASURE)
		seq_puts(m, pt(Opt_measure));
	if (entry->action & DONT_MEASURE)
		seq_puts(m, pt(Opt_dont_measure));
	if (entry->action & APPRAISE)
		seq_puts(m, pt(Opt_appraise));
	if (entry->action & DONT_APPRAISE)
		seq_puts(m, pt(Opt_dont_appraise));
	if (entry->action & AUDIT)
		seq_puts(m, pt(Opt_audit));
	if (entry->action & DONT_AUDIT)
		seq_puts(m, pt(Opt_dont_audit));
	if (entry->action & HASH)
		seq_puts(m, pt(Opt_hash));
	if (entry->action & DONT_HASH)
		seq_puts(m, pt(Opt_dont_hash));

	seq_puts(m, " ");

	if (entry->flags & IMA_FUNC)
		policy_func_show(m, entry->func);

	if ((entry->flags & IMA_MASK) || (entry->flags & IMA_INMASK)) {
		if (entry->flags & IMA_MASK)
			offset = 1;
		if (entry->mask & MAY_EXEC)
			seq_printf(m, pt(Opt_mask), mt(mask_exec) + offset);
		if (entry->mask & MAY_WRITE)
			seq_printf(m, pt(Opt_mask), mt(mask_write) + offset);
		if (entry->mask & MAY_READ)
			seq_printf(m, pt(Opt_mask), mt(mask_read) + offset);
		if (entry->mask & MAY_APPEND)
			seq_printf(m, pt(Opt_mask), mt(mask_append) + offset);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_FSMAGIC) {
		snprintf(tbuf, sizeof(tbuf), "0x%lx", entry->fsmagic);
		seq_printf(m, pt(Opt_fsmagic), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_FSNAME) {
		snprintf(tbuf, sizeof(tbuf), "%s", entry->fsname);
		seq_printf(m, pt(Opt_fsname), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_FS_SUBTYPE) {
		snprintf(tbuf, sizeof(tbuf), "%s", entry->fs_subtype);
		seq_printf(m, pt(Opt_fs_subtype), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_KEYRINGS) {
		seq_puts(m, "keyrings=");
		ima_show_rule_opt_list(m, entry->keyrings);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_LABEL) {
		seq_puts(m, "label=");
		ima_show_rule_opt_list(m, entry->label);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_PCR) {
		snprintf(tbuf, sizeof(tbuf), "%d", entry->pcr);
		seq_printf(m, pt(Opt_pcr), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_FSUUID) {
		seq_printf(m, "fsuuid=%pU", &entry->fsuuid);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_UID) {
		snprintf(tbuf, sizeof(tbuf), "%d", __kuid_val(entry->uid));
		if (entry->uid_op == &uid_gt)
			seq_printf(m, pt(Opt_uid_gt), tbuf);
		else if (entry->uid_op == &uid_lt)
			seq_printf(m, pt(Opt_uid_lt), tbuf);
		else
			seq_printf(m, pt(Opt_uid_eq), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_EUID) {
		snprintf(tbuf, sizeof(tbuf), "%d", __kuid_val(entry->uid));
		if (entry->uid_op == &uid_gt)
			seq_printf(m, pt(Opt_euid_gt), tbuf);
		else if (entry->uid_op == &uid_lt)
			seq_printf(m, pt(Opt_euid_lt), tbuf);
		else
			seq_printf(m, pt(Opt_euid_eq), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_GID) {
		snprintf(tbuf, sizeof(tbuf), "%d", __kgid_val(entry->gid));
		if (entry->gid_op == &gid_gt)
			seq_printf(m, pt(Opt_gid_gt), tbuf);
		else if (entry->gid_op == &gid_lt)
			seq_printf(m, pt(Opt_gid_lt), tbuf);
		else
			seq_printf(m, pt(Opt_gid_eq), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_EGID) {
		snprintf(tbuf, sizeof(tbuf), "%d", __kgid_val(entry->gid));
		if (entry->gid_op == &gid_gt)
			seq_printf(m, pt(Opt_egid_gt), tbuf);
		else if (entry->gid_op == &gid_lt)
			seq_printf(m, pt(Opt_egid_lt), tbuf);
		else
			seq_printf(m, pt(Opt_egid_eq), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_FOWNER) {
		snprintf(tbuf, sizeof(tbuf), "%d", __kuid_val(entry->fowner));
		if (entry->fowner_op == &vfsuid_gt_kuid)
			seq_printf(m, pt(Opt_fowner_gt), tbuf);
		else if (entry->fowner_op == &vfsuid_lt_kuid)
			seq_printf(m, pt(Opt_fowner_lt), tbuf);
		else
			seq_printf(m, pt(Opt_fowner_eq), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_FGROUP) {
		snprintf(tbuf, sizeof(tbuf), "%d", __kgid_val(entry->fgroup));
		if (entry->fgroup_op == &vfsgid_gt_kgid)
			seq_printf(m, pt(Opt_fgroup_gt), tbuf);
		else if (entry->fgroup_op == &vfsgid_lt_kgid)
			seq_printf(m, pt(Opt_fgroup_lt), tbuf);
		else
			seq_printf(m, pt(Opt_fgroup_eq), tbuf);
		seq_puts(m, " ");
	}

	if (entry->flags & IMA_VALIDATE_ALGOS) {
		seq_puts(m, "appraise_algos=");
		ima_policy_show_appraise_algos(m, entry->allowed_algos);
		seq_puts(m, " ");
	}

	for (i = 0; i < MAX_LSM_RULES; i++) {
		if (entry->lsm[i].rule) {
			switch (i) {
			case LSM_OBJ_USER:
				seq_printf(m, pt(Opt_obj_user),
					   entry->lsm[i].args_p);
				break;
			case LSM_OBJ_ROLE:
				seq_printf(m, pt(Opt_obj_role),
					   entry->lsm[i].args_p);
				break;
			case LSM_OBJ_TYPE:
				seq_printf(m, pt(Opt_obj_type),
					   entry->lsm[i].args_p);
				break;
			case LSM_SUBJ_USER:
				seq_printf(m, pt(Opt_subj_user),
					   entry->lsm[i].args_p);
				break;
			case LSM_SUBJ_ROLE:
				seq_printf(m, pt(Opt_subj_role),
					   entry->lsm[i].args_p);
				break;
			case LSM_SUBJ_TYPE:
				seq_printf(m, pt(Opt_subj_type),
					   entry->lsm[i].args_p);
				break;
			}
			seq_puts(m, " ");
		}
	}
	if (entry->template)
		seq_printf(m, "template=%s ", entry->template->name);
	if (entry->flags & IMA_DIGSIG_REQUIRED) {
		if (entry->flags & IMA_VERITY_REQUIRED)
			seq_puts(m, "appraise_type=sigv3 ");
		else if (entry->flags & IMA_MODSIG_ALLOWED)
			seq_puts(m, "appraise_type=imasig|modsig ");
		else
			seq_puts(m, "appraise_type=imasig ");
	}
	if (entry->flags & IMA_VERITY_REQUIRED)
		seq_puts(m, "digest_type=verity ");
	if (entry->flags & IMA_PERMIT_DIRECTIO)
		seq_puts(m, "permit_directio ");
	rcu_read_unlock();
	seq_puts(m, "\n");
	return 0;
}
/* Context-After
 * #endif	/* CONFIG_IMA_READ_POLICY */
 * 
 * #if defined(CONFIG_IMA_APPRAISE) && defined(CONFIG_INTEGRITY_TRUSTED_KEYRING)
 * /*
 *  * ima_appraise_signature: whether IMA will appraise a given function using
 *  * an IMA digital signature. This is restricted to cases where the kernel
 *  * has a set of built-in trusted keys in order to avoid an attacker simply
 *  * loading additional keys.
 *  */
 * bool ima_appraise_signature(enum kernel_read_file_id id)
 * {
 * 	struct ima_rule_entry *entry;
 * 	bool found = false;
 * 	enum ima_hooks func;
 * 	struct list_head *ima_rules_tmp;
 * 
 * 	if (id >= READING_MAX_ID)
 * 		return false;
 * 
 * 	if (id == READING_KEXEC_IMAGE && !(ima_appraise & IMA_APPRAISE_ENFORCE)
 */
