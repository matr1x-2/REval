/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_018 */
/* Category: 2_Goto_Paths */
/* Repo: libuv */
/* Cyclomatic Complexity: 8 */
/* NLOC: 33 */
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
 *     new_loops = uv__realloc(uv__loops, sizeof(uv_loop_t*) * new_capacity);
 *     if (!new_loops)
 *       goto failed_loops_realloc;
 *     uv__loops = new_loops;
 *     for (i = uv__loops_capacity; i < new_capacity; ++i)
 *       uv__loops[i] = NULL;
 *     uv__loops_capacity = new_capacity;
 *   }
 *   uv__loops[uv__loops_size] = loop;
 *   ++uv__loops_size;
 * 
 *   uv_mutex_unlock(&uv__loops_lock);
 *   return 0;
 * 
 * failed_loops_realloc:
 *   uv_mutex_unlock(&uv__loops_lock);
 *   return UV_ENOMEM;
 * }
 */
static void uv__loops_remove(uv_loop_t* loop) {
  int loop_index;
  int smaller_capacity;
  uv_loop_t** new_loops;

  uv_mutex_lock(&uv__loops_lock);

  for (loop_index = 0; loop_index < uv__loops_size; ++loop_index) {
    if (uv__loops[loop_index] == loop)
      break;
  }
  /* If loop was not found, ignore */
  if (loop_index == uv__loops_size)
    goto loop_removed;

  uv__loops[loop_index] = uv__loops[uv__loops_size - 1];
  uv__loops[uv__loops_size - 1] = NULL;
  --uv__loops_size;

  if (uv__loops_size == 0) {
    uv__loops_capacity = 0;
    uv__free(uv__loops);
    uv__loops = NULL;
    goto loop_removed;
  }

  /* If we didn't grow to big skip downsizing */
  if (uv__loops_capacity < 4 * UV__LOOPS_CHUNK_SIZE)
    goto loop_removed;

  /* Downsize only if more than half of buffer is free */
  smaller_capacity = uv__loops_capacity / 2;
  if (uv__loops_size >= smaller_capacity)
    goto loop_removed;
  new_loops = uv__realloc(uv__loops, sizeof(uv_loop_t*) * smaller_capacity);
  if (!new_loops)
    goto loop_removed;
  uv__loops = new_loops;
  uv__loops_capacity = smaller_capacity;

loop_removed:
  uv_mutex_unlock(&uv__loops_lock);
}
/* Context-After
 * void uv__wake_all_loops(void) {
 *   int i;
 *   uv_loop_t* loop;
 * 
 *   uv_mutex_lock(&uv__loops_lock);
 *   for (i = 0; i < uv__loops_size; ++i) {
 *     loop = uv__loops[i];
 *     assert(loop);
 *     if (loop->iocp != INVALID_HANDLE_VALUE)
 *       PostQueuedCompletionStatus(loop->iocp, 0, 0, NULL);
 *   }
 *   uv_mutex_unlock(&uv__loops_lock);
 * }
 * 
 * static void uv__init(void) {
 *   /* Tell Windows that we will handle critical errors. */
 *   SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX |
 *                SEM_NOOPENFILEERRORBOX);
 */
