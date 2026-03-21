/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_023 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 11 */
/* NLOC: 44 */
/* Subsystem: net */
/* Includes
 * #include <linux/gfp.h>
 * #include <linux/kernel.h>
 * #include <linux/random.h>
 * #include <linux/rculist.h>
 * #include "ieee80211_i.h"
 * #include "rate.h"
 * #include "mesh.h"
 */
/* Context-Before
 *  * Locking: this function must be called holding sta->mesh->plink_lock
 *  */
 * static inline void mesh_plink_fsm_restart(struct sta_info *sta)
 * {
 * 	lockdep_assert_held(&sta->mesh->plink_lock);
 * 	sta->mesh->plink_state = NL80211_PLINK_LISTEN;
 * 	sta->mesh->llid = sta->mesh->plid = sta->mesh->reason = 0;
 * 	sta->mesh->plink_retries = 0;
 * }
 * 
 * /*
 *  * mesh_set_short_slot_time - enable / disable ERP short slot time.
 *  *
 *  * The standard indirectly mandates mesh STAs to turn off short slot time by
 *  * disallowing advertising this (802.11-2012 8.4.1.4), but that doesn't mean we
 *  * can't be sneaky about it. Enable short slot time if all mesh STAs in the
 *  * MBSS support ERP rates.
 *  *
 *  * Returns BSS_CHANGED_ERP_SLOT or 0 for no change.
 *  */
 */
static u64 mesh_set_short_slot_time(struct ieee80211_sub_if_data *sdata)
{
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_supported_band *sband;
	struct sta_info *sta;
	u32 erp_rates = 0;
	u64 changed = 0;
	int i;
	bool short_slot = false;

	sband = ieee80211_get_sband(sdata);
	if (!sband)
		return changed;

	if (sband->band == NL80211_BAND_5GHZ) {
		/* (IEEE 802.11-2012 19.4.5) */
		short_slot = true;
		goto out;
	} else if (sband->band != NL80211_BAND_2GHZ) {
		goto out;
	}

	for (i = 0; i < sband->n_bitrates; i++)
		if (sband->bitrates[i].flags & IEEE80211_RATE_ERP_G)
			erp_rates |= BIT(i);

	if (!erp_rates)
		goto out;

	rcu_read_lock();
	list_for_each_entry_rcu(sta, &local->sta_list, list) {
		if (sdata != sta->sdata ||
		    sta->mesh->plink_state != NL80211_PLINK_ESTAB)
			continue;

		short_slot = false;
		if (erp_rates & sta->sta.deflink.supp_rates[sband->band])
			short_slot = true;
		 else
			break;
	}
	rcu_read_unlock();

out:
	if (sdata->vif.bss_conf.use_short_slot != short_slot) {
		sdata->vif.bss_conf.use_short_slot = short_slot;
		changed = BSS_CHANGED_ERP_SLOT;
		mpl_dbg(sdata, "mesh_plink %pM: ERP short slot time %d\n",
			sdata->vif.addr, short_slot);
	}
	return changed;
}
/* Context-After
 * /**
 *  * mesh_set_ht_prot_mode - set correct HT protection mode
 *  * @sdata: the (mesh) interface to handle
 *  *
 *  * Section 9.23.3.5 of IEEE 80211-2012 describes the protection rules for HT
 *  * mesh STA in a MBSS. Three HT protection modes are supported for now, non-HT
 *  * mixed mode, 20MHz-protection and no-protection mode. non-HT mixed mode is
 *  * selected if any non-HT peers are present in our MBSS.  20MHz-protection mode
 *  * is selected if all peers in our 20/40MHz MBSS support HT and at least one
 *  * HT20 peer is present. Otherwise no-protection mode is selected.
 *  *
 *  * Returns: BSS_CHANGED_HT or 0 for no change
 *  */
 * static u64 mesh_set_ht_prot_mode(struct ieee80211_sub_if_data *sdata)
 * {
 * 	struct ieee80211_local *local = sdata->local;
 * 	struct sta_info *sta;
 * 	u16 ht_opmode;
 * 	bool non_ht_sta = false, ht20_sta = false;
 */
