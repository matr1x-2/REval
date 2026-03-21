/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_046 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 52 */
/* NLOC: 218 */
/* Subsystem: tools */
/* Includes
 * #include <stdio.h>
 * #include <unistd.h>
 * #include <stdlib.h>
 * #include <ctype.h>
 * #include <stdbool.h>
 * #include <stdarg.h>
 * #include <setjmp.h>
 * #include <linux/filter.h>
 * #include <linux/if_packet.h>
 * #include <readline/readline.h>
 * #include <readline/history.h>
 * #include <sys/types.h>
 * #include <sys/socket.h>
 * #include <sys/stat.h>
 * #include <sys/mman.h>
 * #include <fcntl.h>
 * #include <errno.h>
 * #include <signal.h>
 * #include <arpa/inet.h>
 * #include <net/ethernet.h>
 */
/* Context-Before
 * {
 * 	int i;
 * 
 * 	rl_printf("%3u: ", 0);
 * 	for (i = 0; i < len; i++) {
 * 		if (i && !(i % 16))
 * 			rl_printf("\n%3u: ", i);
 * 		rl_printf("%02x ", buf[i]);
 * 	}
 * 	rl_printf("\n");
 * }
 * 
 * static bool bpf_prog_loaded(void)
 * {
 * 	if (bpf_prog_len == 0)
 * 		rl_printf("no bpf program loaded!\n");
 * 
 * 	return bpf_prog_len > 0;
 * }
 */
