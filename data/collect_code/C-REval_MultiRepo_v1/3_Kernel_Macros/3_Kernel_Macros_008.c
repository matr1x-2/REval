/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_008 */
/* Category: 3_Kernel_Macros */
/* Repo: libuv */
/* Cyclomatic Complexity: 13 */
/* NLOC: 63 */
/* Subsystem: src */
/* Includes
 * #include <assert.h>
 * #include <errno.h>
 * #include <limits.h>
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <string.h>
 * #include <crtdbg.h>
 * #include "uv.h"
 * #include "internal.h"
 * #include "queue.h"
 * #include "handle-inl.h"
 * #include "heap-inl.h"
 * #include "req-inl.h"
 */
/* Context-Before
 * }
 * 
 * 
 * int uv_loop_alive(const uv_loop_t* loop) {
 *   return uv__loop_alive(loop);
 * }
 * 
 * 
 * int uv_backend_timeout(const uv_loop_t* loop) {
 *   if (loop->stop_flag == 0 &&
 *       /* uv__loop_alive(loop) && */
 *       (uv__has_active_handles(loop) || uv__has_active_reqs(loop)) &&
 *       loop->pending_reqs_tail == NULL &&
 *       loop->idle_handles == NULL &&
 *       loop->endgame_handles == NULL)
 *     return uv__next_timeout(loop);
 *   return 0;
 * }
 */
static void uv__poll(uv_loop_t* loop, DWORD timeout) {
  uv__loop_internal_fields_t* lfields;
  BOOL success;
  uv_req_t* req;
  OVERLAPPED_ENTRY overlappeds[128];
  OVERLAPPED* overlapped;
  ULONG count;
  ULONG i;
  int repeat;
  uint64_t timeout_time;
  uint64_t user_timeout;
  uint64_t actual_timeout;
  int reset_timeout;

  lfields = uv__get_internal_fields(loop);
  timeout_time = loop->time + timeout;

  if (lfields->flags & UV_METRICS_IDLE_TIME) {
    reset_timeout = 1;
    user_timeout = timeout;
    timeout = 0;
  } else {
    reset_timeout = 0;
  }

  for (repeat = 0; ; repeat++) {
    actual_timeout = timeout;

    /* Only need to set the provider_entry_time if timeout != 0. The function
     * will return early if the loop isn't configured with UV_METRICS_IDLE_TIME.
     */
    if (timeout != 0)
      uv__metrics_set_provider_entry_time(loop);

    /* Store the current timeout in a location that's globally accessible so
     * other locations like uv__work_done() can determine whether the queue
     * of events in the callback were waiting when poll was called.
     */
    lfields->current_timeout = timeout;

    success = GetQueuedCompletionStatusEx(loop->iocp,
                                          overlappeds,
                                          ARRAY_SIZE(overlappeds),
                                          &count,
                                          timeout,
                                          FALSE);

    if (reset_timeout != 0) {
      timeout = user_timeout;
      reset_timeout = 0;
    }

    /* Placed here because on success the loop will break whether there is an
     * empty package or not, or if GetQueuedCompletionStatusEx returned early
     * then the timeout will be updated and the loop will run again. In either
     * case the idle time will need to be updated.
     */
    uv__metrics_update_idle_time(loop);

    if (success) {
      for (i = 0; i < count; i++) {
        /* Package was dequeued, but see if it is not a empty package
         * meant only to wake us up.
         */
        if (overlappeds[i].lpOverlapped) {
          uv__metrics_inc_events(loop, 1);
          if (actual_timeout == 0)
            uv__metrics_inc_events_waiting(loop, 1);

          overlapped = overlappeds[i].lpOverlapped;
          req = container_of(overlapped, uv_req_t, u.io.overlapped);
          uv__insert_pending_req(loop, req);
        }
      }

      /* Some time might have passed waiting for I/O,
       * so update the loop time here.
       */
      uv_update_time(loop);
    } else if (GetLastError() != WAIT_TIMEOUT) {
      /* Serious error */
      uv_fatal_error(GetLastError(), "GetQueuedCompletionStatusEx");
    } else if (timeout > 0) {
      /* GetQueuedCompletionStatus can occasionally return a little early.
       * Make sure that the desired timeout target time is reached.
       */
      uv_update_time(loop);
      if (timeout_time > loop->time) {
        timeout = (DWORD)(timeout_time - loop->time);
        /* The first call to GetQueuedCompletionStatus should return very
         * close to the target time and the second should reach it, but
         * this is not stated in the documentation. To make sure a busy
         * loop cannot happen, the timeout is increased exponentially
         * starting on the third round.
         */
        timeout += repeat ? (1 << (repeat - 1)) : 0;
        continue;
      }
    }
    break;
  }
}
/* Context-After
 * #define DELEGATE_STREAM_REQ(loop, req, method, handle_at)                     \
 *   do {                                                                        \
 *     switch (((uv_handle_t*) (req)->handle_at)->type) {                        \
 *       case UV_TCP:                                                            \
 *         uv__process_tcp_##method##_req(loop,                                  \
 *                                       (uv_tcp_t*) ((req)->handle_at),         \
 *                                       req);                                   \
 *         break;                                                                \
 *                                                                               \
 *       case UV_NAMED_PIPE:                                                     \
 *         uv__process_pipe_##method##_req(loop,                                 \
 *                                        (uv_pipe_t*) ((req)->handle_at),       \
 *                                        req);                                  \
 *         break;                                                                \
 *                                                                               \
 *       case UV_TTY:                                                            \
 *         uv__process_tty_##method##_req(loop,                                  \
 *                                       (uv_tty_t*) ((req)->handle_at),         \
 */
