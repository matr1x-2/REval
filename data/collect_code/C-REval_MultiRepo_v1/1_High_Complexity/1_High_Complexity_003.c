/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_003 */
/* Category: 1_High_Complexity */
/* Repo: libevent */
/* Cyclomatic Complexity: 37 */
/* NLOC: 194 */
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
 * 	evutil_gettimeofday(&timer_bias_start, NULL);
 * 	event_base_dispatch(base);
 * 	done = 1;
 * 
 * err:
 * 	if (cfg)
 * 		event_config_free(cfg);
 * 	if (timer)
 * 		event_free(timer);
 * 	if (base)
 * 		event_base_free(base);
 * 
 * 	if (done)
 * 		return MIN(timer_bias_spend / 1e6 / timer_bias_events / TIMER_MAX_COST_USEC, 5);
 * 
 * 	fprintf(stderr, "Couldn't create event for CPU cycle counter bias\n");
 * 	return -1;
 * }
 * 
 * static int
 */
test_ratelimiting(void)
{
	struct event_base *base;
	struct sockaddr_in sin;
	struct evconnlistener *listener;

	struct sockaddr_storage ss;
	ev_socklen_t slen;

	int i;

	struct timeval tv;

	ev_uint64_t total_received;
	double total_sq_persec, total_persec;
	double variance;
	double expected_total_persec = -1.0, expected_avg_persec = -1.0;
	int ok = 1;
	struct event_config *base_cfg;
	struct event *periodic_level_check;
	struct event *group_drain_event=NULL;
	double timer_bias;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0x7f000001); /* 127.0.0.1 */
	sin.sin_port = 0; /* unspecified port */

	if (0)
		event_enable_debug_mode();

	timer_bias = timer_bias_calculate();
	if (timer_bias > 1) {
		fprintf(stderr, "CPU is slow, timers bias is %f\n", timer_bias);
		cfg_connlimit_tolerance  *= timer_bias;
		cfg_grouplimit_tolerance *= timer_bias;
		cfg_stddev_tolerance     *= timer_bias;
	} else {
		printf("CPU is fast enough, timers bias is %f\n", timer_bias);
	}

	base_cfg = event_config_new();

#ifdef _WIN32
	if (cfg_enable_iocp) {
#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
		evthread_use_windows_threads();
#endif
		event_config_set_flag(base_cfg, EVENT_BASE_FLAG_STARTUP_IOCP);
	}
