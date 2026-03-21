/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_038 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 27 */
/* NLOC: 101 */
/* Subsystem: tools */
/* Includes
 * #include <errno.h>
 * #include <linux/err.h>
 * #include <linux/netfilter.h>
 * #include <linux/netfilter_arp.h>
 * #include <linux/perf_event.h>
 * #include <net/if.h>
 * #include <stdio.h>
 * #include <unistd.h>
 * #include <bpf/bpf.h>
 * #include <bpf/hashmap.h>
 * #include "json_writer.h"
 * #include "main.h"
 * #include "xlated_dumper.h"
 */
/* Context-Before
 * 	printf("\n\tevent ");
 * 	perf_type = perf_event_name(perf_type_name, type);
 * 	if (perf_type)
 * 		printf("%s:", perf_type);
 * 	else
 * 		printf("%u :", type);
 * 
 * 	perf_config = perf_config_str(type, config);
 * 	if (perf_config)
 * 		printf("%s  ", perf_config);
 * 	else
 * 		printf("%llu  ", config);
 * 
 * 	if (info->perf_event.event.cookie)
 * 		printf("cookie %llu  ", info->perf_event.event.cookie);
 * 
 * 	if (type == PERF_TYPE_HW_CACHE && perf_config)
 * 		free((void *)perf_config);
 * }
 */
static int show_link_close_plain(int fd, struct bpf_link_info *info)
{
	struct bpf_prog_info prog_info;
	const char *prog_type_str;
	int err;

	show_link_header_plain(info);

	switch (info->type) {
	case BPF_LINK_TYPE_RAW_TRACEPOINT:
		printf("\n\ttp '%s'  ",
		       (const char *)u64_to_ptr(info->raw_tracepoint.tp_name));
		if (info->raw_tracepoint.cookie)
			printf("cookie %llu  ", info->raw_tracepoint.cookie);
		break;
	case BPF_LINK_TYPE_TRACING:
		err = get_prog_info(info->prog_id, &prog_info);
		if (err)
			return err;

		prog_type_str = libbpf_bpf_prog_type_str(prog_info.type);
		/* libbpf will return NULL for variants unknown to it. */
		if (prog_type_str)
			printf("\n\tprog_type %s  ", prog_type_str);
		else
			printf("\n\tprog_type %u  ", prog_info.type);

		show_link_attach_type_plain(info->tracing.attach_type);
		if (info->tracing.target_obj_id || info->tracing.target_btf_id)
			printf("\n\ttarget_obj_id %u  target_btf_id %u  ",
			       info->tracing.target_obj_id,
			       info->tracing.target_btf_id);
		if (info->tracing.cookie)
			printf("\n\tcookie %llu  ", info->tracing.cookie);
		break;
	case BPF_LINK_TYPE_CGROUP:
		printf("\n\tcgroup_id %zu  ", (size_t)info->cgroup.cgroup_id);
		show_link_attach_type_plain(info->cgroup.attach_type);
		break;
	case BPF_LINK_TYPE_ITER:
		show_iter_plain(info);
		break;
	case BPF_LINK_TYPE_NETNS:
		printf("\n\tnetns_ino %u  ", info->netns.netns_ino);
		show_link_attach_type_plain(info->netns.attach_type);
		break;
	case BPF_LINK_TYPE_NETFILTER:
		netfilter_dump_plain(info);
		break;
	case BPF_LINK_TYPE_TCX:
		printf("\n\t");
		show_link_ifindex_plain(info->tcx.ifindex);
		show_link_attach_type_plain(info->tcx.attach_type);
		break;
	case BPF_LINK_TYPE_NETKIT:
		printf("\n\t");
		show_link_ifindex_plain(info->netkit.ifindex);
		show_link_attach_type_plain(info->netkit.attach_type);
		break;
	case BPF_LINK_TYPE_SOCKMAP:
		printf("\n\t");
		printf("map_id %u  ", info->sockmap.map_id);
		show_link_attach_type_plain(info->sockmap.attach_type);
		break;
	case BPF_LINK_TYPE_XDP:
		printf("\n\t");
		show_link_ifindex_plain(info->xdp.ifindex);
		break;
	case BPF_LINK_TYPE_KPROBE_MULTI:
		show_kprobe_multi_plain(info);
		break;
	case BPF_LINK_TYPE_UPROBE_MULTI:
		show_uprobe_multi_plain(info);
		break;
	case BPF_LINK_TYPE_PERF_EVENT:
		switch (info->perf_event.type) {
		case BPF_PERF_EVENT_EVENT:
			show_perf_event_event_plain(info);
			break;
		case BPF_PERF_EVENT_TRACEPOINT:
			show_perf_event_tracepoint_plain(info);
			break;
		case BPF_PERF_EVENT_KPROBE:
		case BPF_PERF_EVENT_KRETPROBE:
			show_perf_event_kprobe_plain(info);
			break;
		case BPF_PERF_EVENT_UPROBE:
		case BPF_PERF_EVENT_URETPROBE:
			show_perf_event_uprobe_plain(info);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (!hashmap__empty(link_table)) {
		struct hashmap_entry *entry;

		hashmap__for_each_key_entry(link_table, entry, info->id)
			printf("\n\tpinned %s", (char *)entry->pvalue);
	}
	emit_obj_refs_plain(refs_table, info->id, "\n\tpids ");

	printf("\n");

	return 0;
}
/* Context-After
 * static int do_show_link(int fd)
 * {
 * 	__u64 *ref_ctr_offsets = NULL, *offsets = NULL, *cookies = NULL;
 * 	struct bpf_link_info info;
 * 	__u32 len = sizeof(info);
 * 	char path_buf[PATH_MAX];
 * 	__u64 *addrs = NULL;
 * 	char buf[PATH_MAX];
 * 	int count;
 * 	int err;
 * 
 * 	memset(&info, 0, sizeof(info));
 * 	buf[0] = '\0';
 * again:
 * 	err = bpf_link_get_info_by_fd(fd, &info, &len);
 * 	if (err) {
 * 		p_err("can't get link info: %s",
 * 		      strerror(errno));
 * 		close(fd);
 */
