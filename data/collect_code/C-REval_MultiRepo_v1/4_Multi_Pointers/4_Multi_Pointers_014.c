/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_014 */
/* Category: 4_Multi_Pointers */
/* Repo: libevent */
/* Cyclomatic Complexity: 8 */
/* NLOC: 27 */
/* Subsystem: test */
/* Includes
 * #include "../util-internal.h"
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <string.h>
 * #include <assert.h>
 * #include <math.h>
 * #include <winsock2.h>
 * #include <ws2tcpip.h>
 * #include <sys/socket.h>
 * #include <netinet/in.h>
 * #include <signal.h>
 * #include "event2/bufferevent.h"
 * #include "event2/buffer.h"
 * #include "event2/event.h"
 * #include "event2/util.h"
 * #include "event2/listener.h"
 * #include "event2/thread.h"
 */
/* Context-Before
 * } options[] = {
 * 	{ "-v", &cfg_verbose, 0, 1 },
 * 	{ "-h", &cfg_help, 0, 1 },
 * 	{ "-n", &cfg_n_connections, 1, 0 },
 * 	{ "-d", &cfg_duration, 1, 0 },
 * 	{ "-c", &cfg_connlimit, 0, 0 },
 * 	{ "-g", &cfg_grouplimit, 0, 0 },
 * 	{ "-G", &cfg_group_drain, -100000, 0 },
 * 	{ "-t", &cfg_tick_msec, 10, 0 },
 * 	{ "--min-share", &cfg_min_share, 0, 0 },
 * 	{ "--check-connlimit", &cfg_connlimit_tolerance, 0, 0 },
 * 	{ "--check-grouplimit", &cfg_grouplimit_tolerance, 0, 0 },
 * 	{ "--check-stddev", &cfg_stddev_tolerance, 0, 0 },
 * #ifdef _WIN32
 * 	{ "--iocp", &cfg_enable_iocp, 0, 1 },
 * #endif
 * 	{ NULL, NULL, -1, 0 },
 * };
 * 
 * static int
 */
handle_option(int argc, char **argv, int *i, const struct option *opt)
{
	long val;
	char *endptr = NULL;
	if (opt->isbool) {
		*opt->ptr = 1;
		return 0;
	}
	if (*i + 1 == argc) {
		fprintf(stderr, "Too few arguments to '%s'\n",argv[*i]);
		return -1;
	}
	val = strtol(argv[*i+1], &endptr, 10);
	if (*argv[*i+1] == '\0' || !endptr || *endptr != '\0') {
		fprintf(stderr, "Couldn't parse numeric value '%s'\n",
		    argv[*i+1]);
		return -1;
	}
	if (val < opt->min || val > 0x7fffffff) {
		fprintf(stderr, "Value '%s' is out-of-range'\n",
		    argv[*i+1]);
		return -1;
	}
	*opt->ptr = (int)val;
	++*i;
	return 0;
}
/* Context-After
 * static void
 * usage(void)
 * {
 * 	fprintf(stderr,
 * "test-ratelim [-v] [-n INT] [-d INT] [-c INT] [-g INT] [-t INT]\n\n"
 * "Pushes bytes through a number of possibly rate-limited connections, and\n"
 * "displays average throughput.\n\n"
 * "  -n INT: Number of connections to open (default: 30)\n"
 * "  -d INT: Duration of the test in seconds (default: 5 sec)\n");
 * 	fprintf(stderr,
 * "  -c INT: Connection-rate limit applied to each connection in bytes per second\n"
 * "	   (default: None.)\n"
 * "  -g INT: Group-rate limit applied to sum of all usage in bytes per second\n"
 * "	   (default: None.)\n"
 * "  -G INT: drain INT bytes from the group limit every tick. (default: 0)\n"
 * "  -t INT: Granularity of timing, in milliseconds (default: 1000 msec)\n");
 * }
 * 
 * int
 */
