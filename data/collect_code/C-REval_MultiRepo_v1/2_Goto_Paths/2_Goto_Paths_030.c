/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_030 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 16 */
/* NLOC: 62 */
/* Subsystem: arch */
/* Includes
 * #include <linux/cpumask.h>
 * #include <linux/export.h>
 * #include <linux/ftrace.h>
 * #include <linux/kallsyms.h>
 * #include <asm/inst.h>
 * #include <asm/loongson.h>
 * #include <asm/ptrace.h>
 * #include <asm/setup.h>
 * #include <asm/unwind.h>
 */
/* Context-Before
 * #endif
 * }
 * 
 * static inline bool unwind_state_fixup(struct unwind_state *state)
 * {
 * 	if (!fix_exception(state->pc) && !fix_ftrace(state->pc))
 * 		return false;
 * 
 * 	state->reset = true;
 * 	return true;
 * }
 * 
 * /*
 *  * LoongArch function prologue is like follows,
 *  *     [instructions not use stack var]
 *  *     addi.d sp, sp, -imm
 *  *     st.d   xx, sp, offset <- save callee saved regs and
 *  *     st.d   yy, sp, offset    save ra if function is nest.
 *  *     [others instructions]
 *  */
 */
static bool unwind_by_prologue(struct unwind_state *state)
{
	long frame_ra = -1;
	unsigned long frame_size = 0;
	unsigned long size, offset, pc;
	struct pt_regs *regs;
	struct stack_info *info = &state->stack_info;
	union loongarch_instruction *ip, *ip_end;

	if (state->sp >= info->end || state->sp < info->begin)
		return false;

	if (state->reset) {
		regs = (struct pt_regs *)state->sp;
		state->first = true;
		state->reset = false;
		state->pc = regs->csr_era;
		state->ra = regs->regs[1];
		state->sp = regs->regs[3];
		return true;
	}

	/*
	 * When first is not set, the PC is a return address in the previous frame.
	 * We need to adjust its value in case overflow to the next symbol.
	 */
	pc = state->pc - (state->first ? 0 : LOONGARCH_INSN_SIZE);
	if (!kallsyms_lookup_size_offset(pc, &size, &offset))
		return false;

	ip = (union loongarch_instruction *)(pc - offset);
	ip_end = (union loongarch_instruction *)pc;

	while (ip < ip_end) {
		if (is_stack_alloc_ins(ip)) {
			frame_size = (1 << 12) - ip->reg2i12_format.immediate;
			ip++;
			break;
		}
		ip++;
	}

	/*
	 * Can't find stack alloc action, PC may be in a leaf function. Only the
	 * first being true is reasonable, otherwise indicate analysis is broken.
	 */
	if (!frame_size) {
		if (state->first)
			goto first;

		return false;
	}

	while (ip < ip_end) {
		if (is_ra_save_ins(ip)) {
			frame_ra = ip->reg2i12_format.immediate;
			break;
		}
		if (is_branch_ins(ip))
			break;
		ip++;
	}

	/* Can't find save $ra action, PC may be in a leaf function, too. */
	if (frame_ra < 0) {
		if (state->first) {
			state->sp = state->sp + frame_size;
			goto first;
		}
		return false;
	}

	state->pc = *(unsigned long *)(state->sp + frame_ra);
	state->sp = state->sp + frame_size;
	goto out;

first:
	state->pc = state->ra;

out:
	state->first = false;
	return unwind_state_fixup(state) || __kernel_text_address(state->pc);
}
/* Context-After
 * static bool next_frame(struct unwind_state *state)
 * {
 * 	unsigned long pc;
 * 	struct pt_regs *regs;
 * 	struct stack_info *info = &state->stack_info;
 * 
 * 	if (unwind_done(state))
 * 		return false;
 * 
 * 	do {
 * 		if (unwind_by_prologue(state)) {
 * 			state->pc = unwind_graph_addr(state, state->pc, state->sp);
 * 			return true;
 * 		}
 * 
 * 		if (info->type == STACK_TYPE_IRQ && info->end == state->sp) {
 * 			regs = (struct pt_regs *)info->next_sp;
 * 			pc = regs->csr_era;
 */
