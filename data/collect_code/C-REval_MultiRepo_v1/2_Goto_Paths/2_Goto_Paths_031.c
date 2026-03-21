/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_031 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 60 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/clk.h>
 * #include <linux/delay.h>
 * #include <linux/err.h>
 * #include <linux/firmware.h>
 * #include <linux/io.h>
 * #include <linux/iopoll.h>
 * #include <linux/irq.h>
 * #include <linux/media-bus-format.h>
 * #include <linux/module.h>
 * #include <linux/of.h>
 * #include <linux/phy/phy.h>
 * #include <linux/phy/phy-dp.h>
 * #include <linux/platform_device.h>
 * #include <linux/slab.h>
 * #include <linux/wait.h>
 * #include <drm/display/drm_dp_helper.h>
 * #include <drm/display/drm_hdcp_helper.h>
 * #include <drm/drm_atomic.h>
 * #include <drm/drm_atomic_helper.h>
 * #include <drm/drm_atomic_state_helper.h>
 */
/* Context-Before
 * 		adjust = drm_dp_get_adjust_request_pre_emphasis(after_cr, i) >>
 * 		      DP_TRAIN_PRE_EMPHASIS_SHIFT;
 * 		req_pre[i] = min(adjust, max_pre);
 * 
 * 		same_pre = (before_cr[i] & DP_TRAIN_PRE_EMPHASIS_MASK) ==
 * 			   req_pre[i] << DP_TRAIN_PRE_EMPHASIS_SHIFT;
 * 		same_volt = (before_cr[i] & DP_TRAIN_VOLTAGE_SWING_MASK) ==
 * 			    req_volt[i];
 * 		if (same_pre && same_volt)
 * 			*same_before_adjust = true;
 * 
 * 		/* 3.1.5.2 in DP Standard v1.4. Table 3-1 */
 * 		if (!*cr_done && req_volt[i] + req_pre[i] >= 3) {
 * 			*max_swing_reached = true;
 * 			return;
 * 		}
 * 	}
 * }
 */
static bool cdns_mhdp_link_training_cr(struct cdns_mhdp_device *mhdp)
{
	u8 lanes_data[CDNS_DP_MAX_NUM_LANES],
	fail_counter_short = 0, fail_counter_cr_long = 0;
	u8 link_status[DP_LINK_STATUS_SIZE];
	bool cr_done;
	union phy_configure_opts phy_cfg;
	int ret;

	dev_dbg(mhdp->dev, "Starting CR phase\n");

	ret = cdns_mhdp_link_training_init(mhdp);
	if (ret)
		goto err;

	drm_dp_dpcd_read_link_status(&mhdp->aux, link_status);

	do {
		u8 requested_adjust_volt_swing[CDNS_DP_MAX_NUM_LANES] = {};
		u8 requested_adjust_pre_emphasis[CDNS_DP_MAX_NUM_LANES] = {};
		bool same_before_adjust, max_swing_reached;

		cdns_mhdp_get_adjust_train(mhdp, link_status, lanes_data,
					   &phy_cfg);
		phy_cfg.dp.lanes = mhdp->link.num_lanes;
		phy_cfg.dp.ssc = cdns_mhdp_get_ssc_supported(mhdp);
		phy_cfg.dp.set_lanes = false;
		phy_cfg.dp.set_rate = false;
		phy_cfg.dp.set_voltages = true;
		ret = phy_configure(mhdp->phy,  &phy_cfg);
		if (ret) {
			dev_err(mhdp->dev, "%s: phy_configure() failed: %d\n",
				__func__, ret);
			goto err;
		}

		cdns_mhdp_adjust_lt(mhdp, mhdp->link.num_lanes, 100,
				    lanes_data, link_status);

		cdns_mhdp_validate_cr(mhdp, &cr_done, &same_before_adjust,
				      &max_swing_reached, lanes_data,
				      link_status,
				      requested_adjust_volt_swing,
				      requested_adjust_pre_emphasis);

		if (max_swing_reached) {
			dev_err(mhdp->dev, "CR: max swing reached\n");
			goto err;
		}

		if (cr_done) {
			cdns_mhdp_print_lt_status("CR phase ok", mhdp,
						  &phy_cfg);
			return true;
		}

		/* Not all CR_DONE bits set */
		fail_counter_cr_long++;

		if (same_before_adjust) {
			fail_counter_short++;
			continue;
		}

		fail_counter_short = 0;
		/*
		 * Voltage swing/pre-emphasis adjust requested
		 * during CR phase
		 */
		cdns_mhdp_adjust_requested_cr(mhdp, link_status,
					      requested_adjust_volt_swing,
					      requested_adjust_pre_emphasis);
	} while (fail_counter_short < 5 && fail_counter_cr_long < 10);

err:
	cdns_mhdp_print_lt_status("CR phase failed", mhdp, &phy_cfg);

	return false;
}
/* Context-After
 * static void cdns_mhdp_lower_link_rate(struct cdns_mhdp_link *link)
 * {
 * 	switch (drm_dp_link_rate_to_bw_code(link->rate)) {
 * 	case DP_LINK_BW_2_7:
 * 		link->rate = drm_dp_bw_code_to_link_rate(DP_LINK_BW_1_62);
 * 		break;
 * 	case DP_LINK_BW_5_4:
 * 		link->rate = drm_dp_bw_code_to_link_rate(DP_LINK_BW_2_7);
 * 		break;
 * 	case DP_LINK_BW_8_1:
 * 		link->rate = drm_dp_bw_code_to_link_rate(DP_LINK_BW_5_4);
 * 		break;
 * 	}
 * }
 * 
 * static int cdns_mhdp_link_training(struct cdns_mhdp_device *mhdp,
 * 				   unsigned int training_interval)
 * {
 * 	u32 reg32;
 */
