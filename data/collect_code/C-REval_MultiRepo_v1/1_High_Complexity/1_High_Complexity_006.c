/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_006 */
/* Category: 1_High_Complexity */
/* Repo: musl */
/* Cyclomatic Complexity: 58 */
/* NLOC: 160 */
/* Subsystem: ldso */
/* Includes
 * #include <stdlib.h>
 * #include <stdarg.h>
 * #include <stddef.h>
 * #include <string.h>
 * #include <unistd.h>
 * #include <stdint.h>
 * #include <elf.h>
 * #include <sys/mman.h>
 * #include <limits.h>
 * #include <fcntl.h>
 * #include <sys/stat.h>
 * #include <errno.h>
 * #include <link.h>
 * #include <setjmp.h>
 * #include <pthread.h>
 * #include <ctype.h>
 * #include <dlfcn.h>
 * #include <semaphore.h>
 * #include <sys/membarrier.h>
 * #include "pthread_impl.h"
 */
/* Context-Before
 * 		"lockf\0lseek\0lstat\0mkostemp\0mkostemps\0mkstemp\0"
 * 		"mkstemps\0mmap\0nftw\0open\0openat\0posix_fadvise\0"
 * 		"posix_fallocate\0pread\0preadv\0prlimit\0pwrite\0"
 * 		"pwritev\0readdir\0scandir\0sendfile\0setrlimit\0"
 * 		"stat\0statfs\0statvfs\0tmpfile\0truncate\0versionsort\0"
 * 		"__fxstat\0__fxstatat\0__lxstat\0__xstat\0";
 * 	if (!strcmp(name, "readdir64_r"))
 * 		return find_sym(&ldso, "readdir_r", 1);
 * 	size_t l = strnlen(name, 18);
 * 	if (l<2 || name[l-2]!='6' || name[l-1]!='4' || name[l])
 * 		goto nomatch;
 * 	for (p=lfs64_list; *p; p++) {
 * 		if (!strncmp(name, p, l-2) && !p[l-2])
 * 			return find_sym(&ldso, p, 1);
 * 		while (*p) p++;
 * 	}
 * nomatch:
 * 	return (struct symdef){ 0 };
 * }
 */
