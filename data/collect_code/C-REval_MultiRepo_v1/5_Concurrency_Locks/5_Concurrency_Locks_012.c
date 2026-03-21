/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_012 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 27 */
/* Subsystem: arch */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/kvm_host.h>
 * #include <linux/err.h>
 * #include <linux/gfp.h>
 * #include <linux/spinlock.h>
 * #include <linux/delay.h>
 * #include <linux/file.h>
 * #include <linux/irqdomain.h>
 * #include <asm/uaccess.h>
 * #include <asm/kvm_book3s.h>
 * #include <asm/kvm_ppc.h>
 * #include <asm/hvcall.h>
 * #include <asm/xive.h>
 * #include <asm/xive-regs.h>
 * #include <asm/debug.h>
 * #include <asm/opal.h>
 * #include <linux/debugfs.h>
 * #include <linux/seq_file.h>
 * #include "book3s_xive.h"
 */
/* Context-Before
 * 		case KVM_DEV_XIVE_NR_SERVERS:
 * 			return 0;
 * 		}
 * 		break;
 * 	case KVM_DEV_XIVE_GRP_SOURCE:
 * 	case KVM_DEV_XIVE_GRP_SOURCE_CONFIG:
 * 	case KVM_DEV_XIVE_GRP_SOURCE_SYNC:
 * 		if (attr->attr >= KVMPPC_XIVE_FIRST_IRQ &&
 * 		    attr->attr < KVMPPC_XIVE_NR_IRQS)
 * 			return 0;
 * 		break;
 * 	case KVM_DEV_XIVE_GRP_EQ_CONFIG:
 * 		return 0;
 * 	}
 * 	return -ENXIO;
 * }
 * 
 * /*
 *  * Called when device fd is closed.  kvm->lock is held.
 *  */
 */
static void kvmppc_xive_native_release(struct kvm_device *dev)
{
	struct kvmppc_xive *xive = dev->private;
	struct kvm *kvm = xive->kvm;
	struct kvm_vcpu *vcpu;
	unsigned long i;

	pr_devel("Releasing xive native device\n");

	/*
	 * Clear the KVM device file address_space which is used to
	 * unmap the ESB pages when a device is passed-through.
	 */
	mutex_lock(&xive->mapping_lock);
	xive->mapping = NULL;
	mutex_unlock(&xive->mapping_lock);

	/*
	 * Since this is the device release function, we know that
	 * userspace does not have any open fd or mmap referring to
	 * the device.  Therefore there can not be any of the
	 * device attribute set/get, mmap, or page fault functions
	 * being executed concurrently, and similarly, the
	 * connect_vcpu and set/clr_mapped functions also cannot
	 * be being executed.
	 */

	debugfs_remove(xive->dentry);

	/*
	 * We should clean up the vCPU interrupt presenters first.
	 */
	kvm_for_each_vcpu(i, vcpu, kvm) {
		/*
		 * Take vcpu->mutex to ensure that no one_reg get/set ioctl
		 * (i.e. kvmppc_xive_native_[gs]et_vp) can be being done.
		 * Holding the vcpu->mutex also means that the vcpu cannot
		 * be executing the KVM_RUN ioctl, and therefore it cannot
		 * be executing the XIVE push or pull code or accessing
		 * the XIVE MMIO regions.
		 */
		mutex_lock(&vcpu->mutex);
		kvmppc_xive_native_cleanup_vcpu(vcpu);
		mutex_unlock(&vcpu->mutex);
	}

	/*
	 * Now that we have cleared vcpu->arch.xive_vcpu, vcpu->arch.irq_type
	 * and vcpu->arch.xive_esc_[vr]addr on each vcpu, we are safe
	 * against xive code getting called during vcpu execution or
	 * set/get one_reg operations.
	 */
	kvm->arch.xive = NULL;

	for (i = 0; i <= xive->max_sbid; i++) {
		if (xive->src_blocks[i])
			kvmppc_xive_free_sources(xive->src_blocks[i]);
		kfree(xive->src_blocks[i]);
		xive->src_blocks[i] = NULL;
	}

	if (xive->vp_base != XIVE_INVALID_VP)
		xive_native_free_vp_block(xive->vp_base);

	/*
	 * A reference of the kvmppc_xive pointer is now kept under
	 * the xive_devices struct of the machine for reuse. It is
	 * freed when the VM is destroyed for now until we fix all the
	 * execution paths.
	 */

	kfree(dev);
}
/* Context-After
 * /*
 *  * Create a XIVE device.  kvm->lock is held.
 *  */
 * static int kvmppc_xive_native_create(struct kvm_device *dev, u32 type)
 * {
 * 	struct kvmppc_xive *xive;
 * 	struct kvm *kvm = dev->kvm;
 * 
 * 	pr_devel("Creating xive native device\n");
 * 
 * 	if (kvm->arch.xive)
 * 		return -EEXIST;
 * 
 * 	xive = kvmppc_xive_get_device(kvm, type);
 * 	if (!xive)
 * 		return -ENOMEM;
 * 
 * 	dev->private = xive;
 * 	xive->dev = dev;
 */
