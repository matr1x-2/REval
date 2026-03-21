/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_018 */
/* Category: 4_Multi_Pointers */
/* Repo: libevent */
/* Cyclomatic Complexity: 12 */
/* NLOC: 67 */
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
 * 	return evdns_add_server_port_with_base(NULL, socket, flags, cb, user_data);
 * }
 * 
 * /* exported function */
 * void
 * evdns_close_server_port(struct evdns_server_port *port)
 * {
 * 	EVDNS_LOCK(port);
 * 	evdns_remove_all_tcp_clients(port);
 * 	if (--port->refcnt == 0) {
 * 		EVDNS_UNLOCK(port);
 * 		server_port_free(port);
 * 	} else {
 * 		port->closing = 1;
 * 		EVDNS_UNLOCK(port);
 * 	}
 * }
 * 
 * /* exported function */
 * int
 */
evdns_server_request_add_reply(struct evdns_server_request *req_, int section, const char *name, int type, int class, int ttl, int datalen, int is_name, const char *data)
{
	struct server_request *req = TO_SERVER_REQUEST(req_);
	struct server_reply_item **itemp, *item;
	int *countp;
	int result = -1;

	EVDNS_LOCK(req->port);
	if (req->response) /* have we already answered? */
		goto done;

	switch (section) {
	case EVDNS_ANSWER_SECTION:
		itemp = &req->answer;
		countp = &req->n_answer;
		break;
	case EVDNS_AUTHORITY_SECTION:
		itemp = &req->authority;
		countp = &req->n_authority;
		break;
	case EVDNS_ADDITIONAL_SECTION:
		itemp = &req->additional;
		countp = &req->n_additional;
		break;
	default:
		goto done;
	}
	while (*itemp) {
		itemp = &((*itemp)->next);
	}
	item = mm_malloc(sizeof(struct server_reply_item));
	if (!item)
		goto done;
	item->next = NULL;
	if (!(item->name = mm_strdup(name))) {
		mm_free(item);
		goto done;
	}
	item->type = type;
	item->dns_question_class = class;
	item->ttl = ttl;
	item->is_name = is_name != 0;
	item->datalen = 0;
	item->data = NULL;
	if (data) {
		if (item->is_name) {
			if (!(item->data = mm_strdup(data))) {
				mm_free(item->name);
				mm_free(item);
				goto done;
			}
			item->datalen = (u16)-1;
		} else {
			if (!(item->data = mm_malloc(datalen))) {
				mm_free(item->name);
				mm_free(item);
				goto done;
			}
			item->datalen = datalen;
			memcpy(item->data, data, datalen);
		}
	}

	*itemp = item;
	++(*countp);
	result = 0;
done:
	EVDNS_UNLOCK(req->port);
	return result;
}
/* Context-After
 * /* exported function */
 * int
 * evdns_server_request_add_a_reply(struct evdns_server_request *req, const char *name, int n, const void *addrs, int ttl)
 * {
 * 	return evdns_server_request_add_reply(
 * 		  req, EVDNS_ANSWER_SECTION, name, TYPE_A, CLASS_INET,
 * 		  ttl, n*4, 0, addrs);
 * }
 * 
 * /* exported function */
 * int
 * evdns_server_request_add_aaaa_reply(struct evdns_server_request *req, const char *name, int n, const void *addrs, int ttl)
 * {
 * 	return evdns_server_request_add_reply(
 * 		  req, EVDNS_ANSWER_SECTION, name, TYPE_AAAA, CLASS_INET,
 * 		  ttl, n*16, 0, addrs);
 * }
 * 
 * /* exported function */
 */
