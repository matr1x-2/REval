/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_019 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 19 */
/* Subsystem: scripts */
/* Includes
 * #include <sys/types.h>
 * #include <sys/stat.h>
 * #include <getopt.h>
 * #include <fcntl.h>
 * #include <stdio.h>
 * #include <stdlib.h>
 * #include <stdbool.h>
 * #include <string.h>
 * #include <unistd.h>
 * #include <errno.h>
 * #include <pthread.h>
 * #include "elf-parse.h"
 */
/* Context-Before
 * }
 * 
 * /**
 *  * for_each_shdr_str - iterator that reads strings that are in an ELF section.
 *  * @len: "int" to hold the length of the current string
 *  * @ehdr: A pointer to the ehdr of the ELF file
 *  * @sec: The section that has the strings to iterate on
 *  *
 *  * This is a for loop that iterates over all the nul terminated strings
 *  * that are in a given ELF section. The variable "str" will hold
 *  * the current string for each iteration and the passed in @len will
 *  * contain the strlen() of that string.
 *  */
 * #define for_each_shdr_str(len, ehdr, sec)				\
 * 	for (const char *str = (void *)(ehdr) + shdr_offset(sec),	\
 * 			*end = str + shdr_size(sec);			\
 * 	     len = strlen(str), str < end;				\
 * 	     str += (len) + 1)
 */
static void make_trace_array(struct elf_tracepoint *etrace)
{
	Elf_Ehdr *ehdr = etrace->ehdr;
	const char **vals = NULL;
	int count = 0;
	int len;

	etrace->array = NULL;

	/*
	 * The __tracepoint_check section is filled with strings of the
	 * names of tracepoints (in tracepoint_strings). Create an array
	 * that points to each string and then sort the array.
	 */
	for_each_shdr_str(len, ehdr, check_data_sec) {
		if (!len)
			continue;
		if (add_string(str, &vals, &count) < 0)
			return;
	}

	/* If CONFIG_TRACEPOINT_VERIFY_USED is not set, there's nothing to do */
	if (!count)
		return;

	qsort(vals, count, sizeof(char *), compare_strings);

	etrace->array = vals;
	etrace->count = count;
}
/* Context-After
 * static int find_event(const char *str, void *array, size_t size)
 * {
 * 	return bsearch(&str, array, size, sizeof(char *), compare_strings) != NULL;
 * }
 * 
 * static void check_tracepoints(struct elf_tracepoint *etrace, const char *fname)
 * {
 * 	Elf_Ehdr *ehdr = etrace->ehdr;
 * 	int len;
 * 
 * 	if (!etrace->array)
 * 		return;
 * 
 * 	/*
 * 	 * The __tracepoints_strings section holds all the names of the
 * 	 * defined tracepoints. If any of them are not in the
 * 	 * __tracepoint_check_section it means they are not used.
 * 	 */
 * 	for_each_shdr_str(len, ehdr, tracepoint_data_sec) {
 */
