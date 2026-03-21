/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_021 */
/* Category: 1_High_Complexity */
/* Repo: musl */
/* Cyclomatic Complexity: 59 */
/* NLOC: 174 */
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
 * 		self_done = 1;
 * 	} else {
 * 		p->funcdescs = malloc(size);
 * 	}
 * 	if (!p->funcdescs) {
 * 		if (!runtime) a_crash();
 * 		error("Error allocating function descriptors for %s", p->name);
 * 		longjmp(*rtld_fail, 1);
 * 	}
 * 	for (i=0; i<nsym; i++) {
 * 		if ((p->syms[i].st_info&0xf)==STT_FUNC && p->syms[i].st_shndx) {
 * 			p->funcdescs[i].addr = laddr(p, p->syms[i].st_value);
 * 			p->funcdescs[i].got = p->got;
 * 		} else {
 * 			p->funcdescs[i].addr = 0;
 * 			p->funcdescs[i].got = 0;
 * 		}
 * 	}
 * }
 */
static struct dso *load_library(const char *name, struct dso *needed_by)
{
	char buf[2*NAME_MAX+2];
	const char *pathname;
	unsigned char *map;
	struct dso *p, temp_dso = {0};
	int fd;
	struct stat st;
	size_t alloc_size;
	int n_th = 0;
	int is_self = 0;

	if (!*name) {
		errno = EINVAL;
		return 0;
	}

	/* Catch and block attempts to reload the implementation itself */
	if (name[0]=='l' && name[1]=='i' && name[2]=='b') {
		static const char reserved[] =
			"c.pthread.rt.m.dl.util.xnet.";
		const char *rp, *next;
		for (rp=reserved; *rp; rp=next) {
			next = strchr(rp, '.') + 1;
			if (strncmp(name+3, rp, next-rp) == 0)
				break;
		}
		if (*rp) {
			if (ldd_mode) {
				/* Track which names have been resolved
				 * and only report each one once. */
				static unsigned reported;
				unsigned mask = 1U<<(rp-reserved);
				if (!(reported & mask)) {
					reported |= mask;
					dprintf(1, "\t%s => %s (%p)\n",
						name, ldso.name,
						ldso.base);
				}
			}
			is_self = 1;
		}
	}
	if (!strcmp(name, ldso.name)) is_self = 1;
	if (is_self) {
		if (!ldso.prev) {
			tail->next = &ldso;
			ldso.prev = tail;
			tail = &ldso;
		}
		return &ldso;
	}
	if (strchr(name, '/')) {
		pathname = name;
		fd = open(name, O_RDONLY|O_CLOEXEC);
	} else {
		/* Search for the name to see if it's already loaded */
		for (p=head->next; p; p=p->next) {
			if (p->shortname && !strcmp(p->shortname, name)) {
				return p;
			}
		}
		if (strlen(name) > NAME_MAX) return 0;
		fd = -1;
		if (env_path) fd = path_open(name, env_path, buf, sizeof buf);
		for (p=needed_by; fd == -1 && p; p=p->needed_by) {
			if (fixup_rpath(p, buf, sizeof buf) < 0)
				fd = -2; /* Inhibit further search. */
			if (p->rpath)
				fd = path_open(name, p->rpath, buf, sizeof buf);
		}
		if (fd == -1) {
			if (!sys_path) {
				char *prefix = 0;
				size_t prefix_len;
				if (ldso.name[0]=='/') {
					char *s, *t, *z;
					for (s=t=z=ldso.name; *s; s++)
						if (*s=='/') z=t, t=s;
					prefix_len = z-ldso.name;
					if (prefix_len < PATH_MAX)
						prefix = ldso.name;
				}
				if (!prefix) {
					prefix = "";
					prefix_len = 0;
				}
				char etc_ldso_path[prefix_len + 1
					+ sizeof "/etc/ld-musl-" LDSO_ARCH ".path"];
				snprintf(etc_ldso_path, sizeof etc_ldso_path,
					"%.*s/etc/ld-musl-" LDSO_ARCH ".path",
					(int)prefix_len, prefix);
				fd = open(etc_ldso_path, O_RDONLY|O_CLOEXEC);
				if (fd>=0) {
					size_t n = 0;
					if (!fstat(fd, &st)) n = st.st_size;
					if ((sys_path = malloc(n+1)))
						sys_path[n] = 0;
					if (!sys_path || read_loop(fd, sys_path, n)<0) {
						free(sys_path);
						sys_path = "";
					}
					close(fd);
				} else if (errno != ENOENT) {
					sys_path = "";
				}
			}
			if (!sys_path) sys_path = "/lib:/usr/local/lib:/usr/lib";
			fd = path_open(name, sys_path, buf, sizeof buf);
		}
		pathname = buf;
	}
	if (fd < 0) return 0;
	if (fstat(fd, &st) < 0) {
		close(fd);
		return 0;
	}
	for (p=head->next; p; p=p->next) {
		if (p->dev == st.st_dev && p->ino == st.st_ino) {
			/* If this library was previously loaded with a
			 * pathname but a search found the same inode,
			 * setup its shortname so it can be found by name. */
			if (!p->shortname && pathname != name)
				p->shortname = strrchr(p->name, '/')+1;
			close(fd);
			return p;
		}
	}
	map = noload ? 0 : map_library(fd, &temp_dso);
	close(fd);
	if (!map) return 0;

