/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_025 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 34 */
/* Subsystem: tools */
/* Includes
 * #include <errno.h>
 * #include <stdio.h>
 * #include <unistd.h>
 * #include <linux/err.h>
 * #include <bpf/bpf.h>
 * #include <bpf/btf.h>
 * #include <bpf/libbpf.h>
 * #include "json_writer.h"
 * #include "main.h"
 */
/* Context-Before
 * 		      info->name, info->id);
 * 		free(value);
 * 		return -1;
 * 	}
 * 
 * 	jsonw_start_object(wtr);
 * 	jsonw_name(wtr, "bpf_map_info");
 * 	btf_dumper_type(d, map_info_type_id, (void *)info);
 * 	jsonw_end_object(wtr);
 * 
 * 	jsonw_start_object(wtr);
 * 	jsonw_name(wtr, struct_ops_name);
 * 	btf_dumper_type(d, info->btf_vmlinux_value_type_id, value);
 * 	jsonw_end_object(wtr);
 * 
 * 	free(value);
 * 
 * 	return 0;
 * }
 */
static int do_dump(int argc, char **argv)
{
	const char *search_type = NULL, *search_term = NULL;
	json_writer_t *wtr = json_wtr;
	const struct btf *kern_btf;
	struct btf_dumper d = {};
	struct res res;

	if (argc && argc != 2)
		usage();

	if (argc == 2) {
		search_type = GET_ARG();
		search_term = GET_ARG();
	}

	kern_btf = get_btf_vmlinux();
	if (!kern_btf)
		return -1;

	if (!json_output) {
		wtr = jsonw_new(stdout);
		if (!wtr) {
			p_err("can't create json writer");
			return -1;
		}
		jsonw_pretty(wtr, true);
	}

	d.btf = kern_btf;
	d.jw = wtr;
	d.is_plain_text = !json_output;
	d.prog_id_as_func_ptr = true;

	res = do_work_on_struct_ops(search_type, search_term, __do_dump, &d,
				    wtr);

	if (!json_output)
		jsonw_destroy(&wtr);

	return cmd_retval(&res, !!search_term);
}
/* Context-After
 * static int __do_unregister(int fd, const struct bpf_map_info *info, void *data,
 * 			   struct json_writer *wtr)
 * {
 * 	int zero = 0;
 * 
 * 	if (bpf_map_delete_elem(fd, &zero)) {
 * 		p_err("can't unload %s %s id %u: %s",
 * 		      get_kern_struct_ops_name(info), info->name,
 * 		      info->id, strerror(errno));
 * 		return -1;
 * 	}
 * 
 * 	p_info("Unregistered %s %s id %u",
 * 	       get_kern_struct_ops_name(info), info->name,
 * 	       info->id);
 * 
 * 	return 0;
 * }
 */
