/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_032 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 26 */
/* NLOC: 96 */
/* Subsystem: lib */
/* Includes
 * #include <linux/init.h>
 * #include <linux/module.h>
 * #include <linux/filter.h>
 * #include <linux/bpf.h>
 * #include <linux/skbuff.h>
 * #include <linux/netdevice.h>
 * #include <linux/if_vlan.h>
 * #include <linux/prandom.h>
 * #include <linux/highmem.h>
 * #include <linux/sched.h>
 */
/* Context-Before
 * 			BPF_STX_MEM(BPF_W, R1, R2, 0),
 * 			TAIL_CALL(TAIL_CALL_INVALID),
 * 			BPF_EXIT_INSN(),
 * 		},
 * 		.flags = FLAG_NEED_STATE | FLAG_RESULT_IN_STATE,
 * 		.result = MAX_TESTRUNS,
 * 		.has_tail_call = true,
 * 	},
 * };
 * 
 * static void __init destroy_tail_call_tests(struct bpf_array *progs)
 * {
 * 	int i;
 * 
 * 	for (i = 0; i < ARRAY_SIZE(tail_call_tests); i++)
 * 		if (progs->ptrs[i])
 * 			bpf_prog_free(progs->ptrs[i]);
 * 	kfree(progs);
 * }
 */
static __init int prepare_tail_call_tests(struct bpf_array **pprogs)
{
	int ntests = ARRAY_SIZE(tail_call_tests);
	struct bpf_array *progs;
	int which, err;

	/* Allocate the table of programs to be used for tail calls */
	progs = kzalloc_flex(*progs, ptrs, ntests + 1);
	if (!progs)
		goto out_nomem;

	/* Create all eBPF programs and populate the table */
	for (which = 0; which < ntests; which++) {
		struct tail_call_test *test = &tail_call_tests[which];
		struct bpf_prog *fp;
		int len, i;

		/* Compute the number of program instructions */
		for (len = 0; len < MAX_INSNS; len++) {
			struct bpf_insn *insn = &test->insns[len];

			if (len < MAX_INSNS - 1 &&
			    insn->code == (BPF_LD | BPF_DW | BPF_IMM))
				len++;
			if (insn->code == 0)
				break;
		}

		/* Allocate and initialize the program */
		fp = bpf_prog_alloc(bpf_prog_size(len), 0);
		if (!fp)
			goto out_nomem;

		fp->len = len;
		fp->type = BPF_PROG_TYPE_SOCKET_FILTER;
		fp->aux->stack_depth = test->stack_depth;
		fp->aux->tail_call_reachable = test->has_tail_call;
		memcpy(fp->insnsi, test->insns, len * sizeof(struct bpf_insn));

		/* Relocate runtime tail call offsets and addresses */
		for (i = 0; i < len; i++) {
			struct bpf_insn *insn = &fp->insnsi[i];
			long addr = 0;

			switch (insn->code) {
			case BPF_LD | BPF_DW | BPF_IMM:
				if (insn->imm != TAIL_CALL_MARKER)
					break;
				insn[0].imm = (u32)(long)progs;
				insn[1].imm = ((u64)(long)progs) >> 32;
				break;

			case BPF_ALU | BPF_MOV | BPF_K:
				if (insn->imm != TAIL_CALL_MARKER)
					break;
				if (insn->off == TAIL_CALL_NULL)
					insn->imm = ntests;
				else if (insn->off == TAIL_CALL_INVALID)
					insn->imm = ntests + 1;
				else
					insn->imm = which + insn->off;
				insn->off = 0;
				break;

			case BPF_JMP | BPF_CALL:
				if (insn->src_reg != BPF_PSEUDO_CALL)
					break;
				switch (insn->imm) {
				case BPF_FUNC_get_numa_node_id:
					addr = (long)&numa_node_id;
					break;
				case BPF_FUNC_ktime_get_ns:
					addr = (long)&ktime_get_ns;
					break;
				case BPF_FUNC_ktime_get_boot_ns:
					addr = (long)&ktime_get_boot_fast_ns;
					break;
				case BPF_FUNC_ktime_get_coarse_ns:
					addr = (long)&ktime_get_coarse_ns;
					break;
				case BPF_FUNC_jiffies64:
					addr = (long)&get_jiffies_64;
					break;
				case BPF_FUNC_test_func:
					addr = (long)&bpf_test_func;
					break;
				default:
					err = -EFAULT;
					goto out_err;
				}
				*insn = BPF_EMIT_CALL(addr);
				if ((long)__bpf_call_base + insn->imm != addr)
					*insn = BPF_JMP_A(0); /* Skip: NOP */
				break;
			}
		}

		fp = bpf_prog_select_runtime(fp, &err);
		if (err)
			goto out_err;

		progs->ptrs[which] = fp;
	}

	/* The last entry contains a NULL program pointer */
	progs->map.max_entries = ntests + 1;
	*pprogs = progs;
	return 0;

out_nomem:
	err = -ENOMEM;

out_err:
	if (progs)
		destroy_tail_call_tests(progs);
	return err;
}
/* Context-After
 * static __init int test_tail_calls(struct bpf_array *progs)
 * {
 * 	int i, err_cnt = 0, pass_cnt = 0;
 * 	int jit_cnt = 0, run_cnt = 0;
 * 
 * 	for (i = 0; i < ARRAY_SIZE(tail_call_tests); i++) {
 * 		struct tail_call_test *test = &tail_call_tests[i];
 * 		struct bpf_prog *fp = progs->ptrs[i];
 * 		int *data = NULL;
 * 		int state = 0;
 * 		u64 duration;
 * 		int ret;
 * 
 * 		cond_resched();
 * 		if (exclude_test(i))
 * 			continue;
 * 
 * 		pr_info("#%d %s ", i, test->descr);
 * 		if (!fp) {
 */