	/* Avoid the danger of getting two versions of libc mapped into the
	 * same process when an absolute pathname was used. The symbols
	 * checked are chosen to catch both musl and glibc, and to avoid
	 * false positives from interposition-hack libraries. */
	decode_dyn(&temp_dso);
	if (find_sym(&temp_dso, "__libc_start_main", 1).sym &&
	    find_sym(&temp_dso, "stdin", 1).sym) {
		unmap_library(&temp_dso);
		return load_library("libc.so", needed_by);
	}
	/* Past this point, if we haven't reached runtime yet, ldso has
	 * committed either to use the mapped library or to abort execution.
	 * Unmapping is not possible, so we can safely reclaim gaps. */
	if (!runtime) reclaim_gaps(&temp_dso);

	/* Allocate storage for the new DSO. When there is TLS, this
	 * storage must include a reservation for all pre-existing
	 * threads to obtain copies of both the new TLS, and an
	 * extended DTV capable of storing an additional slot for
	 * the newly-loaded DSO. */
	alloc_size = sizeof *p + strlen(pathname) + 1;
	if (runtime && temp_dso.tls.image) {
		size_t per_th = temp_dso.tls.size + temp_dso.tls.align
			+ sizeof(void *) * (tls_cnt+3);
		n_th = libc.threads_minus_1 + 1;
		if (n_th > SSIZE_MAX / per_th) alloc_size = SIZE_MAX;
		else alloc_size += n_th * per_th;
	}
	p = calloc(1, alloc_size);
	if (!p) {
		unmap_library(&temp_dso);
		return 0;
	}
	memcpy(p, &temp_dso, sizeof temp_dso);
	p->dev = st.st_dev;
	p->ino = st.st_ino;
	p->needed_by = needed_by;
	p->name = p->buf;
	p->runtime_loaded = runtime;
	strcpy(p->name, pathname);
	/* Add a shortname only if name arg was not an explicit pathname. */
	if (pathname != name) p->shortname = strrchr(p->name, '/')+1;
	if (p->tls.image) {
		p->tls_id = ++tls_cnt;
		tls_align = MAXP2(tls_align, p->tls.align);
#ifdef TLS_ABOVE_TP
		p->tls.offset = tls_offset + ( (p->tls.align-1) &
			(-tls_offset + (uintptr_t)p->tls.image) );
		tls_offset = p->tls.offset + p->tls.size;
#else
		tls_offset += p->tls.size + p->tls.align - 1;
		tls_offset -= (tls_offset + (uintptr_t)p->tls.image)
			& (p->tls.align-1);
		p->tls.offset = tls_offset;
#endif
		p->new_dtv = (void *)(-sizeof(size_t) &
			(uintptr_t)(p->name+strlen(p->name)+sizeof(size_t)));
		p->new_tls = (void *)(p->new_dtv + n_th*(tls_cnt+1));
		if (tls_tail) tls_tail->next = &p->tls;
		else libc.tls_head = &p->tls;
		tls_tail = &p->tls;
	}

	tail->next = p;
	p->prev = tail;
	tail = p;

	if (DL_FDPIC) makefuncdescs(p);

	if (ldd_mode) dprintf(1, "\t%s => %s (%p)\n", name, pathname, p->base);

	return p;
}
/* Context-After
 * static void load_direct_deps(struct dso *p)
 * {
 * 	size_t i, cnt=0;
 * 
 * 	if (p->deps) return;
 * 	/* For head, all preloads are direct pseudo-dependencies.
 * 	 * Count and include them now to avoid realloc later. */
 * 	if (p==head) for (struct dso *q=p->next; q; q=q->next)
 * 		cnt++;
 * 	for (i=0; p->dynv[i]; i+=2)
 * 		if (p->dynv[i] == DT_NEEDED) cnt++;
 * 	/* Use builtin buffer for apps with no external deps, to
 * 	 * preserve property of no runtime failure paths. */
 * 	p->deps = (p==head && cnt<2) ? builtin_deps :
 * 		calloc(cnt+1, sizeof *p->deps);
 * 	if (!p->deps) {
 * 		error("Error loading dependencies for %s", p->name);
 * 		if (runtime) longjmp(*rtld_fail, 1);
 * 	}
 */
