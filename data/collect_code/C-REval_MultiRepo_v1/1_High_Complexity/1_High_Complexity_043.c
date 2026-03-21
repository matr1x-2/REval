/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_043 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 29 */
/* NLOC: 108 */
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
 * static int bpf_fill_atomic32_xor_fetch(struct bpf_test *self)
 * {
 * 	return __bpf_fill_atomic32(self, BPF_XOR | BPF_FETCH);
 * }
 * 
 * static int bpf_fill_atomic32_xchg(struct bpf_test *self)
 * {
 * 	return __bpf_fill_atomic32(self, BPF_XCHG);
 * }
 * 
 * static int bpf_fill_cmpxchg32(struct bpf_test *self)
 * {
 * 	return __bpf_fill_pattern(self, NULL, 64, 64, 0, PATTERN_BLOCK2,
 * 				  &__bpf_emit_cmpxchg32);
 * }
 * 
 * /*
 *  * Test JITs that implement ATOMIC operations as function calls or
 *  * other primitives, and must re-arrange operands for argument passing.
 *  */
 */
static int __bpf_fill_atomic_reg_pairs(struct bpf_test *self, u8 width, u8 op)
{
	struct bpf_insn *insn;
	int len = 2 + 34 * 10 * 10;
	u64 mem, upd, res;
	int rd, rs, i = 0;

	insn = kmalloc_objs(*insn, len);
	if (!insn)
		return -ENOMEM;

	/* Operand and memory values */
	if (width == BPF_DW) {
		mem = 0x0123456789abcdefULL;
		upd = 0xfedcba9876543210ULL;
	} else { /* BPF_W */
		mem = 0x01234567U;
		upd = 0x76543210U;
	}

	/* Memory updated according to operation */
	switch (op) {
	case BPF_XCHG:
		res = upd;
		break;
	case BPF_CMPXCHG:
		res = mem;
		break;
	default:
		__bpf_alu_result(&res, mem, upd, BPF_OP(op));
	}

	/* Test all operand registers */
	for (rd = R0; rd <= R9; rd++) {
		for (rs = R0; rs <= R9; rs++) {
			u64 cmp, src;

			/* Initialize value in memory */
			i += __bpf_ld_imm64(&insn[i], R0, mem);
			insn[i++] = BPF_STX_MEM(width, R10, R0, -8);

			/* Initialize registers in order */
			i += __bpf_ld_imm64(&insn[i], R0, ~mem);
			i += __bpf_ld_imm64(&insn[i], rs, upd);
			insn[i++] = BPF_MOV64_REG(rd, R10);

			/* Perform atomic operation */
			insn[i++] = BPF_ATOMIC_OP(width, op, rd, rs, -8);
			if (op == BPF_CMPXCHG && width == BPF_W)
				insn[i++] = BPF_ZEXT_REG(R0);

			/* Check R0 register value */
			if (op == BPF_CMPXCHG)
				cmp = mem;  /* Expect value from memory */
			else if (R0 == rd || R0 == rs)
				cmp = 0;    /* Aliased, checked below */
			else
				cmp = ~mem; /* Expect value to be preserved */
			if (cmp) {
				insn[i++] = BPF_JMP32_IMM(BPF_JEQ, R0,
							   (u32)cmp, 2);
				insn[i++] = BPF_MOV32_IMM(R0, __LINE__);
				insn[i++] = BPF_EXIT_INSN();
				insn[i++] = BPF_ALU64_IMM(BPF_RSH, R0, 32);
				insn[i++] = BPF_JMP32_IMM(BPF_JEQ, R0,
							   cmp >> 32, 2);
				insn[i++] = BPF_MOV32_IMM(R0, __LINE__);
				insn[i++] = BPF_EXIT_INSN();
			}

			/* Check source register value */
			if (rs == R0 && op == BPF_CMPXCHG)
				src = 0;   /* Aliased with R0, checked above */
			else if (rs == rd && (op == BPF_CMPXCHG ||
					      !(op & BPF_FETCH)))
				src = 0;   /* Aliased with rd, checked below */
			else if (op == BPF_CMPXCHG)
				src = upd; /* Expect value to be preserved */
			else if (op & BPF_FETCH)
				src = mem; /* Expect fetched value from mem */
			else /* no fetch */
				src = upd; /* Expect value to be preserved */
			if (src) {
				insn[i++] = BPF_JMP32_IMM(BPF_JEQ, rs,
							   (u32)src, 2);
				insn[i++] = BPF_MOV32_IMM(R0, __LINE__);
				insn[i++] = BPF_EXIT_INSN();
				insn[i++] = BPF_ALU64_IMM(BPF_RSH, rs, 32);
				insn[i++] = BPF_JMP32_IMM(BPF_JEQ, rs,
							   src >> 32, 2);
				insn[i++] = BPF_MOV32_IMM(R0, __LINE__);
				insn[i++] = BPF_EXIT_INSN();
			}

			/* Check destination register value */
			if (!(rd == R0 && op == BPF_CMPXCHG) &&
			    !(rd == rs && (op & BPF_FETCH))) {
				insn[i++] = BPF_JMP_REG(BPF_JEQ, rd, R10, 2);
				insn[i++] = BPF_MOV32_IMM(R0, __LINE__);
				insn[i++] = BPF_EXIT_INSN();
			}

			/* Check value in memory */
			if (rs != rd) {                  /* No aliasing */
				i += __bpf_ld_imm64(&insn[i], R1, res);
			} else if (op == BPF_XCHG) {     /* Aliased, XCHG */
				insn[i++] = BPF_MOV64_REG(R1, R10);
			} else if (op == BPF_CMPXCHG) {  /* Aliased, CMPXCHG */
				i += __bpf_ld_imm64(&insn[i], R1, mem);
			} else {                        /* Aliased, ALU oper */
				i += __bpf_ld_imm64(&insn[i], R1, mem);
				insn[i++] = BPF_ALU64_REG(BPF_OP(op), R1, R10);
			}

			insn[i++] = BPF_LDX_MEM(width, R0, R10, -8);
			if (width == BPF_DW)
				insn[i++] = BPF_JMP_REG(BPF_JEQ, R0, R1, 2);
			else /* width == BPF_W */
				insn[i++] = BPF_JMP32_REG(BPF_JEQ, R0, R1, 2);
			insn[i++] = BPF_MOV32_IMM(R0, __LINE__);
			insn[i++] = BPF_EXIT_INSN();
		}
	}

	insn[i++] = BPF_MOV64_IMM(R0, 1);
	insn[i++] = BPF_EXIT_INSN();

	self->u.ptr.insns = insn;
	self->u.ptr.len = i;
	BUG_ON(i > len);

	return 0;
}
/* Context-After
 * /* 64-bit atomic register tests */
 * static int bpf_fill_atomic64_add_reg_pairs(struct bpf_test *self)
 * {
 * 	return __bpf_fill_atomic_reg_pairs(self, BPF_DW, BPF_ADD);
 * }
 * 
 * static int bpf_fill_atomic64_and_reg_pairs(struct bpf_test *self)
 * {
 * 	return __bpf_fill_atomic_reg_pairs(self, BPF_DW, BPF_AND);
 * }
 * 
 * static int bpf_fill_atomic64_or_reg_pairs(struct bpf_test *self)
 * {
 * 	return __bpf_fill_atomic_reg_pairs(self, BPF_DW, BPF_OR);
 * }
 * 
 * static int bpf_fill_atomic64_xor_reg_pairs(struct bpf_test *self)
 * {
 * 	return __bpf_fill_atomic_reg_pairs(self, BPF_DW, BPF_XOR);
 */