static void do_relocs(struct dso *dso, size_t *rel, size_t rel_size, size_t stride)
{
	unsigned char *base = dso->base;
	Sym *syms = dso->syms;
	char *strings = dso->strings;
	Sym *sym;
	const char *name;
	void *ctx;
	int type;
	int sym_index;
	struct symdef def;
	size_t *reloc_addr;
	size_t sym_val;
	size_t tls_val;
	size_t addend;
	int skip_relative = 0, reuse_addends = 0, save_slot = 0;

	if (dso == &ldso) {
		/* Only ldso's REL table needs addend saving/reuse. */
		if (rel == apply_addends_to)
			reuse_addends = 1;
		skip_relative = 1;
	}

	for (; rel_size; rel+=stride, rel_size-=stride*sizeof(size_t)) {
		if (skip_relative && IS_RELATIVE(rel[1], dso->syms)) continue;
		type = R_TYPE(rel[1]);
		if (type == REL_NONE) continue;
		reloc_addr = laddr(dso, rel[0]);

		if (stride > 2) {
			addend = rel[2];
		} else if (type==REL_GOT || type==REL_PLT|| type==REL_COPY) {
			addend = 0;
		} else if (reuse_addends) {
			/* Save original addend in stage 2 where the dso
			 * chain consists of just ldso; otherwise read back
			 * saved addend since the inline one was clobbered. */
			if (head==&ldso)
				saved_addends[save_slot] = *reloc_addr;
			addend = saved_addends[save_slot++];
		} else {
			addend = *reloc_addr;
		}

		sym_index = R_SYM(rel[1]);
		if (sym_index) {
			sym = syms + sym_index;
			name = strings + sym->st_name;
			ctx = type==REL_COPY ? head->syms_next : head;
			def = (sym->st_info>>4) == STB_LOCAL
				? (struct symdef){ .dso = dso, .sym = sym }
				: find_sym(ctx, name, type==REL_PLT);
			if (!def.sym) def = get_lfs64(name);
			if (!def.sym && (sym->st_shndx != SHN_UNDEF
			    || sym->st_info>>4 != STB_WEAK)) {
				if (dso->lazy && (type==REL_PLT || type==REL_GOT)) {
					dso->lazy[3*dso->lazy_cnt+0] = rel[0];
					dso->lazy[3*dso->lazy_cnt+1] = rel[1];
					dso->lazy[3*dso->lazy_cnt+2] = addend;
					dso->lazy_cnt++;
					continue;
				}
				error("Error relocating %s: %s: symbol not found",
					dso->name, name);
				if (runtime) longjmp(*rtld_fail, 1);
				continue;
			}
		} else {
			sym = 0;
			def.sym = 0;
			def.dso = dso;
		}

		sym_val = def.sym ? (size_t)laddr(def.dso, def.sym->st_value) : 0;
		tls_val = def.sym ? def.sym->st_value : 0;

		if ((type == REL_TPOFF || type == REL_TPOFF_NEG)
		    && def.dso->tls_id > static_tls_cnt) {
			error("Error relocating %s: %s: initial-exec TLS "
				"resolves to dynamic definition in %s",
				dso->name, name, def.dso->name);
			longjmp(*rtld_fail, 1);
		}

		switch(type) {
		case REL_OFFSET:
			addend -= (size_t)reloc_addr;
		case REL_SYMBOLIC:
		case REL_GOT:
		case REL_PLT:
			*reloc_addr = sym_val + addend;
			break;
		case REL_USYMBOLIC:
			memcpy(reloc_addr, &(size_t){sym_val + addend}, sizeof(size_t));
			break;
		case REL_RELATIVE:
			*reloc_addr = (size_t)base + addend;
			break;
		case REL_SYM_OR_REL:
			if (sym) *reloc_addr = sym_val + addend;
			else *reloc_addr = (size_t)base + addend;
			break;
		case REL_COPY:
			memcpy(reloc_addr, (void *)sym_val, sym->st_size);
			break;
		case REL_OFFSET32:
			*(uint32_t *)reloc_addr = sym_val + addend
				- (size_t)reloc_addr;
			break;
		case REL_FUNCDESC:
			*reloc_addr = def.sym ? (size_t)(def.dso->funcdescs
				+ (def.sym - def.dso->syms)) : 0;
			break;
		case REL_FUNCDESC_VAL:
			if ((sym->st_info&0xf) == STT_SECTION) *reloc_addr += sym_val;
			else *reloc_addr = sym_val;
			reloc_addr[1] = def.sym ? (size_t)def.dso->got : 0;
			break;
		case REL_DTPMOD:
			*reloc_addr = def.dso->tls_id;
			break;
		case REL_DTPOFF:
			*reloc_addr = tls_val + addend - DTP_OFFSET;
			break;
#ifdef TLS_ABOVE_TP
		case REL_TPOFF:
			*reloc_addr = tls_val + def.dso->tls.offset + TPOFF_K + addend;
			break;
#else
		case REL_TPOFF:
			*reloc_addr = tls_val - def.dso->tls.offset + addend;
			break;
		case REL_TPOFF_NEG:
			*reloc_addr = def.dso->tls.offset - tls_val + addend;
			break;
#endif
		case REL_TLSDESC:
			if (stride<3) addend = reloc_addr[!TLSDESC_BACKWARDS];
			if (def.dso->tls_id > static_tls_cnt) {
				struct td_index *new = malloc(sizeof *new);
				if (!new) {
					error(
					"Error relocating %s: cannot allocate TLSDESC for %s",
					dso->name, sym ? name : "(local)" );
					longjmp(*rtld_fail, 1);
				}
				new->next = dso->td_index;
				dso->td_index = new;
				new->args[0] = def.dso->tls_id;
				new->args[1] = tls_val + addend - DTP_OFFSET;
				reloc_addr[0] = (size_t)__tlsdesc_dynamic;
				reloc_addr[1] = (size_t)new;
			} else {
				reloc_addr[0] = (size_t)__tlsdesc_static;
#ifdef TLS_ABOVE_TP
				reloc_addr[1] = tls_val + def.dso->tls.offset
					+ TPOFF_K + addend;
#else
				reloc_addr[1] = tls_val - def.dso->tls.offset
					+ addend;
#endif
			}
			/* Some archs (32-bit ARM at least) invert the order of
			 * the descriptor members. Fix them up here. */
			if (TLSDESC_BACKWARDS) {
				size_t tmp = reloc_addr[0];
				reloc_addr[0] = reloc_addr[1];
				reloc_addr[1] = tmp;
			}
			break;
		default:
			error("Error relocating %s: unsupported relocation type %d",
				dso->name, type);
			if (runtime) longjmp(*rtld_fail, 1);
			continue;
		}
	}
}
/* Context-After
 * static void do_relr_relocs(struct dso *dso, size_t *relr, size_t relr_size)
 * {
 * 	if (dso == &ldso) return; /* self-relocation was done in _dlstart */
 * 	unsigned char *base = dso->base;
 * 	size_t *reloc_addr;
 * 	for (; relr_size; relr++, relr_size-=sizeof(size_t))
 * 		if ((relr[0]&1) == 0) {
 * 			reloc_addr = laddr(dso, relr[0]);
 * 			*reloc_addr++ += (size_t)base;
 * 		} else {
 * 			int i = 0;
 * 			for (size_t bitmap=relr[0]; (bitmap>>=1); i++)
 * 				if (bitmap&1)
 * 					reloc_addr[i] += (size_t)base;
 * 			reloc_addr += 8*sizeof(size_t)-1;
 * 		}
 * }
 * 
 * static void redo_lazy_relocs()
 */
