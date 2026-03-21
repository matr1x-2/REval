/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_020 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 11 */
/* NLOC: 40 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/dim.h>
 * #include <net/tc_act/tc_gact.h>
 * #include <linux/mlx5/fs.h>
 * #include <net/vxlan.h>
 * #include <net/geneve.h>
 * #include <linux/bpf.h>
 * #include <linux/debugfs.h>
 * #include <linux/if_bridge.h>
 * #include <linux/filter.h>
 * #include <net/netdev_lock.h>
 * #include <net/netdev_queues.h>
 * #include <net/netdev_rx_queue.h>
 * #include <net/page_pool/types.h>
 * #include <net/pkt_sched.h>
 * #include <net/xdp_sock_drv.h>
 * #include "eswitch.h"
 * #include "en.h"
 * #include "en/dim.h"
 * #include "en/txrx.h"
 * #include "en_tc.h"
 */
/* Context-Before
 * 		netdev_warn(netdev, "can't set XDP while HW-GRO/LRO is on, disable them first\n");
 * 		return -EINVAL;
 * 	}
 * 
 * 	if (!mlx5e_params_validate_xdp(netdev, mdev, params))
 * 		return -EINVAL;
 * 
 * 	return 0;
 * }
 * 
 * static void mlx5e_rq_replace_xdp_prog(struct mlx5e_rq *rq, struct bpf_prog *prog)
 * {
 * 	struct bpf_prog *old_prog;
 * 
 * 	old_prog = rcu_replace_pointer(rq->xdp_prog, prog,
 * 				       lockdep_is_held(&rq->priv->state_lock));
 * 	if (old_prog)
 * 		bpf_prog_put(old_prog);
 * }
 */
static int mlx5e_xdp_set(struct net_device *netdev, struct bpf_prog *prog)
{
	struct mlx5e_priv *priv = netdev_priv(netdev);
	struct mlx5e_params new_params;
	struct bpf_prog *old_prog;
	int err = 0;
	bool reset;
	int i;

	mutex_lock(&priv->state_lock);

	new_params = priv->channels.params;
	new_params.xdp_prog = prog;

	if (prog) {
		err = mlx5e_xdp_allowed(netdev, priv->mdev, &new_params);
		if (err)
			goto unlock;
	}

	/* no need for full reset when exchanging programs */
	reset = (!priv->channels.params.xdp_prog || !prog);

	old_prog = priv->channels.params.xdp_prog;

	err = mlx5e_safe_switch_params(priv, &new_params, NULL, NULL, reset);
	if (err)
		goto unlock;

	if (old_prog)
		bpf_prog_put(old_prog);

	if (!test_bit(MLX5E_STATE_OPENED, &priv->state) || reset)
		goto unlock;

	/* exchanging programs w/o reset, we update ref counts on behalf
	 * of the channels RQs here.
	 */
	bpf_prog_add(prog, priv->channels.num);
	for (i = 0; i < priv->channels.num; i++) {
		struct mlx5e_channel *c = priv->channels.c[i];

		mlx5e_rq_replace_xdp_prog(&c->rq, prog);
		if (test_bit(MLX5E_CHANNEL_STATE_XSK, c->state)) {
			bpf_prog_inc(prog);
			mlx5e_rq_replace_xdp_prog(&c->xskrq, prog);
		}
	}

unlock:
	mutex_unlock(&priv->state_lock);

	/* Need to fix some features. */
	if (!err)
		netdev_update_features(netdev);

	return err;
}
/* Context-After
 * static int mlx5e_xdp(struct net_device *dev, struct netdev_bpf *xdp)
 * {
 * 	switch (xdp->command) {
 * 	case XDP_SETUP_PROG:
 * 		return mlx5e_xdp_set(dev, xdp->prog);
 * 	case XDP_SETUP_XSK_POOL:
 * 		return mlx5e_xsk_setup_pool(dev, xdp->xsk.pool,
 * 					    xdp->xsk.queue_id);
 * 	default:
 * 		return -EINVAL;
 * 	}
 * }
 * 
 * #ifdef CONFIG_MLX5_ESWITCH
 * static int mlx5e_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
 * 				struct net_device *dev, u32 filter_mask,
 * 				int nlflags)
 * {
 * 	struct mlx5e_priv *priv = netdev_priv(dev);
 */
