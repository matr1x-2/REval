/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_034 */
/* Category: 4_Multi_Pointers */
/* Repo: libevent */
/* Cyclomatic Complexity: 7 */
/* NLOC: 18 */
/* Subsystem: evdns.c */
/* Includes
 * #include "event2/event-config.h"
 * #include "evconfig-private.h"
 * #include <sys/types.h>
 * #include <string.h>
 * #include <fcntl.h>
 * #include <sys/time.h>
 * #include <stdint.h>
 * #include <stdlib.h>
 * #include <string.h>
 * #include <errno.h>
 * #include <unistd.h>
 * #include <limits.h>
 * #include <sys/stat.h>
 * #include <stdio.h>
 * #include <stdarg.h>
 * #include <sys/tree.h>
 * #include <winsock2.h>
 * #include <winerror.h>
 * #include <ws2tcpip.h>
 * #include <shlobj.h>
 */
/* Context-Before
 * /* ================================================================= */
 * /* Parsing resolv.conf files */
 * 
 * static void
 * evdns_resolv_set_defaults(struct evdns_base *base, int flags) {
 * 	int add_default = flags & DNS_OPTION_NAMESERVERS;
 * 	if (flags & DNS_OPTION_NAMESERVERS_NO_DEFAULT)
 * 		add_default = 0;
 * 
 * 	/* if the file isn't found then we assume a local resolver */
 * 	ASSERT_LOCKED(base);
 * 	if (flags & DNS_OPTION_SEARCH)
 * 		search_set_from_hostname(base);
 * 	if (add_default)
 * 		evdns_base_nameserver_ip_add(base, "127.0.0.1");
 * }
 * 
 * #ifndef EVENT__HAVE_STRTOK_R
 * static char *
 */
strtok_r(char *s, const char *delim, char **state) {
	char *cp, *start;
	start = cp = s ? s : *state;
	if (!cp)
		return NULL;
	while (*cp && !strchr(delim, *cp))
		++cp;
	if (!*cp) {
		if (cp == start)
			return NULL;
		*state = NULL;
		return start;
	} else {
		*cp++ = '\0';
		*state = cp;
		return start;
	}
}
/* Context-After
 * #endif
 * 
 * /* helper version of atoi which returns -1 on error */
 * static int
 * strtoint(const char *const str)
 * {
 * 	char *endptr;
 * 	const int r = strtol(str, &endptr, 10);
 * 	if (*endptr) return -1;
 * 	return r;
 * }
 * 
 * /* Parse a number of seconds into a timeval; return -1 on error. */
 * static int
 * evdns_strtotimeval(const char *const str, struct timeval *out)
 * {
 * 	double d;
 * 	char *endptr;
 * 	d = strtod(str, &endptr);
 * 	if (*endptr) return -1;
 */
