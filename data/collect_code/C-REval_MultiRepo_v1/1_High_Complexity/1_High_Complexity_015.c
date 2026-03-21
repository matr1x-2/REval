/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_015 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 24 */
/* NLOC: 115 */
/* Subsystem: net */
/* Includes
 * #include <linux/slab.h>
 * #include <linux/etherdevice.h>
 * #include <linux/unaligned.h>
 * #include "wme.h"
 * #include "mesh.h"
 */
/* Context-Before
 * 			mpath->exp_time = time_after(mpath->exp_time, exp_time)
 * 					  ?  mpath->exp_time : exp_time;
 * 			mpath->hop_count = 1;
 * 			mesh_path_activate(mpath);
 * 			spin_unlock_bh(&mpath->state_lock);
 * 			if (flush_mpath)
 * 				mesh_fast_tx_flush_mpath(mpath);
 * 			ewma_mesh_fail_avg_init(&sta->mesh->fail_avg);
 * 			/* init it at a low value - 0 start is tricky */
 * 			ewma_mesh_fail_avg_add(&sta->mesh->fail_avg, 1);
 * 			mesh_path_tx_pending(mpath);
 * 		} else
 * 			spin_unlock_bh(&mpath->state_lock);
 * 	}
 * 
 * 	rcu_read_unlock();
 * 
 * 	return process ? new_metric : 0;
 * }
 */
static void hwmp_preq_frame_process(struct ieee80211_sub_if_data *sdata,
				    struct ieee80211_mgmt *mgmt,
				    const u8 *preq_elem, u32 orig_metric)
{
	struct ieee80211_if_mesh *ifmsh = &sdata->u.mesh;
	struct mesh_path *mpath = NULL;
	const u8 *target_addr, *orig_addr;
	const u8 *da;
	u8 target_flags, ttl, flags;
	u32 orig_sn, target_sn, lifetime, target_metric = 0;
	bool reply = false;
	bool forward = true;
	bool root_is_gate;

	/* Update target SN, if present */
	target_addr = PREQ_IE_TARGET_ADDR(preq_elem);
	orig_addr = PREQ_IE_ORIG_ADDR(preq_elem);
	target_sn = PREQ_IE_TARGET_SN(preq_elem);
	orig_sn = PREQ_IE_ORIG_SN(preq_elem);
	target_flags = PREQ_IE_TARGET_F(preq_elem);
	/* Proactive PREQ gate announcements */
	flags = PREQ_IE_FLAGS(preq_elem);
	root_is_gate = !!(flags & RANN_FLAG_IS_GATE);

	mhwmp_dbg(sdata, "received PREQ from %pM\n", orig_addr);

	if (ether_addr_equal(target_addr, sdata->vif.addr)) {
		mhwmp_dbg(sdata, "PREQ is for us\n");
		forward = false;
		reply = true;
		target_metric = 0;

		if (SN_GT(target_sn, ifmsh->sn))
			ifmsh->sn = target_sn;

		if (time_after(jiffies, ifmsh->last_sn_update +
					net_traversal_jiffies(sdata)) ||
		    time_before(jiffies, ifmsh->last_sn_update)) {
			++ifmsh->sn;
			ifmsh->last_sn_update = jiffies;
		}
		target_sn = ifmsh->sn;
	} else if (is_broadcast_ether_addr(target_addr) &&
		   (target_flags & IEEE80211_PREQ_TO_FLAG)) {
		rcu_read_lock();
		mpath = mesh_path_lookup(sdata, orig_addr);
		if (mpath) {
			if (flags & IEEE80211_PREQ_PROACTIVE_PREP_FLAG) {
				reply = true;
				target_addr = sdata->vif.addr;
				target_sn = ++ifmsh->sn;
				target_metric = 0;
				ifmsh->last_sn_update = jiffies;
			}
			if (root_is_gate)
				mesh_path_add_gate(mpath);
		}
		rcu_read_unlock();
	} else if (ifmsh->mshcfg.dot11MeshForwarding) {
		rcu_read_lock();
		mpath = mesh_path_lookup(sdata, target_addr);
		if (mpath) {
			if ((!(mpath->flags & MESH_PATH_SN_VALID)) ||
					SN_LT(mpath->sn, target_sn)) {
				mpath->sn = target_sn;
				mpath->flags |= MESH_PATH_SN_VALID;
			} else if ((!(target_flags & IEEE80211_PREQ_TO_FLAG)) &&
					(mpath->flags & MESH_PATH_ACTIVE)) {
				reply = true;
				target_metric = mpath->metric;
				target_sn = mpath->sn;
				/* Case E2 of sec 13.10.9.3 IEEE 802.11-2012*/
				target_flags |= IEEE80211_PREQ_TO_FLAG;
			}
		}
		rcu_read_unlock();
	} else {
		forward = false;
	}

	if (reply) {
		lifetime = PREQ_IE_LIFETIME(preq_elem);
		ttl = ifmsh->mshcfg.element_ttl;
		if (ttl != 0) {
			mhwmp_dbg(sdata, "replying to the PREQ\n");
			mesh_path_sel_frame_tx(MPATH_PREP, 0, orig_addr,
					       orig_sn, 0, target_addr,
					       target_sn, mgmt->sa, 0, ttl,
					       lifetime, target_metric, 0,
					       sdata);
		} else {
			ifmsh->mshstats.dropped_frames_ttl++;
		}
	}

	if (forward) {
		u32 preq_id;
		u8 hopcount;

		ttl = PREQ_IE_TTL(preq_elem);
		lifetime = PREQ_IE_LIFETIME(preq_elem);
		if (ttl <= 1) {
			ifmsh->mshstats.dropped_frames_ttl++;
			return;
		}
		mhwmp_dbg(sdata, "forwarding the PREQ from %pM\n", orig_addr);
		--ttl;
		preq_id = PREQ_IE_PREQ_ID(preq_elem);
		hopcount = PREQ_IE_HOPCOUNT(preq_elem) + 1;
		da = (mpath && mpath->is_root) ?
			mpath->rann_snd_addr : broadcast_addr;

		if (flags & IEEE80211_PREQ_PROACTIVE_PREP_FLAG) {
			target_addr = PREQ_IE_TARGET_ADDR(preq_elem);
			target_sn = PREQ_IE_TARGET_SN(preq_elem);
		}

		mesh_path_sel_frame_tx(MPATH_PREQ, flags, orig_addr,
				       orig_sn, target_flags, target_addr,
				       target_sn, da, hopcount, ttl, lifetime,
				       orig_metric, preq_id, sdata);
		if (!is_multicast_ether_addr(da))
			ifmsh->mshstats.fwded_unicast++;
		else
			ifmsh->mshstats.fwded_mcast++;
		ifmsh->mshstats.fwded_frames++;
	}
}
/* Context-After
 * static inline struct sta_info *
 * next_hop_deref_protected(struct mesh_path *mpath)
 * {
 * 	return rcu_dereference_protected(mpath->next_hop,
 * 					 lockdep_is_held(&mpath->state_lock));
 * }
 * 
 * 
 * static void hwmp_prep_frame_process(struct ieee80211_sub_if_data *sdata,
 * 				    struct ieee80211_mgmt *mgmt,
 * 				    const u8 *prep_elem, u32 metric)
 * {
 * 	struct ieee80211_if_mesh *ifmsh = &sdata->u.mesh;
 * 	struct mesh_path *mpath;
 * 	const u8 *target_addr, *orig_addr;
 * 	u8 ttl, hopcount, flags;
 * 	u8 next_hop[ETH_ALEN];
 * 	u32 target_sn, orig_sn, lifetime;
 */
