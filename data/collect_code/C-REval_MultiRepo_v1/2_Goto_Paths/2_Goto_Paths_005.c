/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_005 */
/* Category: 2_Goto_Paths */
/* Repo: libevent */
/* Cyclomatic Complexity: 12 */
/* NLOC: 53 */
/* Subsystem: ws.c */
/* Includes
 * #include "event2/event-config.h"
 * #include "evconfig-private.h"
 * #include "event2/buffer.h"
 * #include "event2/bufferevent.h"
 * #include "event2/event.h"
 * #include "event2/http.h"
 * #include "event2/ws.h"
 * #include "util-internal.h"
 * #include "mm-internal.h"
 * #include "sha1.h"
 * #include "event2/bufferevent.h"
 * #include "sys/queue.h"
 * #include "http-internal.h"
 * #include "bufferevent-internal.h"
 * #include <assert.h>
 * #include <inttypes.h>
 * #include <string.h>
 * #include <stdbool.h>
 * #include <sys/socket.h>
 * #include <sys/stat.h>
 */
/* Context-Before
 * 			event_warn("%s: unexpected frame type %d\n", __func__, type);
 * 			evws_force_disconnect_(evws);
 * 		}
 * 		evbuffer_drain(input, msg_len);
 * 	}
 * 
 * bailout:
 * 	bufferevent_decref_and_unlock_(evws->bufev);
 * }
 * 
 * static void
 * ws_evhttp_error_cb(struct bufferevent *bufev, short what, void *arg)
 * {
 * 	/* when client just disappears after connection (wscat closed by Cmd+Q) */
 * 	if (what & BEV_EVENT_EOF) {
 * 		close_after_write_cb(bufev, arg);
 * 	}
 * }
 * 
 * struct evws_connection *
 */
evws_new_session(
	struct evhttp_request *req, ws_on_msg_cb cb, void *arg, int options)
{
	struct evws_connection *evws = NULL;
	struct evkeyvalq *in_hdrs;
	const char *upgrade, *connection, *ws_key, *ws_protocol;
	struct evkeyvalq *out_hdrs;
	struct evhttp_connection *evcon;

	in_hdrs = evhttp_request_get_input_headers(req);
	upgrade = evhttp_find_header(in_hdrs, "Upgrade");
	if (upgrade == NULL || evutil_ascii_strcasecmp(upgrade, "websocket"))
		goto error;

	connection = evhttp_find_header(in_hdrs, "Connection");
	if (connection == NULL || evutil_ascii_strcasestr(connection, "Upgrade") == NULL)
		goto error;

	ws_key = evhttp_find_header(in_hdrs, "Sec-WebSocket-Key");
	if (ws_key == NULL)
		goto error;

	out_hdrs = evhttp_request_get_output_headers(req);
	evhttp_add_header(out_hdrs, "Upgrade", "websocket");
	evhttp_add_header(out_hdrs, "Connection", "Upgrade");

	evhttp_add_header(out_hdrs, "Sec-WebSocket-Accept",
		ws_gen_accept_key(ws_key, (char[32]){0}));

	ws_protocol = evhttp_find_header(in_hdrs, "Sec-WebSocket-Protocol");
	if (ws_protocol != NULL)
		evhttp_add_header(out_hdrs, "Sec-WebSocket-Protocol", ws_protocol);

	if ((evws = mm_calloc(1, sizeof(struct evws_connection))) == NULL) {
		event_warn("%s: calloc failed", __func__);
		goto error;
	}

	evws->cb = cb;
	evws->cb_arg = arg;

	evcon = evhttp_request_get_connection(req);
	evws->http_server = evcon->http_server;

	evws->bufev = evhttp_start_ws_(req);
	if (evws->bufev == NULL) {
		goto error;
	}

	if (options & BEV_OPT_THREADSAFE) {
		if (bufferevent_enable_locking_(evws->bufev, NULL) < 0)
			goto error;
	}

	bufferevent_setcb(
		evws->bufev, ws_evhttp_read_cb, NULL, ws_evhttp_error_cb, evws);

	TAILQ_INSERT_TAIL(&evws->http_server->ws_sessions, evws, next);
	evws->http_server->connection_cnt++;

	return evws;

error:
	if (evws)
		evws_connection_free(evws);

	evhttp_send_reply(req, HTTP_BADREQUEST, NULL, NULL);
	return NULL;
}
/* Context-After
 * static void
 * make_ws_frame(struct evbuffer *output, enum WebSocketFrameType frame_type,
 * 	unsigned char *msg, size_t len)
 * {
 * 	size_t pos = 0;
 * 	unsigned char header[16] = {0};
 * 
 * 	header[pos++] = (unsigned char)frame_type | 0x80; /* fin */
 * 	if (len <= 125) {
 * 		header[pos++] = len;
 * 	} else if (len <= 65535) {
 * 		header[pos++] = 126;			   /* 16 bit length */
 * 		header[pos++] = (len >> 8) & 0xFF; /* rightmost first */
 * 		header[pos++] = len & 0xFF;
 * 	} else {				 /* >2^16-1 */
 * 		int i;
 * 		const uint64_t tmp64 = len;
 * 		header[pos++] = 127;            /* 64 bit length */
 * 		/* swap bytes from host byte order to big endian */
 */
