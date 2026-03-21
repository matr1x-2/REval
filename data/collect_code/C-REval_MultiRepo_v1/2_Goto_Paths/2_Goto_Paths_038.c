/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_038 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 53 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/cpumask.h>
 * #include <linux/cpufreq.h>
 * #include <linux/cpuhotplug.h>
 * #include <linux/dtpm.h>
 * #include <linux/energy_model.h>
 * #include <linux/of.h>
 * #include <linux/pm_qos.h>
 * #include <linux/slab.h>
 */
/* Context-Before
 * 	struct dtpm_cpu *dtpm_cpu;
 * 
 * 	dtpm_cpu = per_cpu(dtpm_per_cpu, cpu);
 * 	if (dtpm_cpu)
 * 		dtpm_update_power(&dtpm_cpu->dtpm);
 * 
 * 	return 0;
 * }
 * 
 * static int cpuhp_dtpm_cpu_online(unsigned int cpu)
 * {
 * 	struct dtpm_cpu *dtpm_cpu;
 * 
 * 	dtpm_cpu = per_cpu(dtpm_per_cpu, cpu);
 * 	if (dtpm_cpu)
 * 		return dtpm_update_power(&dtpm_cpu->dtpm);
 * 
 * 	return 0;
 * }
 */
static int __dtpm_cpu_setup(int cpu, struct dtpm *parent)
{
	struct dtpm_cpu *dtpm_cpu;
	struct cpufreq_policy *policy;
	struct em_perf_state *table;
	struct em_perf_domain *pd;
	char name[CPUFREQ_NAME_LEN];
	int ret = -ENOMEM;

	dtpm_cpu = per_cpu(dtpm_per_cpu, cpu);
	if (dtpm_cpu)
		return 0;

	policy = cpufreq_cpu_get(cpu);
	if (!policy)
		return 0;

	pd = em_cpu_get(cpu);
	if (!pd || em_is_artificial(pd)) {
		ret = -EINVAL;
		goto release_policy;
	}

	dtpm_cpu = kzalloc_obj(*dtpm_cpu);
	if (!dtpm_cpu) {
		ret = -ENOMEM;
		goto release_policy;
	}

	dtpm_init(&dtpm_cpu->dtpm, &dtpm_ops);
	dtpm_cpu->cpu = cpu;

	for_each_cpu(cpu, policy->related_cpus)
		per_cpu(dtpm_per_cpu, cpu) = dtpm_cpu;

	snprintf(name, sizeof(name), "cpu%d-cpufreq", dtpm_cpu->cpu);

	ret = dtpm_register(name, &dtpm_cpu->dtpm, parent);
	if (ret)
		goto out_kfree_dtpm_cpu;

	rcu_read_lock();
	table = em_perf_state_from_pd(pd);
	ret = freq_qos_add_request(&policy->constraints,
				   &dtpm_cpu->qos_req, FREQ_QOS_MAX,
				   table[pd->nr_perf_states - 1].frequency);
	rcu_read_unlock();
	if (ret < 0)
		goto out_dtpm_unregister;

	cpufreq_cpu_put(policy);
	return 0;

out_dtpm_unregister:
	dtpm_unregister(&dtpm_cpu->dtpm);
	dtpm_cpu = NULL;

out_kfree_dtpm_cpu:
	for_each_cpu(cpu, policy->related_cpus)
		per_cpu(dtpm_per_cpu, cpu) = NULL;
	kfree(dtpm_cpu);

release_policy:
	cpufreq_cpu_put(policy);
	return ret;
}
/* Context-After
 * static int dtpm_cpu_setup(struct dtpm *dtpm, struct device_node *np)
 * {
 * 	int cpu;
 * 
 * 	cpu = of_cpu_node_to_id(np);
 * 	if (cpu < 0)
 * 		return 0;
 * 
 * 	return __dtpm_cpu_setup(cpu, dtpm);
 * }
 * 
 * static int dtpm_cpu_init(void)
 * {
 * 	int ret;
 * 
 * 	/*
 * 	 * The callbacks at CPU hotplug time are calling
 * 	 * dtpm_update_power() which in turns calls update_pd_power().
 * 	 *
 */
