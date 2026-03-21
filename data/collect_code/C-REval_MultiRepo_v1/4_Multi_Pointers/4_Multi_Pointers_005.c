/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_005 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 10 */
/* NLOC: 46 */
/* Subsystem: tools */
/* Includes
 * #include <elf.h>
 * #include <errno.h>
 * #include <gelf.h>
 * #include <fcntl.h>
 * #include <inttypes.h>
 * #include <string.h>
 * #include <unistd.h>
 * #include <sys/mman.h>
 * #include <linux/list.h>
 * #include <linux/zalloc.h>
 * #include <libunwind.h>
 * #include <libunwind-ptrace.h>
 * #include "callchain.h"
 * #include "thread.h"
 * #include "session.h"
 * #include "perf_regs.h"
 * #include "unwind.h"
 * #include "map.h"
 * #include "symbol.h"
 * #include "debug.h"
 */
/* Context-Before
 *  * #define DW_EH_PE_datarel       0x30
 *  */
 * 
 * struct unwind_info {
 * 	struct perf_sample	*sample;
 * 	struct machine		*machine;
 * 	struct thread		*thread;
 * 	bool			 best_effort;
 * };
 * 
 * #define dw_read(ptr, type, end) ({	\
 * 	type *__p = (type *) ptr;	\
 * 	type  __v;			\
 * 	if ((__p + 1) > (type *) end)	\
 * 		return -EINVAL;		\
 * 	__v = *__p++;			\
 * 	ptr = (typeof(ptr)) __p;	\
 * 	__v;				\
 * 	})
 */
static int __dw_read_encoded_value(u8 **p, u8 *end, u64 *val,
				   u8 encoding)
{
	u8 *cur = *p;
	*val = 0;

	switch (encoding) {
	case DW_EH_PE_omit:
		*val = 0;
		goto out;
	case DW_EH_PE_ptr:
		*val = dw_read(cur, unsigned long, end);
		goto out;
	default:
		break;
	}

	switch (encoding & DW_EH_PE_APPL_MASK) {
	case DW_EH_PE_absptr:
		break;
	case DW_EH_PE_pcrel:
		*val = (unsigned long) cur;
		break;
	default:
		return -EINVAL;
	}

	if ((encoding & 0x07) == 0x00)
		encoding |= DW_EH_PE_udata4;

	switch (encoding & DW_EH_PE_FORMAT_MASK) {
	case DW_EH_PE_sdata4:
		*val += dw_read(cur, s32, end);
		break;
	case DW_EH_PE_udata4:
		*val += dw_read(cur, u32, end);
		break;
	case DW_EH_PE_sdata8:
		*val += dw_read(cur, s64, end);
		break;
	case DW_EH_PE_udata8:
		*val += dw_read(cur, u64, end);
		break;
	default:
		return -EINVAL;
	}

 out:
	*p = cur;
	return 0;
}
/* Context-After
 * #define dw_read_encoded_value(ptr, end, enc) ({			\
 * 	u64 __v;						\
 * 	if (__dw_read_encoded_value(&ptr, end, &__v, enc)) {	\
 * 		return -EINVAL;                                 \
 * 	}                                                       \
 * 	__v;                                                    \
 * 	})
 * 
 * static int elf_section_address_and_offset(int fd, const char *name, u64 *address, u64 *offset)
 * {
 * 	Elf *elf;
 * 	GElf_Ehdr ehdr;
 * 	GElf_Shdr shdr;
 * 	int ret = -1;
 * 
 * 	elf = elf_begin(fd, PERF_ELF_C_READ_MMAP, NULL);
 * 	if (elf == NULL)
 * 		return -1;
 */
