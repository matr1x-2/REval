/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_010 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 9 */
/* NLOC: 47 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/cdev.h>
 * #include <linux/poll.h>
 * #include <linux/sched/signal.h>
 * #include <linux/hid-roccat.h>
 * #include <linux/module.h>
 */
/* Context-Before
 * 	 * read pointers
 * 	 */
 * 	struct roccat_report cbuf[ROCCAT_CBUF_SIZE];
 * 	int cbuf_end;
 * 	struct mutex cbuf_lock;
 * };
 * 
 * struct roccat_reader {
 * 	struct list_head node;
 * 	struct roccat_device *device;
 * 	int cbuf_start;
 * };
 * 
 * static int roccat_major;
 * static struct cdev roccat_cdev;
 * 
 * static struct roccat_device *devices[ROCCAT_MAX_DEVICES];
 * /* protects modifications of devices array */
 * static DEFINE_MUTEX(devices_lock);
 */
static ssize_t roccat_read(struct file *file, char __user *buffer,
		size_t count, loff_t *ppos)
{
	struct roccat_reader *reader = file->private_data;
	struct roccat_device *device = reader->device;
	struct roccat_report *report;
	ssize_t retval = 0, len;
	DECLARE_WAITQUEUE(wait, current);

	mutex_lock(&device->cbuf_lock);

	/* no data? */
	if (reader->cbuf_start == device->cbuf_end) {
		add_wait_queue(&device->wait, &wait);
		set_current_state(TASK_INTERRUPTIBLE);

		/* wait for data */
		while (reader->cbuf_start == device->cbuf_end) {
			if (file->f_flags & O_NONBLOCK) {
				retval = -EAGAIN;
				break;
			}
			if (signal_pending(current)) {
				retval = -ERESTARTSYS;
				break;
			}
			if (!device->exist) {
				retval = -EIO;
				break;
			}

			mutex_unlock(&device->cbuf_lock);
			schedule();
			mutex_lock(&device->cbuf_lock);
			set_current_state(TASK_INTERRUPTIBLE);
		}

		set_current_state(TASK_RUNNING);
		remove_wait_queue(&device->wait, &wait);
	}

	/* here we either have data or a reason to return if retval is set */
	if (retval)
		goto exit_unlock;

	report = &device->cbuf[reader->cbuf_start];
	/*
	 * If report is larger than requested amount of data, rest of report
	 * is lost!
	 */
	len = device->report_size > count ? count : device->report_size;

	if (copy_to_user(buffer, report->value, len)) {
		retval = -EFAULT;
		goto exit_unlock;
	}
	retval += len;
	reader->cbuf_start = (reader->cbuf_start + 1) % ROCCAT_CBUF_SIZE;

exit_unlock:
	mutex_unlock(&device->cbuf_lock);
	return retval;
}
/* Context-After
 * static __poll_t roccat_poll(struct file *file, poll_table *wait)
 * {
 * 	struct roccat_reader *reader = file->private_data;
 * 	poll_wait(file, &reader->device->wait, wait);
 * 	if (reader->cbuf_start != reader->device->cbuf_end)
 * 		return EPOLLIN | EPOLLRDNORM;
 * 	if (!reader->device->exist)
 * 		return EPOLLERR | EPOLLHUP;
 * 	return 0;
 * }
 * 
 * static int roccat_open(struct inode *inode, struct file *file)
 * {
 * 	unsigned int minor = iminor(inode);
 * 	struct roccat_reader *reader;
 * 	struct roccat_device *device;
 * 	int error = 0;
 * 
 * 	reader = kzalloc_obj(struct roccat_reader);
 */
