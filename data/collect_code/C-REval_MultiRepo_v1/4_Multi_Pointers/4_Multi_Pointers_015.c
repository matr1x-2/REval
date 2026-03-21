/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_015 */
/* Category: 4_Multi_Pointers */
/* Repo: musl */
/* Cyclomatic Complexity: 70 */
/* NLOC: 220 */
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
 * 	 * use during dynamic linking. If possible it will also serve as the
 * 	 * thread pointer at runtime. */
 * 	search_vec(auxv, &__hwcap, AT_HWCAP);
 * 	libc.auxv = auxv;
 * 	libc.tls_size = sizeof builtin_tls;
 * 	libc.tls_align = tls_align;
 * 	if (__init_tp(__copy_tls((void *)builtin_tls)) < 0) {
 * 		a_crash();
 * 	}
 * 
 * 	struct symdef dls3_def = find_sym(&ldso, "__dls3", 0);
 * 	if (DL_FDPIC) ((stage3_func)&ldso.funcdescs[dls3_def.sym-ldso.syms])(sp, auxv);
 * 	else ((stage3_func)laddr(&ldso, dls3_def.sym->st_value))(sp, auxv);
 * }
 * 
 * /* Stage 3 of the dynamic linker is called with the dynamic linker/libc
 *  * fully functional. Its job is to load (if not already loaded) and
 *  * process dependencies and relocations for the main application and
 *  * transfer control to its entry point. */
 */