static void bpf_disasm(const struct sock_filter f, unsigned int i)
{
	const char *op, *fmt;
	int val = f.k;
	char buf[256];

	switch (f.code) {
	case BPF_RET | BPF_K:
		op = op_table[BPF_RET];
		fmt = "#%#x";
		break;
	case BPF_RET | BPF_A:
		op = op_table[BPF_RET];
		fmt = "a";
		break;
	case BPF_RET | BPF_X:
		op = op_table[BPF_RET];
		fmt = "x";
		break;
	case BPF_MISC_TAX:
		op = op_table[BPF_MISC_TAX];
		fmt = "";
		break;
	case BPF_MISC_TXA:
		op = op_table[BPF_MISC_TXA];
		fmt = "";
		break;
	case BPF_ST:
		op = op_table[BPF_ST];
		fmt = "M[%d]";
		break;
	case BPF_STX:
		op = op_table[BPF_STX];
		fmt = "M[%d]";
		break;
	case BPF_LD_W | BPF_ABS:
		op = op_table[BPF_LD_W];
		fmt = "[%d]";
		break;
	case BPF_LD_H | BPF_ABS:
		op = op_table[BPF_LD_H];
		fmt = "[%d]";
		break;
	case BPF_LD_B | BPF_ABS:
		op = op_table[BPF_LD_B];
		fmt = "[%d]";
		break;
	case BPF_LD_W | BPF_LEN:
		op = op_table[BPF_LD_W];
		fmt = "#len";
		break;
	case BPF_LD_W | BPF_IND:
		op = op_table[BPF_LD_W];
		fmt = "[x+%d]";
		break;
	case BPF_LD_H | BPF_IND:
		op = op_table[BPF_LD_H];
		fmt = "[x+%d]";
		break;
	case BPF_LD_B | BPF_IND:
		op = op_table[BPF_LD_B];
		fmt = "[x+%d]";
		break;
	case BPF_LD | BPF_IMM:
		op = op_table[BPF_LD_W];
		fmt = "#%#x";
		break;
	case BPF_LDX | BPF_IMM:
		op = op_table[BPF_LDX];
		fmt = "#%#x";
		break;
	case BPF_LDX_B | BPF_MSH:
		op = op_table[BPF_LDX_B];
		fmt = "4*([%d]&0xf)";
		break;
	case BPF_LD | BPF_MEM:
		op = op_table[BPF_LD_W];
		fmt = "M[%d]";
		break;
	case BPF_LDX | BPF_MEM:
		op = op_table[BPF_LDX];
		fmt = "M[%d]";
		break;
	case BPF_JMP_JA:
		op = op_table[BPF_JMP_JA];
		fmt = "%d";
		val = i + 1 + f.k;
		break;
	case BPF_JMP_JGT | BPF_X:
		op = op_table[BPF_JMP_JGT];
		fmt = "x";
		break;
	case BPF_JMP_JGT | BPF_K:
		op = op_table[BPF_JMP_JGT];
		fmt = "#%#x";
		break;
	case BPF_JMP_JGE | BPF_X:
		op = op_table[BPF_JMP_JGE];
		fmt = "x";
		break;
	case BPF_JMP_JGE | BPF_K:
		op = op_table[BPF_JMP_JGE];
		fmt = "#%#x";
		break;
	case BPF_JMP_JEQ | BPF_X:
		op = op_table[BPF_JMP_JEQ];
		fmt = "x";
		break;
	case BPF_JMP_JEQ | BPF_K:
		op = op_table[BPF_JMP_JEQ];
		fmt = "#%#x";
		break;
	case BPF_JMP_JSET | BPF_X:
		op = op_table[BPF_JMP_JSET];
		fmt = "x";
		break;
	case BPF_JMP_JSET | BPF_K:
		op = op_table[BPF_JMP_JSET];
		fmt = "#%#x";
		break;
	case BPF_ALU_NEG:
		op = op_table[BPF_ALU_NEG];
		fmt = "";
		break;
	case BPF_ALU_LSH | BPF_X:
		op = op_table[BPF_ALU_LSH];
		fmt = "x";
		break;
	case BPF_ALU_LSH | BPF_K:
		op = op_table[BPF_ALU_LSH];
		fmt = "#%d";
		break;
	case BPF_ALU_RSH | BPF_X:
		op = op_table[BPF_ALU_RSH];
		fmt = "x";
		break;
	case BPF_ALU_RSH | BPF_K:
		op = op_table[BPF_ALU_RSH];
		fmt = "#%d";
		break;
	case BPF_ALU_ADD | BPF_X:
		op = op_table[BPF_ALU_ADD];
		fmt = "x";
		break;
	case BPF_ALU_ADD | BPF_K:
		op = op_table[BPF_ALU_ADD];
		fmt = "#%d";
		break;
	case BPF_ALU_SUB | BPF_X:
		op = op_table[BPF_ALU_SUB];
		fmt = "x";
		break;
	case BPF_ALU_SUB | BPF_K:
		op = op_table[BPF_ALU_SUB];
		fmt = "#%d";
		break;
	case BPF_ALU_MUL | BPF_X:
		op = op_table[BPF_ALU_MUL];
		fmt = "x";
		break;
	case BPF_ALU_MUL | BPF_K:
		op = op_table[BPF_ALU_MUL];
		fmt = "#%d";
		break;
	case BPF_ALU_DIV | BPF_X:
		op = op_table[BPF_ALU_DIV];
		fmt = "x";
		break;
	case BPF_ALU_DIV | BPF_K:
		op = op_table[BPF_ALU_DIV];
		fmt = "#%d";
		break;
	case BPF_ALU_MOD | BPF_X:
		op = op_table[BPF_ALU_MOD];
		fmt = "x";
		break;
	case BPF_ALU_MOD | BPF_K:
		op = op_table[BPF_ALU_MOD];
		fmt = "#%d";
		break;
	case BPF_ALU_AND | BPF_X:
		op = op_table[BPF_ALU_AND];
		fmt = "x";
		break;
	case BPF_ALU_AND | BPF_K:
		op = op_table[BPF_ALU_AND];
		fmt = "#%#x";
		break;
	case BPF_ALU_OR | BPF_X:
		op = op_table[BPF_ALU_OR];
		fmt = "x";
		break;
	case BPF_ALU_OR | BPF_K:
		op = op_table[BPF_ALU_OR];
		fmt = "#%#x";
		break;
	case BPF_ALU_XOR | BPF_X:
		op = op_table[BPF_ALU_XOR];
		fmt = "x";
		break;
	case BPF_ALU_XOR | BPF_K:
		op = op_table[BPF_ALU_XOR];
		fmt = "#%#x";
		break;
	default:
		op = "nosup";
		fmt = "%#x";
		val = f.code;
		break;
	}

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), fmt, val);
	buf[sizeof(buf) - 1] = 0;

	if ((BPF_CLASS(f.code) == BPF_JMP && BPF_OP(f.code) != BPF_JA))
		rl_printf("l%d:\t%s %s, l%d, l%d\n", i, op, buf,
			  i + 1 + f.jt, i + 1 + f.jf);
	else
		rl_printf("l%d:\t%s %s\n", i, op, buf);
}
/* Context-After
 * static void bpf_dump_curr(struct bpf_regs *r, struct sock_filter *f)
 * {
 * 	int i, m = 0;
 * 
 * 	rl_printf("pc:       [%u]\n", r->Pc);
 * 	rl_printf("code:     [%u] jt[%u] jf[%u] k[%u]\n",
 * 		  f->code, f->jt, f->jf, f->k);
 * 	rl_printf("curr:     ");
 * 	bpf_disasm(*f, r->Pc);
 * 
 * 	if (f->jt || f->jf) {
 * 		rl_printf("jt:       ");
 * 		bpf_disasm(*(f + f->jt + 1), r->Pc + f->jt + 1);
 * 		rl_printf("jf:       ");
 * 		bpf_disasm(*(f + f->jf + 1), r->Pc + f->jf + 1);
 * 	}
 * 
 * 	rl_printf("A:        [%#08x][%u]\n", r->A, r->A);
 * 	rl_printf("X:        [%#08x][%u]\n", r->X, r->X);
 */
