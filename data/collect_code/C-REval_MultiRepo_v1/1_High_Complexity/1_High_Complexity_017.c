/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_017 */
/* Category: 1_High_Complexity */
/* Repo: libevent */
/* Cyclomatic Complexity: 59 */
/* NLOC: 126 */
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
 * 			log(EVDNS_LOG_WARN, "EVDNS_SOPT_TCP_IDLE_TIMEOUT option can be set only on TCP server");
 * 			res = -1;
 * 			goto end;
 * 		}
 * 		port->tcp_idle_timeout.tv_sec = value;
 * 		port->tcp_idle_timeout.tv_usec = 0;
 * 		log(EVDNS_LOG_DEBUG, "Setting EVDNS_SOPT_TCP_IDLE_TIMEOUT to %u seconds",
 * 			(unsigned)port->tcp_idle_timeout.tv_sec);
 * 		break;
 * 	default:
 * 		log(EVDNS_LOG_WARN, "Invalid DNS server option %d", (int)option);
 * 		res = -1;
 * 		break;
 * 	}
 * end:
 * 	EVDNS_UNLOCK(port);
 * 	return res;
 * }
 * 
 * static int
 */
evdns_base_set_option_impl(struct evdns_base *base,
    const char *option, const char *val, int flags)
{
	ASSERT_LOCKED(base);
	if (str_matches_option(option, "ndots:")) {
		const int ndots = strtoint(val);
		if (ndots == -1) return -1;
		if (!(flags & DNS_OPTION_SEARCH)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting ndots to %d", ndots);
		if (!base->global_search_state) base->global_search_state = search_state_new();
		if (!base->global_search_state) return -1;
		base->global_search_state->ndots = ndots;
	} else if (str_matches_option(option, "timeout:")) {
		struct timeval tv;
		if (evdns_strtotimeval(val, &tv) == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting timeout to %s", val);
		memcpy(&base->global_timeout, &tv, sizeof(struct timeval));
	} else if (str_matches_option(option, "getaddrinfo-allow-skew:")) {
		struct timeval tv;
		if (evdns_strtotimeval(val, &tv) == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting getaddrinfo-allow-skew to %s",
		    val);
		memcpy(&base->global_getaddrinfo_allow_skew, &tv,
		    sizeof(struct timeval));
	} else if (str_matches_option(option, "max-timeouts:")) {
		const int maxtimeout = strtoint_clipped(val, 1, 255);
		if (maxtimeout == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting maximum allowed timeouts to %d",
			maxtimeout);
		base->global_max_nameserver_timeout = maxtimeout;
	} else if (str_matches_option(option, "max-inflight:")) {
		const int maxinflight = strtoint_clipped(val, 1, 65000);
		if (maxinflight == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting maximum inflight requests to %d",
			maxinflight);
		evdns_base_set_max_requests_inflight(base, maxinflight);
	} else if (str_matches_option(option, "attempts:")) {
		int retries = strtoint(val);
		if (retries == -1) return -1;
		if (retries > 255) retries = 255;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting retries to %d", retries);
		base->global_max_retransmits = retries;
	} else if (str_matches_option(option, "randomize-case:")) {
		int randcase = strtoint(val);
		if (randcase == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		base->global_randomize_case = randcase;
	} else if (str_matches_option(option, "bind-to:")) {
		/* XXX This only applies to successive nameservers, not
		 * to already-configured ones.	We might want to fix that. */
		int len = sizeof(base->global_outgoing_address);
		if (!(flags & DNS_OPTION_NAMESERVERS)) return 0;
		if (evutil_parse_sockaddr_port(val,
			(struct sockaddr*)&base->global_outgoing_address, &len))
			return -1;
		base->global_outgoing_addrlen = len;
	} else if (str_matches_option(option, "initial-probe-timeout:")) {
		struct timeval tv;
		if (evdns_strtotimeval(val, &tv) == -1) return -1;
		if (tv.tv_sec > 3600)
			tv.tv_sec = 3600;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting initial probe timeout to %s",
		    val);
		memcpy(&base->global_nameserver_probe_initial_timeout, &tv,
		    sizeof(tv));
	} else if (str_matches_option(option, "max-probe-timeout:")) {
		const int max_probe_timeout = strtoint_clipped(val, 1, 3600);
		if (max_probe_timeout == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting maximum probe timeout to %d",
			max_probe_timeout);
		base->ns_max_probe_timeout = max_probe_timeout;
		if (base->global_nameserver_probe_initial_timeout.tv_sec > max_probe_timeout) {
			base->global_nameserver_probe_initial_timeout.tv_sec = max_probe_timeout;
			base->global_nameserver_probe_initial_timeout.tv_usec = 0;
			log(EVDNS_LOG_DEBUG, "Setting initial probe timeout to %s",
				val);
		}
	} else if (str_matches_option(option, "probe-backoff-factor:")) {
		const int backoff_backtor = strtoint_clipped(val, 1, 10);
		if (backoff_backtor == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting probe timeout backoff factor to %d",
			backoff_backtor);
		base->ns_timeout_backoff_factor = backoff_backtor;
	} else if (str_matches_option(option, "so-rcvbuf:")) {
		int buf = strtoint(val);
		if (buf == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting SO_RCVBUF to %s", val);
		base->so_rcvbuf = buf;
	} else if (str_matches_option(option, "so-sndbuf:")) {
		int buf = strtoint(val);
		if (buf == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting SO_SNDBUF to %s", val);
		base->so_sndbuf = buf;
	} else if (str_matches_option(option, "tcp-idle-timeout:")) {
		struct timeval tv;
		if (evdns_strtotimeval(val, &tv) == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting tcp idle timeout to %s", val);
		memcpy(&base->global_tcp_idle_timeout, &tv, sizeof(tv));
	} else if (str_matches_option(option, "use-vc:")) {
		if (!(flags & DNS_OPTION_MISC)) return 0;
		if (val && strlen(val)) return -1;
		log(EVDNS_LOG_DEBUG, "Setting use-vc option");
		base->global_tcp_flags |= DNS_QUERY_USEVC;
	} else if (str_matches_option(option, "ignore-tc:")) {
		if (!(flags & DNS_OPTION_MISC)) return 0;
		if (val && strlen(val)) return -1;
		log(EVDNS_LOG_DEBUG, "Setting ignore-tc option");
		base->global_tcp_flags |= DNS_QUERY_IGNTC;
	} else if (str_matches_option(option, "edns-udp-size:")) {
		const int sz = strtoint_clipped(val, DNS_MAX_UDP_SIZE, EDNS_MAX_UDP_SIZE);
		if (sz == -1) return -1;
		if (!(flags & DNS_OPTION_MISC)) return 0;
		log(EVDNS_LOG_DEBUG, "Setting edns-udp-size to %d", sz);
		base->global_max_udp_size = sz;
	}
	return 0;
}
/* Context-After
 * int
 * evdns_set_option(const char *option, const char *val, int flags)
 * {
 * 	if (!current_base)
 * 		current_base = evdns_base_new(NULL, 0);
 * 	return evdns_base_set_option(current_base, option, val);
 * }
 * 
 * static void
 * resolv_conf_parse_line(struct evdns_base *base, char *const start, int flags) {
 * 	char *strtok_state;
 * 	static const char *const delims = " \t";
 * #define NEXT_TOKEN strtok_r(NULL, delims, &strtok_state)
 * 
 * 
 * 	char *const first_token = strtok_r(start, delims, &strtok_state);
 * 	ASSERT_LOCKED(base);
 * 	if (!first_token) return;
 */