void __dls3(size_t *sp, size_t *auxv)
{
	static struct dso app, vdso;
	size_t aux[AUX_CNT];
	size_t i;
	char *env_preload=0;
	char *replace_argv0=0;
	size_t vdso_base;
	int argc = *sp;
	char **argv = (void *)(sp+1);
	char **argv_orig = argv;
	char **envp = argv+argc+1;

	/* Find aux vector just past environ[] and use it to initialize
	 * global data that may be needed before we can make syscalls. */
	__environ = envp;
	decode_vec(auxv, aux, AUX_CNT);
	search_vec(auxv, &__sysinfo, AT_SYSINFO);
	__pthread_self()->sysinfo = __sysinfo;
	libc.page_size = aux[AT_PAGESZ];
	libc.secure = ((aux[0]&0x7800)!=0x7800 || aux[AT_UID]!=aux[AT_EUID]
		|| aux[AT_GID]!=aux[AT_EGID] || aux[AT_SECURE]);

	/* Only trust user/env if kernel says we're not suid/sgid */
	if (!libc.secure) {
		env_path = getenv("LD_LIBRARY_PATH");
		env_preload = getenv("LD_PRELOAD");
	}

	/* Activate error handler function */
	error = error_impl;

	/* If the main program was already loaded by the kernel,
	 * AT_PHDR will point to some location other than the dynamic
	 * linker's program headers. */
	if (aux[AT_PHDR] != (size_t)ldso.phdr) {
		size_t interp_off = 0;
		size_t tls_image = 0;
		/* Find load address of the main program, via AT_PHDR vs PT_PHDR. */
		Phdr *phdr = app.phdr = (void *)aux[AT_PHDR];
		app.phnum = aux[AT_PHNUM];
		app.phentsize = aux[AT_PHENT];
		for (i=aux[AT_PHNUM]; i; i--, phdr=(void *)((char *)phdr + aux[AT_PHENT])) {
			if (phdr->p_type == PT_PHDR)
				app.base = (void *)(aux[AT_PHDR] - phdr->p_vaddr);
			else if (phdr->p_type == PT_INTERP)
				interp_off = (size_t)phdr->p_vaddr;
			else if (phdr->p_type == PT_TLS) {
				tls_image = phdr->p_vaddr;
				app.tls.len = phdr->p_filesz;
				app.tls.size = phdr->p_memsz;
				app.tls.align = phdr->p_align;
			}
		}
		if (DL_FDPIC) app.loadmap = app_loadmap;
		if (app.tls.size) app.tls.image = laddr(&app, tls_image);
		if (interp_off) ldso.name = laddr(&app, interp_off);
		if ((aux[0] & (1UL<<AT_EXECFN))
		    && strncmp((char *)aux[AT_EXECFN], "/proc/", 6))
			app.name = (char *)aux[AT_EXECFN];
		else
			app.name = argv[0];
		kernel_mapped_dso(&app);
	} else {
		int fd;
		char *ldname = argv[0];
		size_t l = strlen(ldname);
		if (l >= 3 && !strcmp(ldname+l-3, "ldd")) ldd_mode = 1;
		argv++;
		while (argv[0] && argv[0][0]=='-' && argv[0][1]=='-') {
			char *opt = argv[0]+2;
			*argv++ = (void *)-1;
			if (!*opt) {
				break;
			} else if (!memcmp(opt, "list", 5)) {
				ldd_mode = 1;
			} else if (!memcmp(opt, "library-path", 12)) {
				if (opt[12]=='=') env_path = opt+13;
				else if (opt[12]) *argv = 0;
				else if (*argv) env_path = *argv++;
			} else if (!memcmp(opt, "preload", 7)) {
				if (opt[7]=='=') env_preload = opt+8;
				else if (opt[7]) *argv = 0;
				else if (*argv) env_preload = *argv++;
			} else if (!memcmp(opt, "argv0", 5)) {
				if (opt[5]=='=') replace_argv0 = opt+6;
				else if (opt[5]) *argv = 0;
				else if (*argv) replace_argv0 = *argv++;
			} else {
				argv[0] = 0;
			}
		}
		argv[-1] = (void *)(argc - (argv-argv_orig));
		if (!argv[0]) {
			dprintf(2, "musl libc (" LDSO_ARCH ")\n"
				"Version %s\n"
				"Dynamic Program Loader\n"
				"Usage: %s [options] [--] pathname%s\n",
				__libc_version, ldname,
				ldd_mode ? "" : " [args]");
			_exit(1);
		}
		fd = open(argv[0], O_RDONLY);
		if (fd < 0) {
			dprintf(2, "%s: cannot load %s: %s\n", ldname, argv[0], strerror(errno));
			_exit(1);
		}
		Ehdr *ehdr = map_library(fd, &app);
		if (!ehdr) {
			dprintf(2, "%s: %s: Not a valid dynamic program\n", ldname, argv[0]);
			_exit(1);
		}
		close(fd);
		ldso.name = ldname;
		app.name = argv[0];
		aux[AT_ENTRY] = (size_t)laddr(&app, ehdr->e_entry);
		/* Find the name that would have been used for the dynamic
		 * linker had ldd not taken its place. */
		if (ldd_mode) {
			for (i=0; i<app.phnum; i++) {
				if (app.phdr[i].p_type == PT_INTERP)
					ldso.name = laddr(&app, app.phdr[i].p_vaddr);
			}
			dprintf(1, "\t%s (%p)\n", ldso.name, ldso.base);
		}
	}
	if (app.tls.size) {
		libc.tls_head = tls_tail = &app.tls;
		app.tls_id = tls_cnt = 1;
#ifdef TLS_ABOVE_TP
		app.tls.offset = GAP_ABOVE_TP;
		app.tls.offset += (-GAP_ABOVE_TP + (uintptr_t)app.tls.image)
			& (app.tls.align-1);
		tls_offset = app.tls.offset + app.tls.size;
#else
		tls_offset = app.tls.offset = app.tls.size
			+ ( -((uintptr_t)app.tls.image + app.tls.size)
			& (app.tls.align-1) );
#endif
		tls_align = MAXP2(tls_align, app.tls.align);
	}
	decode_dyn(&app);
	if (DL_FDPIC) {
		makefuncdescs(&app);
		if (!app.loadmap) {
			app.loadmap = (void *)&app_dummy_loadmap;
			app.loadmap->nsegs = 1;
			app.loadmap->segs[0].addr = (size_t)app.map;
			app.loadmap->segs[0].p_vaddr = (size_t)app.map
				- (size_t)app.base;
			app.loadmap->segs[0].p_memsz = app.map_len;
		}
		argv[-3] = (void *)app.loadmap;
	}

	/* Initial dso chain consists only of the app. */
	head = tail = syms_tail = &app;

	/* Donate unused parts of app and library mapping to malloc */
	reclaim_gaps(&app);
	reclaim_gaps(&ldso);

	/* Load preload/needed libraries, add symbols to global namespace. */
	ldso.deps = (struct dso **)no_deps;
	if (env_preload) load_preload(env_preload);
 	load_deps(&app);
	for (struct dso *p=head; p; p=p->next)
		add_syms(p);

	/* Attach to vdso, if provided by the kernel, last so that it does
	 * not become part of the global namespace.  */
	if (search_vec(auxv, &vdso_base, AT_SYSINFO_EHDR) && vdso_base) {
		Ehdr *ehdr = (void *)vdso_base;
		Phdr *phdr = vdso.phdr = (void *)(vdso_base + ehdr->e_phoff);
		vdso.phnum = ehdr->e_phnum;
		vdso.phentsize = ehdr->e_phentsize;
		for (i=ehdr->e_phnum; i; i--, phdr=(void *)((char *)phdr + ehdr->e_phentsize)) {
			if (phdr->p_type == PT_DYNAMIC)
				vdso.dynv = (void *)(vdso_base + phdr->p_offset);
			if (phdr->p_type == PT_LOAD)
				vdso.base = (void *)(vdso_base - phdr->p_vaddr + phdr->p_offset);
		}
		vdso.name = "";
		vdso.shortname = "linux-gate.so.1";
		vdso.relocated = 1;
		vdso.deps = (struct dso **)no_deps;
		decode_dyn(&vdso);
		vdso.prev = tail;
		tail->next = &vdso;
		tail = &vdso;
	}

	for (i=0; app.dynv[i]; i+=2) {
		if (!DT_DEBUG_INDIRECT && app.dynv[i]==DT_DEBUG)
			app.dynv[i+1] = (size_t)&debug;
		if (DT_DEBUG_INDIRECT && app.dynv[i]==DT_DEBUG_INDIRECT) {
			size_t *ptr = (size_t *) app.dynv[i+1];
			*ptr = (size_t)&debug;
		}
		if (app.dynv[i]==DT_DEBUG_INDIRECT_REL) {
			size_t *ptr = (size_t *)((size_t)&app.dynv[i] + app.dynv[i+1]);
			*ptr = (size_t)&debug;
		}
	}

	/* This must be done before final relocations, since it calls
	 * malloc, which may be provided by the application. Calling any
	 * application code prior to the jump to its entry point is not
	 * valid in our model and does not work with FDPIC, where there
	 * are additional relocation-like fixups that only the entry point
	 * code can see to perform. */
	main_ctor_queue = queue_ctors(&app);

	/* Initial TLS must also be allocated before final relocations
	 * might result in calloc being a call to application code. */
	update_tls_size();
	void *initial_tls = builtin_tls;
	if (libc.tls_size > sizeof builtin_tls || tls_align > MIN_TLS_ALIGN) {
		initial_tls = calloc(libc.tls_size, 1);
		if (!initial_tls) {
			dprintf(2, "%s: Error getting %zu bytes thread-local storage: %m\n",
				argv[0], libc.tls_size);
			_exit(127);
		}
	}
	static_tls_cnt = tls_cnt;

	/* The main program must be relocated LAST since it may contain
	 * copy relocations which depend on libraries' relocations. */
	reloc_all(app.next);
	reloc_all(&app);

	/* Actual copying to new TLS needs to happen after relocations,
	 * since the TLS images might have contained relocated addresses. */
	if (initial_tls != builtin_tls) {
		if (__init_tp(__copy_tls(initial_tls)) < 0) {
			a_crash();
		}
	} else {
		size_t tmp_tls_size = libc.tls_size;
		pthread_t self = __pthread_self();
		/* Temporarily set the tls size to the full size of
		 * builtin_tls so that __copy_tls will use the same layout
		 * as it did for before. Then check, just to be safe. */
		libc.tls_size = sizeof builtin_tls;
		if (__copy_tls((void*)builtin_tls) != self) a_crash();
		libc.tls_size = tmp_tls_size;
	}

	if (ldso_fail) _exit(127);
	if (ldd_mode) _exit(0);

	/* Determine if malloc was interposed by a replacement implementation
	 * so that calloc and the memalign family can harden against the
	 * possibility of incomplete replacement. */
	if (find_sym(head, "malloc", 1).dso != &ldso)
		__malloc_replaced = 1;
	if (find_sym(head, "aligned_alloc", 1).dso != &ldso)
		__aligned_alloc_replaced = 1;

	/* Switch to runtime mode: any further failures in the dynamic
	 * linker are a reportable failure rather than a fatal startup
	 * error. */
	runtime = 1;

	debug.ver = 1;
	debug.bp = dl_debug_state;
	debug.head = head;
	debug.base = ldso.base;
	debug.state = RT_CONSISTENT;
	_dl_debug_state();

	if (replace_argv0) argv[0] = replace_argv0;

	errno = 0;

	CRTJMP((void *)aux[AT_ENTRY], argv-1);
	for(;;);
}
/* Context-After
 * static void prepare_lazy(struct dso *p)
 * {
 * 	size_t dyn[DYN_CNT], n, flags1=0;
 * 	decode_vec(p->dynv, dyn, DYN_CNT);
 * 	search_vec(p->dynv, &flags1, DT_FLAGS_1);
 * 	if (dyn[DT_BIND_NOW] || (dyn[DT_FLAGS] & DF_BIND_NOW) || (flags1 & DF_1_NOW))
 * 		return;
 * 	n = dyn[DT_RELSZ]/2 + dyn[DT_RELASZ]/3 + dyn[DT_PLTRELSZ]/2 + 1;
 * 	if (NEED_MIPS_GOT_RELOCS) {
 * 		size_t j=0; search_vec(p->dynv, &j, DT_MIPS_GOTSYM);
 * 		size_t i=0; search_vec(p->dynv, &i, DT_MIPS_SYMTABNO);
 * 		n += i-j;
 * 	}
 * 	p->lazy = calloc(n, 3*sizeof(size_t));
 * 	if (!p->lazy) {
 * 		error("Error preparing lazy relocation for %s: %m", p->name);
 * 		longjmp(*rtld_fail, 1);
 * 	}
 * 	p->lazy_next = lazy_head;
 */
