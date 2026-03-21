/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_018 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 33 */
/* NLOC: 102 */
/* Subsystem: tools */
/* Includes
 * #include <errno.h>
 * #include <limits.h>
 * #include <fcntl.h>
 * #include <string.h>
 * #include <stdarg.h>
 * #include <stdbool.h>
 * #include <stdint.h>
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <strings.h>
 * #include <signal.h>
 * #include <unistd.h>
 * #include <time.h>
 * #include <sys/ioctl.h>
 * #include <sys/poll.h>
 * #include <sys/random.h>
 * #include <sys/sendfile.h>
 * #include <sys/stat.h>
 * #include <sys/socket.h>
 * #include <sys/types.h>
 */
/* Context-Before
 * 		return;
 * 
 * 	if (nonblock)
 * 		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
 * 	else
 * 		fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
 * }
 * 
 * static void shut_wr(int fd)
 * {
 * 	/* Close our write side, ev. give some time
 * 	 * for address notification and/or checking
 * 	 * the current status
 * 	 */
 * 	if (cfg_wait)
 * 		usleep(cfg_wait);
 * 
 * 	shutdown(fd, SHUT_WR);
 * }
 */
static int copyfd_io_poll(int infd, int peerfd, int outfd,
			  bool *in_closed_after_out, struct wstate *winfo)
{
	struct pollfd fds = {
		.fd = peerfd,
		.events = POLLIN | POLLOUT,
	};
	unsigned int total_wlen = 0, total_rlen = 0;

	set_nonblock(peerfd, true);

	for (;;) {
		char rbuf[8192];
		ssize_t len;

		if (fds.events == 0 || quit)
			break;

		switch (poll(&fds, 1, poll_timeout)) {
		case -1:
			if (errno == EINTR)
				continue;
			perror("poll");
			return 1;
		case 0:
			fprintf(stderr, "%s: poll timed out (events: "
				"POLLIN %u, POLLOUT %u)\n", __func__,
				fds.events & POLLIN, fds.events & POLLOUT);
			return 2;
		}

		if (fds.revents & POLLIN) {
			ssize_t rb = sizeof(rbuf);

			/* limit the total amount of read data to the trunc value*/
			if (cfg_truncate > 0) {
				if (rb + total_rlen > cfg_truncate)
					rb = cfg_truncate - total_rlen;
				len = read(peerfd, rbuf, rb);
			} else {
				len = do_rnd_read(peerfd, rbuf, sizeof(rbuf));
			}
			if (len == 0) {
				/* no more data to receive:
				 * peer has closed its write side
				 */
				fds.events &= ~POLLIN;

				if ((fds.events & POLLOUT) == 0) {
					*in_closed_after_out = true;
					/* and nothing more to send */
					break;
				}

			/* Else, still have data to transmit */
			} else if (len < 0) {
				if (cfg_rcv_trunc)
					return 0;
				perror("read");
				return 3;
			}

			total_rlen += len;
			do_write(outfd, rbuf, len);
		}

		if (fds.revents & POLLOUT) {
			if (winfo->len == 0) {
				winfo->off = 0;
				winfo->len = read(infd, winfo->buf, sizeof(winfo->buf));
			}

			if (winfo->len > 0) {
				ssize_t bw;

				/* limit the total amount of written data to the trunc value */
				if (cfg_truncate > 0 && winfo->len + total_wlen > cfg_truncate)
					winfo->len = cfg_truncate - total_wlen;

				bw = do_rnd_write(peerfd, winfo->buf + winfo->off, winfo->len);
				if (bw < 0) {
					/* expected reset, continue to read */
					if (cfg_rcv_trunc &&
					    (errno == ECONNRESET ||
					     errno == EPIPE)) {
						fds.events &= ~POLLOUT;
						continue;
					}

					perror("write");
					return 111;
				}

				winfo->off += bw;
				winfo->len -= bw;
				total_wlen += bw;
			} else if (winfo->len == 0) {
				/* We have no more data to send. */
				fds.events &= ~POLLOUT;

				if ((fds.events & POLLIN) == 0)
					/* ... and peer also closed already */
					break;

				shut_wr(peerfd);
			} else {
				if (errno == EINTR)
					continue;
				perror("read");
				return 4;
			}
		}

		if (fds.revents & (POLLERR | POLLNVAL)) {
			if (cfg_rcv_trunc) {
				fds.events &= ~(POLLERR | POLLNVAL);
				continue;
			}
			fprintf(stderr, "Unexpected revents: "
				"POLLERR/POLLNVAL(%x)\n", fds.revents);
			return 5;
		}

		if (cfg_truncate > 0 && total_wlen >= cfg_truncate &&
		    total_rlen >= cfg_truncate)
			break;
	}

	/* leave some time for late join/announce */
	if (cfg_remove && !quit)
		usleep(cfg_wait);

	return 0;
}
/* Context-After
 * static int do_recvfile(int infd, int outfd)
 * {
 * 	ssize_t r;
 * 
 * 	do {
 * 		char buf[16384];
 * 
 * 		r = do_rnd_read(infd, buf, sizeof(buf));
 * 		if (r > 0) {
 * 			if (write(outfd, buf, r) != r)
 * 				break;
 * 		} else if (r < 0) {
 * 			perror("read");
 * 		}
 * 	} while (r > 0);
 * 
 * 	return (int)r;
 * }
 */
