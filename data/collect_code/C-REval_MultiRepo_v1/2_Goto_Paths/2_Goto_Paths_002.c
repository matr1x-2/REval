/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_002 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 13 */
/* NLOC: 52 */
/* Subsystem: kernel */
/* Includes
 * #include <linux/cpu.h>
 * #include <linux/err.h>
 * #include <linux/hrtimer.h>
 * #include <linux/interrupt.h>
 * #include <linux/percpu.h>
 * #include <linux/profile.h>
 * #include <linux/sched.h>
 * #include <linux/smp.h>
 * #include <linux/module.h>
 * #include "tick-internal.h"
 */
/* Context-Before
 * 	return bc->bound_on == cpu ? -EBUSY : 0;
 * }
 * 
 * static void broadcast_shutdown_local(struct clock_event_device *bc,
 * 				     struct clock_event_device *dev)
 * {
 * 	/*
 * 	 * For hrtimer based broadcasting we cannot shutdown the cpu
 * 	 * local device if our own event is the first one to expire or
 * 	 * if we own the broadcast timer.
 * 	 */
 * 	if (bc->features & CLOCK_EVT_FEAT_HRTIMER) {
 * 		if (broadcast_needs_cpu(bc, smp_processor_id()))
 * 			return;
 * 		if (dev->next_event < bc->next_event)
 * 			return;
 * 	}
 * 	clockevents_switch_state(dev, CLOCK_EVT_STATE_SHUTDOWN);
 * }
 */
static int ___tick_broadcast_oneshot_control(enum tick_broadcast_state state,
					     struct tick_device *td,
					     int cpu)
{
	struct clock_event_device *bc, *dev = td->evtdev;
	int ret = 0;
	ktime_t now;

	raw_spin_lock(&tick_broadcast_lock);
	bc = tick_broadcast_device.evtdev;

	if (state == TICK_BROADCAST_ENTER) {
		/*
		 * If the current CPU owns the hrtimer broadcast
		 * mechanism, it cannot go deep idle and we do not add
		 * the CPU to the broadcast mask. We don't have to go
		 * through the EXIT path as the local timer is not
		 * shutdown.
		 */
		ret = broadcast_needs_cpu(bc, cpu);
		if (ret)
			goto out;

		/*
		 * If the broadcast device is in periodic mode, we
		 * return.
		 */
		if (tick_broadcast_device.mode == TICKDEV_MODE_PERIODIC) {
			/* If it is a hrtimer based broadcast, return busy */
			if (bc->features & CLOCK_EVT_FEAT_HRTIMER)
				ret = -EBUSY;
			goto out;
		}

		if (!cpumask_test_and_set_cpu(cpu, tick_broadcast_oneshot_mask)) {
			WARN_ON_ONCE(cpumask_test_cpu(cpu, tick_broadcast_pending_mask));

			/* Conditionally shut down the local timer. */
			broadcast_shutdown_local(bc, dev);

			/*
			 * We only reprogram the broadcast timer if we
			 * did not mark ourself in the force mask and
			 * if the cpu local event is earlier than the
			 * broadcast event. If the current CPU is in
			 * the force mask, then we are going to be
			 * woken by the IPI right away; we return
			 * busy, so the CPU does not try to go deep
			 * idle.
			 */
			if (cpumask_test_cpu(cpu, tick_broadcast_force_mask)) {
				ret = -EBUSY;
			} else if (dev->next_event < bc->next_event) {
				tick_broadcast_set_event(bc, cpu, dev->next_event);
				/*
				 * In case of hrtimer broadcasts the
				 * programming might have moved the
				 * timer to this cpu. If yes, remove
				 * us from the broadcast mask and
				 * return busy.
				 */
				ret = broadcast_needs_cpu(bc, cpu);
				if (ret) {
					cpumask_clear_cpu(cpu,
						tick_broadcast_oneshot_mask);
				}
			}
		}
	} else {
		if (cpumask_test_and_clear_cpu(cpu, tick_broadcast_oneshot_mask)) {
			clockevents_switch_state(dev, CLOCK_EVT_STATE_ONESHOT);
			/*
			 * The cpu which was handling the broadcast
			 * timer marked this cpu in the broadcast
			 * pending mask and fired the broadcast
			 * IPI. So we are going to handle the expired
			 * event anyway via the broadcast IPI
			 * handler. No need to reprogram the timer
			 * with an already expired event.
			 */
			if (cpumask_test_and_clear_cpu(cpu,
				       tick_broadcast_pending_mask))
				goto out;

			/*
			 * Bail out if there is no next event.
			 */
			if (dev->next_event == KTIME_MAX)
				goto out;
			/*
			 * If the pending bit is not set, then we are
			 * either the CPU handling the broadcast
			 * interrupt or we got woken by something else.
			 *
			 * We are no longer in the broadcast mask, so
			 * if the cpu local expiry time is already
			 * reached, we would reprogram the cpu local
			 * timer with an already expired event.
			 *
			 * This can lead to a ping-pong when we return
			 * to idle and therefore rearm the broadcast
			 * timer before the cpu local timer was able
			 * to fire. This happens because the forced
			 * reprogramming makes sure that the event
			 * will happen in the future and depending on
			 * the min_delta setting this might be far
			 * enough out that the ping-pong starts.
			 *
			 * If the cpu local next_event has expired
			 * then we know that the broadcast timer
			 * next_event has expired as well and
			 * broadcast is about to be handled. So we
			 * avoid reprogramming and enforce that the
			 * broadcast handler, which did not run yet,
			 * will invoke the cpu local handler.
			 *
			 * We cannot call the handler directly from
			 * here, because we might be in a NOHZ phase
			 * and we did not go through the irq_enter()
			 * nohz fixups.
			 */
			now = ktime_get();
			if (dev->next_event <= now) {
				cpumask_set_cpu(cpu, tick_broadcast_force_mask);
				goto out;
			}
			/*
			 * We got woken by something else. Reprogram
			 * the cpu local timer device.
			 */
			tick_program_event(dev->next_event, 1);
		}
	}
out:
	raw_spin_unlock(&tick_broadcast_lock);
	return ret;
}
/* Context-After
 * static int tick_oneshot_wakeup_control(enum tick_broadcast_state state,
 * 				       struct tick_device *td,
 * 				       int cpu)
 * {
 * 	struct clock_event_device *dev, *wd;
 * 
 * 	dev = td->evtdev;
 * 	if (td->mode != TICKDEV_MODE_ONESHOT)
 * 		return -EINVAL;
 * 
 * 	wd = tick_get_oneshot_wakeup_device(cpu);
 * 	if (!wd)
 * 		return -ENODEV;
 * 
 * 	switch (state) {
 * 	case TICK_BROADCAST_ENTER:
 * 		clockevents_switch_state(dev, CLOCK_EVT_STATE_ONESHOT_STOPPED);
 * 		clockevents_switch_state(wd, CLOCK_EVT_STATE_ONESHOT);
 * 		clockevents_program_event(wd, dev->next_event, 1);
 */
