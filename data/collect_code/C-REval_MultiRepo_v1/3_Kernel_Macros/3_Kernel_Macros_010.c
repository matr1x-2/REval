/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_010 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 27 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/devcoredump.h>
 * #include <linux/etherdevice.h>
 * #include <linux/timekeeping.h>
 * #include "mt7921.h"
 * #include "../dma.h"
 * #include "../mt76_connac2_mac.h"
 * #include "mcu.h"
 */
/* Context-Before
 * 		}
 * 	} else {
 * 		status->flag |= RX_FLAG_8023;
 * 	}
 * 
 * 	mt792x_mac_assoc_rssi(dev, skb);
 * 
 * 	if (rxv && mode >= MT_PHY_TYPE_HE_SU && !(status->flag & RX_FLAG_8023))
 * 		mt76_connac2_mac_decode_he_radiotap(&dev->mt76, skb, rxv, mode);
 * 
 * 	if (!status->wcid || !ieee80211_is_data_qos(fc))
 * 		return 0;
 * 
 * 	status->aggr = unicast && !ieee80211_is_qos_nullfunc(fc);
 * 	status->seqno = IEEE80211_SEQ_TO_SN(seq_ctrl);
 * 	status->qos_ctl = qos_ctl;
 * 
 * 	return 0;
 * }
 */
void mt7921_mac_add_txs(struct mt792x_dev *dev, void *data)
{
	struct mt792x_link_sta *mlink;
	struct mt76_wcid *wcid;
	__le32 *txs_data = data;
	u16 wcidx;
	u8 pid;

	if (le32_get_bits(txs_data[0], MT_TXS0_TXS_FORMAT) > 1)
		return;

	wcidx = le32_get_bits(txs_data[2], MT_TXS2_WCID);
	pid = le32_get_bits(txs_data[3], MT_TXS3_PID);

	if (pid < MT_PACKET_ID_FIRST)
		return;

	if (wcidx >= MT792x_WTBL_SIZE)
		return;

	rcu_read_lock();

	wcid = mt76_wcid_ptr(dev, wcidx);
	if (!wcid)
		goto out;

	mlink = container_of(wcid, struct mt792x_link_sta, wcid);

	mt76_connac2_mac_add_txs_skb(&dev->mt76, wcid, pid, txs_data);
	if (!wcid->sta)
		goto out;

	mt76_wcid_add_poll(&dev->mt76, &mlink->wcid);

out:
	rcu_read_unlock();
}
/* Context-After
 * static void mt7921_mac_tx_free(struct mt792x_dev *dev, void *data, int len)
 * {
 * 	struct mt76_connac_tx_free *free = data;
 * 	__le32 *tx_info = (__le32 *)(data + sizeof(*free));
 * 	struct mt76_dev *mdev = &dev->mt76;
 * 	struct mt76_txwi_cache *txwi;
 * 	struct ieee80211_sta *sta = NULL;
 * 	struct mt76_wcid *wcid = NULL;
 * 	struct sk_buff *skb, *tmp;
 * 	void *end = data + len;
 * 	LIST_HEAD(free_list);
 * 	bool wake = false;
 * 	u8 i, count;
 * 
 * 	/* clean DMA queues and unmap buffers first */
 * 	mt76_queue_tx_cleanup(dev, dev->mphy.q_tx[MT_TXQ_PSD], false);
 * 	mt76_queue_tx_cleanup(dev, dev->mphy.q_tx[MT_TXQ_BE], false);
 * 
 * 	count = le16_get_bits(free->ctrl, MT_TX_FREE_MSDU_CNT);
 */
