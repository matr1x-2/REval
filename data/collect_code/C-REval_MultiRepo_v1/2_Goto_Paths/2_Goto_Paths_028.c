/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_028 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 37 */
/* Subsystem: kernel */
/* Includes
 * #include <linux/compiler.h>
 * #include <linux/completion.h>
 * #include <linux/cpu.h>
 * #include <linux/init.h>
 * #include <linux/kthread.h>
 * #include <linux/export.h>
 * #include <linux/percpu.h>
 * #include <linux/sched.h>
 * #include <linux/stop_machine.h>
 * #include <linux/interrupt.h>
 * #include <linux/kallsyms.h>
 * #include <linux/smpboot.h>
 * #include <linux/atomic.h>
 * #include <linux/nmi.h>
 * #include <linux/sched/wake_q.h>
 */
/* Context-Before
 * 			default:
 * 				break;
 * 			}
 * 			ack_state(msdata);
 * 		} else if (curstate > MULTI_STOP_PREPARE) {
 * 			/*
 * 			 * At this stage all other CPUs we depend on must spin
 * 			 * in the same loop. Any reason for hard-lockup should
 * 			 * be detected and reported on their side.
 * 			 */
 * 			touch_nmi_watchdog();
 * 			/* Also suppress RCU CPU stall warnings. */
 * 			rcu_momentary_eqs();
 * 		}
 * 	} while (curstate != MULTI_STOP_EXIT);
 * 
 * 	local_irq_restore(flags);
 * 	return err;
 * }
 */
static int cpu_stop_queue_two_works(int cpu1, struct cpu_stop_work *work1,
				    int cpu2, struct cpu_stop_work *work2)
{
	struct cpu_stopper *stopper1 = per_cpu_ptr(&cpu_stopper, cpu1);
	struct cpu_stopper *stopper2 = per_cpu_ptr(&cpu_stopper, cpu2);
	int err;

retry:
	/*
	 * The waking up of stopper threads has to happen in the same
	 * scheduling context as the queueing.  Otherwise, there is a
	 * possibility of one of the above stoppers being woken up by another
	 * CPU, and preempting us. This will cause us to not wake up the other
	 * stopper forever.
	 */
	preempt_disable();
	raw_spin_lock_irq(&stopper1->lock);
	raw_spin_lock_nested(&stopper2->lock, SINGLE_DEPTH_NESTING);

	if (!stopper1->enabled || !stopper2->enabled) {
		err = -ENOENT;
		goto unlock;
	}

	/*
	 * Ensure that if we race with __stop_cpus() the stoppers won't get
	 * queued up in reverse order leading to system deadlock.
	 *
	 * We can't miss stop_cpus_in_progress if queue_stop_cpus_work() has
	 * queued a work on cpu1 but not on cpu2, we hold both locks.
	 *
	 * It can be falsely true but it is safe to spin until it is cleared,
	 * queue_stop_cpus_work() does everything under preempt_disable().
	 */
	if (unlikely(stop_cpus_in_progress)) {
		err = -EDEADLK;
		goto unlock;
	}

	err = 0;
	__cpu_stop_queue_work(stopper1, work1);
	__cpu_stop_queue_work(stopper2, work2);

unlock:
	raw_spin_unlock(&stopper2->lock);
	raw_spin_unlock_irq(&stopper1->lock);

	if (unlikely(err == -EDEADLK)) {
		preempt_enable();

		while (stop_cpus_in_progress)
			cpu_relax();

		goto retry;
	}

	if (!err) {
		wake_up_process(stopper1->thread);
		wake_up_process(stopper2->thread);
	}
	preempt_enable();

	return err;
}
/* Context-After
 * /**
 *  * stop_two_cpus - stops two cpus
 *  * @cpu1: the cpu to stop
 *  * @cpu2: the other cpu to stop
 *  * @fn: function to execute
 *  * @arg: argument to @fn
 *  *
 *  * Stops both the current and specified CPU and runs @fn on one of them.
 *  *
 *  * returns when both are completed.
 *  */
 * int stop_two_cpus(unsigned int cpu1, unsigned int cpu2, cpu_stop_fn_t fn, void *arg)
 * {
 * 	struct cpu_stop_done done;
 * 	struct cpu_stop_work work1, work2;
 * 	struct multi_stop_data msdata;
 * 
 * 	msdata = (struct multi_stop_data){
 * 		.fn = fn,
 * 		.data = arg,
 */
