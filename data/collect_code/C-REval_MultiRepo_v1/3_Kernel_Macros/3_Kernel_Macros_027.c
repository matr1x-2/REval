/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_027 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 34 */
/* NLOC: 127 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/bitfield.h>
 * #include <media/v4l2-mem2mem.h>
 * #include "iris_hfi_gen1.h"
 * #include "iris_hfi_gen1_defines.h"
 * #include "iris_instance.h"
 * #include "iris_vdec.h"
 * #include "iris_vpu_buffer.h"
 */
/* Context-Before
 * 		iris_inst_change_state(inst, IRIS_INST_ERROR);
 * 	}
 * 
 * 	if (!(buf->attr & BUF_ATTR_QUEUED))
 * 		return;
 * 
 * 	buf->attr &= ~BUF_ATTR_QUEUED;
 * 
 * 	if (!(buf->attr & BUF_ATTR_BUFFER_DONE)) {
 * 		buf->attr |= BUF_ATTR_BUFFER_DONE;
 * 		iris_vb2_buffer_done(inst, buf);
 * 	}
 * 
 * 	return;
 * 
 * error:
 * 	iris_inst_change_state(inst, IRIS_INST_ERROR);
 * 	dev_err(inst->core->dev, "error in etb done\n");
 * }
 */
static void iris_hfi_gen1_session_ftb_done(struct iris_inst *inst, void *packet)
{
	struct hfi_msg_session_fbd_uncompressed_plane0_pkt *uncom_pkt = packet;
	struct hfi_msg_session_fbd_compressed_pkt *com_pkt = packet;
	struct v4l2_m2m_ctx *m2m_ctx = inst->m2m_ctx;
	struct v4l2_m2m_buffer *m2m_buffer, *n;
	struct hfi_session_flush_pkt flush_pkt;
	u32 timestamp_hi;
	u32 timestamp_lo;
	struct iris_core *core = inst->core;
	u32 filled_len;
	u32 pic_type;
	u32 output_tag;
	struct iris_buffer *buf, *iter;
	struct iris_buffers *buffers;
	u32 hfi_flags;
	u32 offset;
	u64 timestamp_us = 0;
	bool found = false;
	u32 flags = 0;

	if (inst->domain == DECODER) {
		timestamp_hi = uncom_pkt->time_stamp_hi;
		timestamp_lo = uncom_pkt->time_stamp_lo;
		filled_len = uncom_pkt->filled_len;
		pic_type = uncom_pkt->picture_type;
		output_tag = uncom_pkt->output_tag;
		hfi_flags = uncom_pkt->flags;
		offset = uncom_pkt->offset;
	} else {
		timestamp_hi = com_pkt->time_stamp_hi;
		timestamp_lo = com_pkt->time_stamp_lo;
		filled_len = com_pkt->filled_len;
		pic_type = com_pkt->picture_type;
		output_tag = com_pkt->output_tag;
		hfi_flags = com_pkt->flags;
		offset = com_pkt->offset;
	}

	if ((hfi_flags & HFI_BUFFERFLAG_EOS) && !filled_len) {
		reinit_completion(&inst->flush_completion);

		flush_pkt.shdr.hdr.size = sizeof(struct hfi_session_flush_pkt);
		flush_pkt.shdr.hdr.pkt_type = HFI_CMD_SESSION_FLUSH;
		flush_pkt.shdr.session_id = inst->session_id;
		flush_pkt.flush_type = HFI_FLUSH_OUTPUT;
		if (!iris_hfi_queue_cmd_write(core, &flush_pkt, flush_pkt.shdr.hdr.size))
			inst->flush_responses_pending++;

		iris_inst_sub_state_change_drain_last(inst);
	}

	if (iris_split_mode_enabled(inst) && inst->domain == DECODER &&
	    uncom_pkt->stream_id == 0) {
		buffers = &inst->buffers[BUF_DPB];
		if (!buffers)
			goto error;

		found = false;
		list_for_each_entry(iter, &buffers->list, list) {
			if (!(iter->attr & BUF_ATTR_QUEUED))
				continue;

			found = (iter->index == output_tag &&
				iter->data_offset == offset);

			if (found) {
				buf = iter;
				break;
			}
		}
	} else {
		v4l2_m2m_for_each_dst_buf_safe(m2m_ctx, m2m_buffer, n) {
			buf = to_iris_buffer(&m2m_buffer->vb);
			if (!(buf->attr & BUF_ATTR_QUEUED))
				continue;

			found = (buf->index == output_tag &&
				 buf->data_offset == offset);

			if (found)
				break;
		}
	}
	if (!found)
		goto error;

	buf->data_offset = offset;
	buf->data_size = filled_len;

	if (filled_len) {
		timestamp_us = timestamp_hi;
		timestamp_us = (timestamp_us << 32) | timestamp_lo;
	} else {
		if (inst->domain == DECODER && uncom_pkt->stream_id == 1 &&
		    !inst->last_buffer_dequeued) {
			if (iris_drc_pending(inst) || iris_drain_pending(inst)) {
				flags |= V4L2_BUF_FLAG_LAST;
				inst->last_buffer_dequeued = true;
			}
		} else if (inst->domain == ENCODER) {
			if (!inst->last_buffer_dequeued && iris_drain_pending(inst)) {
				flags |= V4L2_BUF_FLAG_LAST;
				inst->last_buffer_dequeued = true;
			}
		}
	}
	buf->timestamp = timestamp_us;

	switch (pic_type) {
	case HFI_GEN1_PICTURE_IDR:
	case HFI_GEN1_PICTURE_I:
		flags |= V4L2_BUF_FLAG_KEYFRAME;
		break;
	case HFI_GEN1_PICTURE_P:
		flags |= V4L2_BUF_FLAG_PFRAME;
		break;
	case HFI_GEN1_PICTURE_B:
		flags |= V4L2_BUF_FLAG_BFRAME;
		break;
	case HFI_FRAME_NOTCODED:
	case HFI_UNUSED_PICT:
	case HFI_FRAME_YUV:
	default:
		break;
	}

	buf->attr &= ~BUF_ATTR_QUEUED;
	buf->attr |= BUF_ATTR_DEQUEUED;
	buf->attr |= BUF_ATTR_BUFFER_DONE;

	if (hfi_flags & HFI_BUFFERFLAG_DATACORRUPT)
		flags |= V4L2_BUF_FLAG_ERROR;

	if (hfi_flags & HFI_BUFFERFLAG_DROP_FRAME)
		flags |= V4L2_BUF_FLAG_ERROR;

	buf->flags |= flags;

	iris_vb2_buffer_done(inst, buf);

	return;

error:
	iris_inst_change_state(inst, IRIS_INST_ERROR);
	dev_err(core->dev, "error in ftb done\n");
}
/* Context-After
 * struct iris_hfi_gen1_response_pkt_info {
 * 	u32 pkt;
 * 	u32 pkt_sz;
 * };
 * 
 * static const struct iris_hfi_gen1_response_pkt_info pkt_infos[] = {
 * 	{
 * 	 .pkt = HFI_MSG_EVENT_NOTIFY,
 * 	 .pkt_sz = sizeof(struct hfi_msg_event_notify_pkt),
 * 	},
 * 	{
 * 	 .pkt = HFI_MSG_SYS_INIT,
 * 	 .pkt_sz = sizeof(struct hfi_msg_sys_init_done_pkt),
 * 	},
 * 	{
 * 	 .pkt = HFI_MSG_SYS_PROPERTY_INFO,
 * 	 .pkt_sz = sizeof(struct hfi_msg_sys_property_info_pkt),
 * 	},
 * 	{
 */
