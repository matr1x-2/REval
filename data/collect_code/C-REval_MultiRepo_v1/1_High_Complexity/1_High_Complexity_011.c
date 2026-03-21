/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_011 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 43 */
/* NLOC: 138 */
/* Subsystem: kernel */
/* Includes
 * #include <linux/suspend.h>
 * #include <linux/reboot.h>
 * #include <linux/string.h>
 * #include <linux/device.h>
 * #include <linux/miscdevice.h>
 * #include <linux/mm.h>
 * #include <linux/swap.h>
 * #include <linux/swapops.h>
 * #include <linux/pm.h>
 * #include <linux/fs.h>
 * #include <linux/compat.h>
 * #include <linux/console.h>
 * #include <linux/cpu.h>
 * #include <linux/freezer.h>
 * #include <linux/uaccess.h>
 * #include "power.h"
 */
/* Context-Before
 * 	} else {
 * 		struct resume_swap_area swap_area;
 * 
 * 		if (copy_from_user(&swap_area, argp, sizeof(swap_area)))
 * 			return -EFAULT;
 * 		swdev = new_decode_dev(swap_area.dev);
 * 		offset = swap_area.offset;
 * 	}
 * 
 * 	/*
 * 	 * User space encodes device types as two-byte values,
 * 	 * so we need to recode them
 * 	 */
 * 	data->swap = swap_type_of(swdev, offset);
 * 	if (data->swap < 0)
 * 		return swdev ? -ENODEV : -EINVAL;
 * 	data->dev = swdev;
 * 	return 0;
 * }
 */
static long snapshot_ioctl(struct file *filp, unsigned int cmd,
							unsigned long arg)
{
	int error = 0;
	struct snapshot_data *data;
	loff_t size;
	sector_t offset;

	if (need_wait) {
		wait_for_device_probe();
		need_wait = false;
	}

	if (_IOC_TYPE(cmd) != SNAPSHOT_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > SNAPSHOT_IOC_MAXNR)
		return -ENOTTY;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (!mutex_trylock(&system_transition_mutex))
		return -EBUSY;

	lock_device_hotplug();
	data = filp->private_data;

	switch (cmd) {

	case SNAPSHOT_FREEZE:
		if (data->frozen)
			break;

		error = pm_sleep_fs_sync();
		if (error)
			break;

		error = freeze_processes();
		if (error)
			break;

		error = create_basic_memory_bitmaps();
		if (error)
			thaw_processes();
		else
			data->frozen = true;

		break;

	case SNAPSHOT_UNFREEZE:
		if (!data->frozen || data->ready)
			break;
		pm_restore_gfp_mask();
		free_basic_memory_bitmaps();
		data->free_bitmaps = false;
		thaw_processes();
		data->frozen = false;
		break;

	case SNAPSHOT_CREATE_IMAGE:
		if (data->mode != O_RDONLY || !data->frozen  || data->ready) {
			error = -EPERM;
			break;
		}
		pm_restore_gfp_mask();
		error = hibernation_snapshot(data->platform_support);
		if (!error) {
			error = put_user(in_suspend, (int __user *)arg);
			data->ready = !freezer_test_done && !error;
			freezer_test_done = false;
		}
		break;

	case SNAPSHOT_ATOMIC_RESTORE:
		error = snapshot_write_finalize(&data->handle);
		if (error)
			break;
		if (data->mode != O_WRONLY || !data->frozen ||
		    !snapshot_image_loaded(&data->handle)) {
			error = -EPERM;
			break;
		}
		error = hibernation_restore(data->platform_support);
		break;

	case SNAPSHOT_FREE:
		swsusp_free();
		memset(&data->handle, 0, sizeof(struct snapshot_handle));
		data->ready = false;
		/*
		 * It is necessary to thaw kernel threads here, because
		 * SNAPSHOT_CREATE_IMAGE may be invoked directly after
		 * SNAPSHOT_FREE.  In that case, if kernel threads were not
		 * thawed, the preallocation of memory carried out by
		 * hibernation_snapshot() might run into problems (i.e. it
		 * might fail or even deadlock).
		 */
		thaw_kernel_threads();
		break;

	case SNAPSHOT_PREF_IMAGE_SIZE:
		image_size = arg;
		break;

	case SNAPSHOT_GET_IMAGE_SIZE:
		if (!data->ready) {
			error = -ENODATA;
			break;
		}
		size = snapshot_get_image_size();
		size <<= PAGE_SHIFT;
		error = put_user(size, (loff_t __user *)arg);
		break;

	case SNAPSHOT_AVAIL_SWAP_SIZE:
		size = count_swap_pages(data->swap, 1);
		size <<= PAGE_SHIFT;
		error = put_user(size, (loff_t __user *)arg);
		break;

	case SNAPSHOT_ALLOC_SWAP_PAGE:
		if (data->swap < 0 || data->swap >= MAX_SWAPFILES) {
			error = -ENODEV;
			break;
		}
		offset = alloc_swapdev_block(data->swap);
		if (offset) {
			offset <<= PAGE_SHIFT;
			error = put_user(offset, (loff_t __user *)arg);
		} else {
			error = -ENOSPC;
		}
		break;

	case SNAPSHOT_FREE_SWAP_PAGES:
		if (data->swap < 0 || data->swap >= MAX_SWAPFILES) {
			error = -ENODEV;
			break;
		}
		free_all_swap_pages(data->swap);
		break;

	case SNAPSHOT_S2RAM:
		if (!data->frozen) {
			error = -EPERM;
			break;
		}
		/*
		 * Tasks are frozen and the notifiers have been called with
		 * PM_HIBERNATION_PREPARE
		 */
		error = suspend_devices_and_enter(PM_SUSPEND_MEM);
		data->ready = false;
		break;

	case SNAPSHOT_PLATFORM_SUPPORT:
		data->platform_support = !!arg;
		break;

	case SNAPSHOT_POWER_OFF:
		if (data->platform_support)
			error = hibernation_platform_enter();
		break;

	case SNAPSHOT_SET_SWAP_AREA:
		error = snapshot_set_swap_area(data, (void __user *)arg);
		break;

	default:
		error = -ENOTTY;

	}

	unlock_device_hotplug();
	mutex_unlock(&system_transition_mutex);

	return error;
}
/* Context-After
 * #ifdef CONFIG_COMPAT
 * static long
 * snapshot_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
 * {
 * 	BUILD_BUG_ON(sizeof(loff_t) != sizeof(compat_loff_t));
 * 
 * 	switch (cmd) {
 * 	case SNAPSHOT_GET_IMAGE_SIZE:
 * 	case SNAPSHOT_AVAIL_SWAP_SIZE:
 * 	case SNAPSHOT_ALLOC_SWAP_PAGE:
 * 	case SNAPSHOT_CREATE_IMAGE:
 * 	case SNAPSHOT_SET_SWAP_AREA:
 * 		return snapshot_ioctl(file, cmd,
 * 				      (unsigned long) compat_ptr(arg));
 * 	default:
 * 		return snapshot_ioctl(file, cmd, arg);
 * 	}
 * }
 * #endif /* CONFIG_COMPAT */
 */
