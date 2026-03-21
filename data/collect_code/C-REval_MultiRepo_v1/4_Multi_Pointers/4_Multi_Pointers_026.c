/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_026 */
/* Category: 4_Multi_Pointers */
/* Repo: musl */
/* Cyclomatic Complexity: 22 */
/* NLOC: 55 */
/* Subsystem: src */
/* Includes
 * #include <fcntl.h>
 * #include <unistd.h>
 * #include <sys/stat.h>
 * #include <ctype.h>
 * #include <pthread.h>
 * #include "pwf.h"
 */
/* Context-Before
 * 	s++; sp->sp_warn = xatol(&s);
 * 	if (*s != ':') return -1;
 * 
 * 	s++; sp->sp_inact = xatol(&s);
 * 	if (*s != ':') return -1;
 * 
 * 	s++; sp->sp_expire = xatol(&s);
 * 	if (*s != ':') return -1;
 * 
 * 	s++; sp->sp_flag = xatol(&s);
 * 	if (*s != '\n') return -1;
 * 	return 0;
 * }
 * 
 * static void cleanup(void *p)
 * {
 * 	fclose(p);
 * }
 */
int getspnam_r(const char *name, struct spwd *sp, char *buf, size_t size, struct spwd **res)
{
	char path[20+NAME_MAX];
	FILE *f = 0;
	int rv = 0;
	int fd;
	size_t k, l = strlen(name);
	int skip = 0;
	int cs;
	int orig_errno = errno;

	*res = 0;

	/* Disallow potentially-malicious user names */
	if (*name=='.' || strchr(name, '/') || !l)
		return errno = EINVAL;

	/* Buffer size must at least be able to hold name, plus some.. */
	if (size < l+100)
		return errno = ERANGE;

	/* Protect against truncation */
	if (snprintf(path, sizeof path, "/etc/tcb/%s/shadow", name) >= sizeof path)
		return errno = EINVAL;

	fd = open(path, O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC);
	if (fd >= 0) {
		struct stat st = { 0 };
		errno = EINVAL;
		if (fstat(fd, &st) || !S_ISREG(st.st_mode) || !(f = fdopen(fd, "rb"))) {
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cs);
			close(fd);
			pthread_setcancelstate(cs, 0);
			return errno;
		}
	} else {
		if (errno != ENOENT && errno != ENOTDIR)
			return errno;
		f = fopen("/etc/shadow", "rbe");
		if (!f) {
			if (errno != ENOENT && errno != ENOTDIR)
				return errno;
			return 0;
		}
	}

	pthread_cleanup_push(cleanup, f);
	while (fgets(buf, size, f) && (k=strlen(buf))>0) {
		if (skip || strncmp(name, buf, l) || buf[l]!=':') {
			skip = buf[k-1] != '\n';
			continue;
		}
		if (buf[k-1] != '\n') {
			rv = ERANGE;
			break;
		}

		if (__parsespent(buf, sp) < 0) continue;
		*res = sp;
		break;
	}
	pthread_cleanup_pop(1);
	errno = rv ? rv : orig_errno;
	return rv;
}
/* Context-After: <empty> */
