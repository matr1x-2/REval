/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_013 */
/* Category: 4_Multi_Pointers */
/* Repo: libevent */
/* Cyclomatic Complexity: 18 */
/* NLOC: 66 */
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
 * 	u16 = (uint16_t *)&fr[2];
 * 	*u16 = htons((int16_t)reason);
 * 	output = bufferevent_get_output(evws->bufev);
 * 	evbuffer_add(output, fr, 4);
 * 
 * 	/* wait for close frame writing complete and close connection */
 * 	bufferevent_setcb(
 * 		evws->bufev, NULL, close_after_write_cb, close_event_cb, evws);
 * }
 * 
 * static void
 * evws_force_disconnect_(struct evws_connection *evws)
 * {
 * 	evws_close(evws, WS_CR_NONE);
 * }
 * 
 * /* parse base frame according to
 *  * https://www.rfc-editor.org/rfc/rfc6455#section-5.2
 *  */
 * static enum WebSocketFrameType
 */
get_ws_frame(unsigned char *in_buffer, size_t buf_len,
	unsigned char **payload_ptr, size_t *out_len)
{
	unsigned char opcode;
	unsigned char fin;
	unsigned char masked;
	size_t payload_len;
	size_t pos;
	int length_field;

	if (buf_len < 2) {
		return INCOMPLETE_DATA;
	}

	opcode = in_buffer[0] & 0x0F;
	fin = (in_buffer[0] >> 7) & 0x01;
	masked = (in_buffer[1] >> 7) & 0x01;

	payload_len = 0;
	pos = 2;
	length_field = in_buffer[1] & (~0x80);

	if (length_field <= 125) {
		payload_len = length_field;
	} else if (length_field == 126) { /* msglen is 16bit */
		uint16_t tmp16;
		if (buf_len < 4)
			return INCOMPLETE_DATA;
		memcpy(&tmp16, in_buffer + pos, 2);
		payload_len = ntohs(tmp16);
		pos += 2;
	} else if (length_field == 127) { /* msglen is 64bit */
		int i;
		uint64_t tmp64 = 0;
		if (buf_len < 10)
			return INCOMPLETE_DATA;
		/* swap bytes from big endian to host byte order */
		for (i = 56; i >= 0; i -= 8) {
			tmp64 |= (uint64_t)in_buffer[pos++] << i;
		}
		if (tmp64 > WS_MAX_RECV_FRAME_SZ) {
			/* Implementation limitation, we support up to 10 MiB
			 * length, as a DoS prevention measure.
			 */
			event_warn("%s: frame length %" PRIu64 " exceeds %" PRIu64 "\n",
				__func__, tmp64, (uint64_t)WS_MAX_RECV_FRAME_SZ);
			/* Calling code needs these values; do the best we can here.
			 * Caller will close the connection anyway.
			 */
			*payload_ptr = in_buffer + pos;
			*out_len = 0;
			return ERROR_FRAME;
		}
		payload_len = (size_t)tmp64;
	}
	if (buf_len < payload_len + pos + (masked ? 4u : 0u)) {
		return INCOMPLETE_DATA;
	}

	/* According to RFC it seems that unmasked data should be prohibited
	 * but we support it for nonconformant clients
	 */
	if (masked) {
		unsigned char *c, *mask;
		size_t i;

		mask = in_buffer + pos; /* first 4 bytes are mask bytes */
		pos += 4;

		/* unmask data */
		c = in_buffer + pos;
		for (i = 0; i < payload_len; i++) {
			c[i] = c[i] ^ mask[i % 4u];
		}
	}

	*payload_ptr = in_buffer + pos;
	*out_len = payload_len;

	/* are reserved for further frames */
	if ((opcode >= 3 && opcode <= 7) || (opcode >= 0xb))
		return ERROR_FRAME;

	if (opcode <= 0x3 && !fin) {
		return INCOMPLETE_FRAME;
	}
	return opcode;
}
/* Context-After
 * static void
 * ws_evhttp_read_cb(struct bufferevent *bufev, void *arg)
 * {
 * 	struct evws_connection *evws = arg;
 * 	unsigned char *payload;
 * 	enum WebSocketFrameType type;
 * 	size_t msg_len, in_len, header_sz;
 * 	struct evbuffer *input = bufferevent_get_input(evws->bufev);
 * 
 * 	bufferevent_incref_and_lock_(evws->bufev);
 * 	while ((in_len = evbuffer_get_length(input))) {
 * 		unsigned char *data = evbuffer_pullup(input, in_len);
 * 		if (data == NULL) {
 * 			goto bailout;
 * 		}
 * 
 * 		type = get_ws_frame(data, in_len, &payload, &msg_len);
 * 		if (type == INCOMPLETE_DATA) {
 */
