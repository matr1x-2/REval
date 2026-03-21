/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_030 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 39 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/etherdevice.h>
 * #include <linux/netdevice.h>
 * #include <linux/ieee80211.h>
 * #include <linux/rtnetlink.h>
 * #include <linux/module.h>
 * #include <linux/moduleparam.h>
 * #include <linux/mei_cl_bus.h>
 * #include <linux/rcupdate.h>
 * #include <linux/debugfs.h>
 * #include <linux/skbuff.h>
 * #include <linux/wait.h>
 * #include <linux/slab.h>
 * #include <linux/mm.h>
 * #include <net/cfg80211.h>
 * #include "internal.h"
 * #include "iwl-mei.h"
 * #include "trace.h"
 * #include "trace-data.h"
 * #include "sap.h"
 */
/* Context-Before
 * 			return ret;
 * 
 * 		ret = wait_event_timeout(mei->pldr_wq, mei->pldr_active, HZ / 2);
 * 		if (ret)
 * 			break;
 * 
 * 		/* Take the mutex for the next iteration */
 * 		mutex_lock(&iwl_mei_mutex);
 * 	}
 * 
 * 	if (ret)
 * 		return 0;
 * 
 * 	ret = -ETIMEDOUT;
 * out:
 * 	mutex_unlock(&iwl_mei_mutex);
 * 	return ret;
 * }
 * EXPORT_SYMBOL_GPL(iwl_mei_pldr_req);
 */
int iwl_mei_get_ownership(void)
{
	struct iwl_mei *mei;
	int ret;

	mutex_lock(&iwl_mei_mutex);

	/* In case we didn't have a bind */
	if (!iwl_mei_is_connected()) {
		ret = 0;
		goto out;
	}

	mei = mei_cldev_get_drvdata(iwl_mei_global_cldev);

	if (!mei) {
		ret = -ENODEV;
		goto out;
	}

	if (!mei->amt_enabled) {
		ret = 0;
		goto out;
	}

	if (mei->got_ownership) {
		ret = 0;
		goto out;
	}

	ret = iwl_mei_send_sap_msg(mei->cldev,
				   SAP_MSG_NOTIF_HOST_ASKS_FOR_NIC_OWNERSHIP);
	if (ret)
		goto out;

	mutex_unlock(&iwl_mei_mutex);

	ret = wait_event_timeout(mei->get_ownership_wq,
				 mei->got_ownership, HZ / 2);
	if (!ret) {
		schedule_delayed_work(&mei->ownership_dwork,
				      MEI_OWNERSHIP_RETAKE_TIMEOUT_MS);
		return -ETIMEDOUT;
	}

	return 0;
out:
	mutex_unlock(&iwl_mei_mutex);
	return ret;
}
/* Context-After
 * EXPORT_SYMBOL_GPL(iwl_mei_get_ownership);
 * 
 * void iwl_mei_alive_notif(bool success)
 * {
 * 	struct iwl_mei *mei;
 * 	struct iwl_sap_pldr_end_data msg = {
 * 		.hdr.type = cpu_to_le16(SAP_MSG_NOTIF_PLDR_END),
 * 		.hdr.len = cpu_to_le16(sizeof(msg) - sizeof(msg.hdr)),
 * 		.status = success ? cpu_to_le32(SAP_PLDR_STATUS_SUCCESS) :
 * 			cpu_to_le32(SAP_PLDR_STATUS_FAILURE),
 * 	};
 * 
 * 	mutex_lock(&iwl_mei_mutex);
 * 
 * 	if (!iwl_mei_is_connected())
 * 		goto out;
 * 
 * 	mei = mei_cldev_get_drvdata(iwl_mei_global_cldev);
 * 	if (!mei || !mei->pldr_active)
 * 		goto out;
 */
