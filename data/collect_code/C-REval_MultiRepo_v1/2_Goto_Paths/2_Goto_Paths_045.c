/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_045 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 28 */
/* NLOC: 77 */
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
 * 			 * restarted.
 * 			 */
 * 			event = CLS_ACPT;
 * 		else if (sta->mesh->plid != plid)
 * 			event = CLS_IGNR;
 * 		else if (ie_len == 8 && sta->mesh->llid != llid)
 * 			event = CLS_IGNR;
 * 		else
 * 			event = CLS_ACPT;
 * 		break;
 * 	default:
 * 		mpl_dbg(sdata, "Mesh plink: unknown frame subtype\n");
 * 		break;
 * 	}
 * 
 * out:
 * 	return event;
 * }
 * 
 * static void
 */
mesh_process_plink_frame(struct ieee80211_sub_if_data *sdata,
			 struct ieee80211_mgmt *mgmt,
			 struct ieee802_11_elems *elems,
			 struct ieee80211_rx_status *rx_status)
{

	struct sta_info *sta;
	enum plink_event event;
	enum ieee80211_self_protected_actioncode ftype;
	u64 changed = 0;
	u8 ie_len = elems->peering_len;
	u16 plid, llid = 0;

	if (!elems->peering) {
		mpl_dbg(sdata,
			"Mesh plink: missing necessary peer link ie\n");
		return;
	}

	if (elems->rsn_len &&
	    sdata->u.mesh.security == IEEE80211_MESH_SEC_NONE) {
		mpl_dbg(sdata,
			"Mesh plink: can't establish link with secure peer\n");
		return;
	}

	ftype = mgmt->u.action.u.self_prot.action_code;
	if ((ftype == WLAN_SP_MESH_PEERING_OPEN && ie_len != 4) ||
	    (ftype == WLAN_SP_MESH_PEERING_CONFIRM && ie_len != 6) ||
	    (ftype == WLAN_SP_MESH_PEERING_CLOSE && ie_len != 6
							&& ie_len != 8)) {
		mpl_dbg(sdata,
			"Mesh plink: incorrect plink ie length %d %d\n",
			ftype, ie_len);
		return;
	}

	if (ftype != WLAN_SP_MESH_PEERING_CLOSE &&
	    (!elems->mesh_id || !elems->mesh_config)) {
		mpl_dbg(sdata, "Mesh plink: missing necessary ie\n");
		return;
	}
	/* Note the lines below are correct, the llid in the frame is the plid
	 * from the point of view of this host.
	 */
	plid = get_unaligned_le16(PLINK_GET_LLID(elems->peering));
	if (ftype == WLAN_SP_MESH_PEERING_CONFIRM ||
	    (ftype == WLAN_SP_MESH_PEERING_CLOSE && ie_len == 8))
		llid = get_unaligned_le16(PLINK_GET_PLID(elems->peering));

	/* WARNING: Only for sta pointer, is dropped & re-acquired */
	rcu_read_lock();

	sta = sta_info_get(sdata, mgmt->sa);

	if (ftype == WLAN_SP_MESH_PEERING_OPEN &&
	    !rssi_threshold_check(sdata, sta)) {
		mpl_dbg(sdata, "Mesh plink: %pM does not meet rssi threshold\n",
			mgmt->sa);
		goto unlock_rcu;
	}

	/* Now we will figure out the appropriate event... */
	event = mesh_plink_get_event(sdata, sta, elems, ftype, llid, plid);

	if (event == OPN_ACPT) {
		rcu_read_unlock();
		/* allocate sta entry if necessary and update info */
		sta = mesh_sta_info_get(sdata, mgmt->sa, elems, rx_status);
		if (!sta) {
			mpl_dbg(sdata, "Mesh plink: failed to init peer!\n");
			goto unlock_rcu;
		}
		sta->mesh->plid = plid;
	} else if (!sta && event == OPN_RJCT) {
		mesh_plink_frame_tx(sdata, NULL, WLAN_SP_MESH_PEERING_CLOSE,
				    mgmt->sa, 0, plid,
				    WLAN_REASON_MESH_CONFIG);
		goto unlock_rcu;
	} else if (!sta || event == PLINK_UNDEFINED) {
		/* something went wrong */
		goto unlock_rcu;
	}

	if (event == CNF_ACPT) {
		/* 802.11-2012 13.3.7.2 - update plid on CNF if not set */
		if (!sta->mesh->plid)
			sta->mesh->plid = plid;

		sta->mesh->aid = get_unaligned_le16(PLINK_CNF_AID(mgmt));
	}

	changed |= mesh_plink_fsm(sdata, sta, event);

unlock_rcu:
	rcu_read_unlock();

	if (changed)
		ieee80211_mbss_info_change_notify(sdata, changed);
}
/* Context-After
 * void mesh_rx_plink_frame(struct ieee80211_sub_if_data *sdata,
 * 			 struct ieee80211_mgmt *mgmt, size_t len,
 * 			 struct ieee80211_rx_status *rx_status)
 * {
 * 	struct ieee802_11_elems *elems;
 * 	size_t baselen;
 * 	u8 *baseaddr;
 * 
 * 	/* need action_code, aux */
 * 	if (len < IEEE80211_MIN_ACTION_SIZE + 3)
 * 		return;
 * 
 * 	if (sdata->u.mesh.user_mpm)
 * 		/* userspace must register for these */
 * 		return;
 * 
 * 	if (is_multicast_ether_addr(mgmt->da)) {
 * 		mpl_dbg(sdata,
 * 			"Mesh plink: ignore frame from multicast address\n");
 */
