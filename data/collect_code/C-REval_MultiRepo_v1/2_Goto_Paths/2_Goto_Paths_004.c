/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_004 */
/* Category: 2_Goto_Paths */
/* Repo: libuv */
/* Cyclomatic Complexity: 8 */
/* NLOC: 67 */
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
 *   /* Initialize winsock */
 *   uv__winsock_init();
 * 
 *   /* Initialize FS */
 *   uv__fs_init();
 * 
 *   /* Initialize signal stuff */
 *   uv__signals_init();
 * 
 *   /* Initialize console */
 *   uv__console_init();
 * 
 *   /* Initialize utilities */
 *   uv__util_init();
 * 
 *   /* Initialize system wakeup detection */
 *   uv__init_detect_system_wakeup();
 * }
 */
int uv_loop_init(uv_loop_t* loop) {
  uv__loop_internal_fields_t* lfields;
  struct heap* timer_heap;
  int err;

  /* Initialize libuv itself first */
  uv__once_init();

  /* Create an I/O completion port */
  loop->iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  if (loop->iocp == NULL)
    return uv_translate_sys_error(GetLastError());

  lfields = (uv__loop_internal_fields_t*) uv__calloc(1, sizeof(*lfields));
  if (lfields == NULL)
    return UV_ENOMEM;
  loop->internal_fields = lfields;

  err = uv_mutex_init(&lfields->loop_metrics.lock);
  if (err)
    goto fail_metrics_mutex_init;
  memset(&lfields->loop_metrics.metrics,
         0,
         sizeof(lfields->loop_metrics.metrics));

  /* To prevent uninitialized memory access, loop->time must be initialized
   * to zero before calling uv_update_time for the first time.
   */
  loop->time = 0;
  uv_update_time(loop);

  uv__queue_init(&loop->wq);
  uv__queue_init(&loop->handle_queue);
  loop->active_reqs.count = 0;
  loop->active_handles = 0;

  loop->pending_reqs_tail = NULL;

  loop->endgame_handles = NULL;

  loop->timer_heap = timer_heap = uv__malloc(sizeof(*timer_heap));
  if (timer_heap == NULL) {
    err = UV_ENOMEM;
    goto fail_timers_alloc;
  }

  heap_init(timer_heap);

  loop->check_handles = NULL;
  loop->prepare_handles = NULL;
  loop->idle_handles = NULL;

  loop->next_prepare_handle = NULL;
  loop->next_check_handle = NULL;
  loop->next_idle_handle = NULL;

  memset(&loop->poll_peer_sockets, 0, sizeof loop->poll_peer_sockets);

  loop->timer_counter = 0;
  loop->stop_flag = 0;

  err = uv_mutex_init(&loop->wq_mutex);
  if (err)
    goto fail_mutex_init;

  err = uv_async_init(loop, &loop->wq_async, uv__work_done);
  if (err)
    goto fail_async_init;

  uv__handle_unref(&loop->wq_async);
  loop->wq_async.flags |= UV_HANDLE_INTERNAL;

  err = uv__loops_add(loop);
  if (err)
    goto fail_async_init;

  return 0;

fail_async_init:
  uv_mutex_destroy(&loop->wq_mutex);

fail_mutex_init:
  uv__free(timer_heap);
  loop->timer_heap = NULL;

fail_timers_alloc:
  uv_mutex_destroy(&lfields->loop_metrics.lock);

fail_metrics_mutex_init:
  uv__free(lfields);
  loop->internal_fields = NULL;
  CloseHandle(loop->iocp);
  loop->iocp = INVALID_HANDLE_VALUE;

  return err;
}
/* Context-After
 * void uv_update_time(uv_loop_t* loop) {
 *   uint64_t new_time = uv__hrtime(1000);
 *   assert(new_time >= loop->time);
 *   loop->time = new_time;
 * }
 * 
 * 
 * void uv__once_init(void) {
 *   uv_once(&uv_init_guard_, uv__init);
 * }
 * 
 * 
 * void uv__loop_close(uv_loop_t* loop) {
 *   uv__loop_internal_fields_t* lfields;
 *   size_t i;
 * 
 *   uv__loops_remove(loop);
 */