#endif

	base = event_base_new_with_config(base_cfg);
	event_config_free(base_cfg);
	if (! base) {
		fprintf(stderr, "Couldn't create event_base");
		return 1;
	}

	listener = evconnlistener_new_bind(base, echo_listenercb, base,
	    LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
	    (struct sockaddr *)&sin, sizeof(sin));
	if (! listener) {
		fprintf(stderr, "Couldn't create listener");
		return 1;
	}

	slen = sizeof(ss);
	if (getsockname(evconnlistener_get_fd(listener), (struct sockaddr *)&ss,
		&slen) < 0) {
		perror("getsockname");
		return 1;
	}

	if (cfg_connlimit > 0) {
		conn_bucket_cfg = ev_token_bucket_cfg_new(
			cfg_connlimit, cfg_connlimit * 4,
			cfg_connlimit, cfg_connlimit * 4,
			&cfg_tick);
		assert(conn_bucket_cfg);
	}

	if (cfg_grouplimit > 0) {
		group_bucket_cfg = ev_token_bucket_cfg_new(
			cfg_grouplimit, cfg_grouplimit * 4,
			cfg_grouplimit, cfg_grouplimit * 4,
			&cfg_tick);
		group = ratelim_group = bufferevent_rate_limit_group_new(
			base, group_bucket_cfg);
		expected_total_persec = cfg_grouplimit - (cfg_group_drain / seconds_per_tick);
		expected_avg_persec = cfg_grouplimit / cfg_n_connections;
		if (cfg_connlimit > 0 && expected_avg_persec > cfg_connlimit)
			expected_avg_persec = cfg_connlimit;
		if (cfg_min_share >= 0)
			bufferevent_rate_limit_group_set_min_share(
				ratelim_group, cfg_min_share);
	}

	if (expected_avg_persec < 0 && cfg_connlimit > 0)
		expected_avg_persec = cfg_connlimit;

	if (expected_avg_persec > 0)
		expected_avg_persec /= seconds_per_tick;
	if (expected_total_persec > 0)
		expected_total_persec /= seconds_per_tick;

	bevs = calloc(cfg_n_connections, sizeof(struct bufferevent *));
	states = calloc(cfg_n_connections, sizeof(struct client_state));

	for (i = 0; i < cfg_n_connections; ++i) {
		bevs[i] = bufferevent_socket_new(base, -1,
		    BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
		assert(bevs[i]);
		bufferevent_setcb(bevs[i], discard_readcb, loud_writecb,
		    write_on_connectedcb, &states[i]);
		bufferevent_enable(bevs[i], EV_READ|EV_WRITE);
		bufferevent_socket_connect(bevs[i], (struct sockaddr *)&ss,
		    slen);
	}

	tv.tv_sec = cfg_duration - 1;
	tv.tv_usec = 995000;

	event_base_loopexit(base, &tv);

	tv.tv_sec = 0;
	tv.tv_usec = 100*1000;
	ms100_common = event_base_init_common_timeout(base, &tv);

	periodic_level_check = event_new(base, -1, EV_PERSIST, check_group_bucket_levels_cb, NULL);
	event_add(periodic_level_check, ms100_common);

	if (cfg_group_drain && ratelim_group) {
		group_drain_event = event_new(base, -1, EV_PERSIST, group_drain_cb, NULL);
		event_add(group_drain_event, &cfg_tick);
	}

	event_base_dispatch(base);

	ratelim_group = NULL; /* So no more responders get added */
	event_free(periodic_level_check);
	if (group_drain_event)
		event_free(group_drain_event);

	for (i = 0; i < cfg_n_connections; ++i) {
		bufferevent_free(bevs[i]);
	}
	evconnlistener_free(listener);

	/* Make sure no new echo_conns get added to the group. */
	ratelim_group = NULL;

	/* This should get _everybody_ freed */
	while (n_echo_conns_open) {
		printf("waiting for %d conns\n", n_echo_conns_open);
		tv.tv_sec = 0;
		tv.tv_usec = 300000;
		event_base_loopexit(base, &tv);
		event_base_dispatch(base);
	}

	if (group)
		bufferevent_rate_limit_group_free(group);

	if (total_n_bev_checks) {
		printf("Average read bucket level: %f\n",
		    (double)total_rbucket_level/total_n_bev_checks);
		printf("Average write bucket level: %f\n",
		    (double)total_wbucket_level/total_n_bev_checks);
		printf("Highest read bucket level: %f\n",
		    (double)max_bucket_level);
		printf("Highest write bucket level: %f\n",
		    (double)min_bucket_level);
		printf("Average max-to-read: %f\n",
		    ((double)total_max_to_read)/total_n_bev_checks);
		printf("Average max-to-write: %f\n",
		    ((double)total_max_to_write)/total_n_bev_checks);
	}
	if (total_n_group_bev_checks) {
		printf("Average group read bucket level: %f\n",
		    ((double)total_group_rbucket_level)/total_n_group_bev_checks);
		printf("Average group write bucket level: %f\n",
		    ((double)total_group_wbucket_level)/total_n_group_bev_checks);
	}

	total_received = 0;
	total_persec = 0.0;
	total_sq_persec = 0.0;
	for (i=0; i < cfg_n_connections; ++i) {
		double persec = states[i].received;
		persec /= cfg_duration;
		total_received += states[i].received;
		total_persec += persec;
		total_sq_persec += persec*persec;
		printf("%d: %f per second\n", i+1, persec);
	}
	printf("   total: %f per second\n",
	    ((double)total_received)/cfg_duration);
	if (expected_total_persec > 0) {
		double diff = expected_total_persec -
		    ((double)total_received/cfg_duration);
		printf("  [Off by %lf]\n", diff);
		if (cfg_grouplimit_tolerance > 0 &&
		    fabs(diff) > cfg_grouplimit_tolerance) {
			fprintf(stderr, "Group bandwidth out of bounds\n");
			ok = 0;
		}
	}

	printf(" average: %f per second\n",
	    (((double)total_received)/cfg_duration)/cfg_n_connections);
	if (expected_avg_persec > 0) {
		double diff = expected_avg_persec - (((double)total_received)/cfg_duration)/cfg_n_connections;
		printf("  [Off by %lf]\n", diff);
		if (cfg_connlimit_tolerance > 0 &&
		    fabs(diff) > cfg_connlimit_tolerance) {
			fprintf(stderr, "Connection bandwidth out of bounds\n");
			ok = 0;
		}
	}

	variance = total_sq_persec/cfg_n_connections - total_persec*total_persec/(cfg_n_connections*cfg_n_connections);

	printf("  stddev: %f per second\n", sqrt(variance));
	if (cfg_stddev_tolerance > 0 &&
	    sqrt(variance) > cfg_stddev_tolerance) {
		fprintf(stderr, "Connection variance out of bounds\n");
		ok = 0;
	}

	event_base_free(base);
	free(bevs);
	free(states);

	return ok ? 0 : 1;
}
/* Context-After
 * static struct option {
 * 	const char *name; int *ptr; int min; int isbool;
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
 */
