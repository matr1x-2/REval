/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_004 */
/* Category: 4_Multi_Pointers */
/* Repo: jemalloc */
/* Cyclomatic Complexity: 7 */
/* NLOC: 36 */
/* Subsystem: test */
/* Includes
 * #include "test/jemalloc_test.h"
 * #include "jemalloc/internal/prof_data.h"
 * #include "jemalloc/internal/prof_sys.h"
 */
/* Context-Before
 * 	expect_d_eq(mallctl("prof.reset", NULL, NULL, NULL, 0), 0,
 * 	    "Unexpected error while resetting heap profile data");
 * 	prof_cnt_all(&cnt_all);
 * 	expect_u64_eq(cnt_all.curobjs, 0, "Expected 0 allocations");
 * 	expect_zu_eq(prof_bt_count(), 1, "Expected 1 backtrace");
 * 
 * 	dallocx(p, 0);
 * 	expect_zu_eq(prof_bt_count(), 0, "Expected 0 backtraces");
 * 
 * 	set_prof_active(false);
 * }
 * TEST_END
 * 
 * #define NTHREADS 4
 * #define NALLOCS_PER_THREAD (1U << 13)
 * #define OBJ_RING_BUF_COUNT 1531
 * #define RESET_INTERVAL (1U << 10)
 * #define DUMP_INTERVAL 3677
 * static void *
 */
thd_start(void *varg) {
	unsigned thd_ind = *(unsigned *)varg;
	unsigned i;
	void    *objs[OBJ_RING_BUF_COUNT];

	memset(objs, 0, sizeof(objs));

	for (i = 0; i < NALLOCS_PER_THREAD; i++) {
		if (i % RESET_INTERVAL == 0) {
			expect_d_eq(mallctl("prof.reset", NULL, NULL, NULL, 0),
			    0,
			    "Unexpected error while resetting heap profile "
			    "data");
		}

		if (i % DUMP_INTERVAL == 0) {
			expect_d_eq(mallctl("prof.dump", NULL, NULL, NULL, 0),
			    0, "Unexpected error while dumping heap profile");
		}

		{
			void **pp = &objs[i % OBJ_RING_BUF_COUNT];
			if (*pp != NULL) {
				dallocx(*pp, 0);
				*pp = NULL;
			}
			*pp = btalloc(1, thd_ind * NALLOCS_PER_THREAD + i);
			expect_ptr_not_null(
			    *pp, "Unexpected btalloc() failure");
		}
	}

	/* Clean up any remaining objects. */
	for (i = 0; i < OBJ_RING_BUF_COUNT; i++) {
		void **pp = &objs[i % OBJ_RING_BUF_COUNT];
		if (*pp != NULL) {
			dallocx(*pp, 0);
			*pp = NULL;
		}
	}

	return NULL;
}
/* Context-After
 * TEST_BEGIN(test_prof_reset) {
 * 	size_t   lg_prof_sample_orig;
 * 	thd_t    thds[NTHREADS];
 * 	unsigned thd_args[NTHREADS];
 * 	unsigned i;
 * 	size_t   bt_count, tdata_count;
 * 
 * 	test_skip_if(!config_prof);
 * 
 * 	bt_count = prof_bt_count();
 * 	expect_zu_eq(bt_count, 0, "Unexpected pre-existing tdata structures");
 * 	tdata_count = prof_tdata_count();
 * 
 * 	lg_prof_sample_orig = get_lg_prof_sample();
 * 	do_prof_reset(5);
 * 
 * 	set_prof_active(true);
 * 
 * 	for (i = 0; i < NTHREADS; i++) {
 */
