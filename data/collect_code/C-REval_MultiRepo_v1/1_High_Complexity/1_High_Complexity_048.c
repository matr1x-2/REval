/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_048 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 29 */
/* NLOC: 148 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/bitfield.h>
 * #include "iris_hfi_gen2.h"
 * #include "iris_hfi_gen2_packet.h"
 */
/* Context-Before
 * static int iris_hfi_gen2_session_subscribe_mode(struct iris_inst *inst,
 * 						u32 cmd, u32 plane, u32 payload_type,
 * 						void *payload, u32 payload_size)
 * {
 * 	struct iris_inst_hfi_gen2 *inst_hfi_gen2 = to_iris_inst_hfi_gen2(inst);
 * 
 * 	iris_hfi_gen2_packet_session_command(inst,
 * 					     cmd,
 * 					     (HFI_HOST_FLAGS_RESPONSE_REQUIRED |
 * 					     HFI_HOST_FLAGS_INTR_REQUIRED),
 * 					     iris_hfi_gen2_get_port(inst, plane),
 * 					     inst->session_id,
 * 					     payload_type,
 * 					     payload,
 * 					     payload_size);
 * 
 * 	return iris_hfi_queue_cmd_write(inst->core, inst_hfi_gen2->packet,
 * 					inst_hfi_gen2->packet->size);
 * }
 */
static int iris_hfi_gen2_subscribe_change_param(struct iris_inst *inst, u32 plane)
{
	struct iris_inst_hfi_gen2 *inst_hfi_gen2 = to_iris_inst_hfi_gen2(inst);
	struct hfi_subscription_params subsc_params;
	u32 prop_type, payload_size, payload_type;
	struct iris_core *core = inst->core;
	const u32 *change_param = NULL;
	u32 change_param_size = 0;
	u32 payload[32] = {0};
	u32 hfi_port = 0, i;
	int ret;

	if (inst->domain == ENCODER)
		return 0;

	if ((V4L2_TYPE_IS_OUTPUT(plane) && inst_hfi_gen2->ipsc_properties_set) ||
	    (V4L2_TYPE_IS_CAPTURE(plane) && inst_hfi_gen2->opsc_properties_set)) {
		dev_dbg(core->dev, "%cPSC already set\n", V4L2_TYPE_IS_OUTPUT(plane) ? 'I' : 'O');
		return 0;
	}

	switch (inst->codec) {
	case V4L2_PIX_FMT_H264:
		change_param = core->iris_platform_data->dec_input_config_params_default;
		change_param_size =
			core->iris_platform_data->dec_input_config_params_default_size;
		break;
	case V4L2_PIX_FMT_HEVC:
		change_param = core->iris_platform_data->dec_input_config_params_hevc;
		change_param_size =
			core->iris_platform_data->dec_input_config_params_hevc_size;
		break;
	case V4L2_PIX_FMT_VP9:
		change_param = core->iris_platform_data->dec_input_config_params_vp9;
		change_param_size =
			core->iris_platform_data->dec_input_config_params_vp9_size;
		break;
	case V4L2_PIX_FMT_AV1:
		change_param = core->iris_platform_data->dec_input_config_params_av1;
		change_param_size =
			core->iris_platform_data->dec_input_config_params_av1_size;
		break;
	}

	payload[0] = HFI_MODE_PORT_SETTINGS_CHANGE;

	for (i = 0; i < change_param_size; i++)
		payload[i + 1] = change_param[i];

	ret = iris_hfi_gen2_session_subscribe_mode(inst,
						   HFI_CMD_SUBSCRIBE_MODE,
						   plane,
						   HFI_PAYLOAD_U32_ARRAY,
						   &payload[0],
						   ((change_param_size + 1) * sizeof(u32)));
	if (ret)
		return ret;

	if (V4L2_TYPE_IS_OUTPUT(plane)) {
		inst_hfi_gen2->ipsc_properties_set = true;
	} else {
		hfi_port = iris_hfi_gen2_get_port(inst, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
		memcpy(&inst_hfi_gen2->dst_subcr_params,
		       &inst_hfi_gen2->src_subcr_params,
		       sizeof(inst_hfi_gen2->src_subcr_params));
		subsc_params = inst_hfi_gen2->dst_subcr_params;
		for (i = 0; i < change_param_size; i++) {
			payload[0] = 0;
			payload[1] = 0;
			payload_size = 0;
			payload_type = 0;
			prop_type = change_param[i];
			switch (prop_type) {
			case HFI_PROP_BITSTREAM_RESOLUTION:
				payload[0] = subsc_params.bitstream_resolution;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_CROP_OFFSETS:
				payload[0] = subsc_params.crop_offsets[0];
				payload[1] = subsc_params.crop_offsets[1];
				payload_size = sizeof(u64);
				payload_type = HFI_PAYLOAD_64_PACKED;
				break;
			case HFI_PROP_CODED_FRAMES:
				payload[0] = subsc_params.coded_frames;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_LUMA_CHROMA_BIT_DEPTH:
				payload[0] = subsc_params.bit_depth;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_BUFFER_FW_MIN_OUTPUT_COUNT:
				payload[0] = subsc_params.fw_min_count;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_PIC_ORDER_CNT_TYPE:
				payload[0] = subsc_params.pic_order_cnt;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_SIGNAL_COLOR_INFO:
				payload[0] = subsc_params.color_info;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_PROFILE:
				payload[0] = subsc_params.profile;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_LEVEL:
				payload[0] = subsc_params.level;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_TIER:
				payload[0] = subsc_params.tier;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_AV1_FILM_GRAIN_PRESENT:
				payload[0] = subsc_params.film_grain;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			case HFI_PROP_AV1_SUPER_BLOCK_ENABLED:
				payload[0] = subsc_params.super_block;
				payload_size = sizeof(u32);
				payload_type = HFI_PAYLOAD_U32;
				break;
			default:
				prop_type = 0;
				ret = -EINVAL;
				break;
			}
			if (prop_type) {
				ret = iris_hfi_gen2_session_set_property(inst,
									 prop_type,
									 HFI_HOST_FLAGS_NONE,
									 hfi_port,
									 payload_type,
									 &payload,
									 payload_size);
				if (ret)
					return ret;
			}
		}
		inst_hfi_gen2->opsc_properties_set = true;
	}

	return 0;
}
/* Context-After
 * static int iris_hfi_gen2_subscribe_property(struct iris_inst *inst, u32 plane)
 * {
 * 	struct iris_core *core = inst->core;
 * 	u32 subscribe_prop_size = 0, i;
 * 	const u32 *subcribe_prop = NULL;
 * 	u32 payload[32] = {0};
 * 
 * 	payload[0] = HFI_MODE_PROPERTY;
 * 
 * 	if (inst->domain == ENCODER)
 * 		return 0;
 * 
 * 	if (V4L2_TYPE_IS_OUTPUT(plane)) {
 * 		subscribe_prop_size = core->iris_platform_data->dec_input_prop_size;
 * 		subcribe_prop = core->iris_platform_data->dec_input_prop;
 * 	} else {
 * 		switch (inst->codec) {
 * 		case V4L2_PIX_FMT_H264:
 * 			subcribe_prop = core->iris_platform_data->dec_output_prop_avc;
 */
