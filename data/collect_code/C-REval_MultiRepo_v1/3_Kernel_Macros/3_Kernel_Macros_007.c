/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_007 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 38 */
/* Subsystem: arch */
/* Includes
 * #include <linux/anon_inodes.h>
 * #include <linux/file.h>
 * #include <linux/fs.h>
 * #include <linux/init.h>
 * #include <linux/kernel.h>
 * #include <linux/miscdevice.h>
 * #include <asm/machdep.h>
 * #include <asm/rtas-work-area.h>
 * #include <asm/rtas.h>
 * #include <uapi/asm/papr-platform-dump.h>
 */
/* Context-Before
 * /**
 *  * papr_platform_dump_create_handle() - Create a fd-based handle for
 *  * reading platform dump
 *  *
 *  * Handler for PAPR_PLATFORM_DUMP_IOC_CREATE_HANDLE ioctl command
 *  * Allocates RTAS parameter struct and work area and attached to the
 *  * file descriptor for reading by user space with the multiple RTAS
 *  * calls until the dump is completed. This memory allocation is freed
 *  * when the file is released.
 *  *
 *  * Multiple dump requests with different IDs are allowed at the same
 *  * time, but not with the same dump ID. So if the user space is
 *  * already opened file descriptor for the specific dump ID, return
 *  * -EALREADY for the next request.
 *  *
 *  * @dump_tag: Dump ID for the dump requested to retrieve from the
 *  *		hypervisor
 *  *
 *  * Return: The installed fd number if successful, -ve errno otherwise.
 *  */
 */
static long papr_platform_dump_create_handle(u64 dump_tag)
{
	struct ibm_platform_dump_params *params;
	u64 param_dump_tag;
	int fd;

	/*
	 * Return failure if the user space is already opened FD for
	 * the specific dump ID. This check will prevent multiple dump
	 * requests for the same dump ID at the same time. Generally
	 * should not expect this, but in case.
	 */
	list_for_each_entry(params, &platform_dump_list, list) {
		param_dump_tag = (u64) (((u64)params->dump_tag_hi << 32) |
					params->dump_tag_lo);
		if (dump_tag == param_dump_tag) {
			pr_err("Platform dump for ID(%llu) is already in progress\n",
					dump_tag);
			return -EALREADY;
		}
	}

	params =  kzalloc_obj(struct ibm_platform_dump_params,
			      GFP_KERNEL_ACCOUNT);
	if (!params)
		return -ENOMEM;

	params->work_area = rtas_work_area_alloc(SZ_4K);
	params->buf_length = SZ_4K;
	params->dump_tag_hi = (u32)(dump_tag >> 32);
	params->dump_tag_lo = (u32)(dump_tag & 0x00000000ffffffffULL);
	params->status = RTAS_IBM_PLATFORM_DUMP_START;

	fd = FD_ADD(O_RDONLY | O_CLOEXEC,
		    anon_inode_getfile_fmode("[papr-platform-dump]",
					     &papr_platform_dump_handle_ops,
					     (void *)params, O_RDONLY,
					     FMODE_LSEEK | FMODE_PREAD));
	if (fd < 0) {
		rtas_work_area_free(params->work_area);
		kfree(params);
		return fd;
	}

	list_add(&params->list, &platform_dump_list);

	pr_info("%s (%d) initiated platform dump for dump tag %llu\n",
		current->comm, current->pid, dump_tag);
	return fd;
}
/* Context-After
 * /*
 *  * Top-level ioctl handler for /dev/papr-platform-dump.
 *  */
 * static long papr_platform_dump_dev_ioctl(struct file *filp,
 * 					unsigned int ioctl,
 * 					unsigned long arg)
 * {
 * 	u64 __user *argp = (void __user *)arg;
 * 	u64 dump_tag;
 * 	long ret;
 * 
 * 	if (get_user(dump_tag, argp))
 * 		return -EFAULT;
 * 
 * 	switch (ioctl) {
 * 	case PAPR_PLATFORM_DUMP_IOC_CREATE_HANDLE:
 * 		mutex_lock(&platform_dump_list_mutex);
 * 		ret = papr_platform_dump_create_handle(dump_tag);
 * 		mutex_unlock(&platform_dump_list_mutex);
 */
