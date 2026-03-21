/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_005 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 10 */
/* NLOC: 55 */
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
 * 	/* Disable the VP */
 * 	xive_native_disable_vp(xc->vp_id);
 * 
 * 	/* Clear the cam word so guest entry won't try to push context */
 * 	vcpu->arch.xive_cam_word = 0;
 * 
 * 	/* Free the queues */
 * 	for (i = 0; i < KVMPPC_XIVE_Q_COUNT; i++) {
 * 		kvmppc_xive_native_cleanup_queue(vcpu, i);
 * 	}
 * 
 * 	/* Free the VP */
 * 	kfree(xc);
 * 
 * 	/* Cleanup the vcpu */
 * 	vcpu->arch.irq_type = KVMPPC_IRQ_DEFAULT;
 * 	vcpu->arch.xive_vcpu = NULL;
 * }
 */
int kvmppc_xive_native_connect_vcpu(struct kvm_device *dev,
				    struct kvm_vcpu *vcpu, u32 server_num)
{
	struct kvmppc_xive *xive = dev->private;
	struct kvmppc_xive_vcpu *xc = NULL;
	int rc;
	u32 vp_id;

	pr_devel("native_connect_vcpu(server=%d)\n", server_num);

	if (dev->ops != &kvm_xive_native_ops) {
		pr_devel("Wrong ops !\n");
		return -EPERM;
	}
	if (xive->kvm != vcpu->kvm)
		return -EPERM;
	if (vcpu->arch.irq_type != KVMPPC_IRQ_DEFAULT)
		return -EBUSY;

	mutex_lock(&xive->lock);

	rc = kvmppc_xive_compute_vp_id(xive, server_num, &vp_id);
	if (rc)
		goto bail;

	xc = kzalloc_obj(*xc);
	if (!xc) {
		rc = -ENOMEM;
		goto bail;
	}

	vcpu->arch.xive_vcpu = xc;
	xc->xive = xive;
	xc->vcpu = vcpu;
	xc->server_num = server_num;

	xc->vp_id = vp_id;
	xc->valid = true;
	vcpu->arch.irq_type = KVMPPC_IRQ_XIVE;

	rc = xive_native_get_vp_info(xc->vp_id, &xc->vp_cam, &xc->vp_chip_id);
	if (rc) {
		pr_err("Failed to get VP info from OPAL: %d\n", rc);
		goto bail;
	}

	if (!kvmppc_xive_check_save_restore(vcpu)) {
		pr_err("inconsistent save-restore setup for VCPU %d\n", server_num);
		rc = -EIO;
		goto bail;
	}

	/*
	 * Enable the VP first as the single escalation mode will
	 * affect escalation interrupts numbering
	 */
	rc = xive_native_enable_vp(xc->vp_id, kvmppc_xive_has_single_escalation(xive));
	if (rc) {
		pr_err("Failed to enable VP in OPAL: %d\n", rc);
		goto bail;
	}

	/* Configure VCPU fields for use by assembly push/pull */
	vcpu->arch.xive_saved_state.w01 = cpu_to_be64(0xff000000);
	vcpu->arch.xive_cam_word = cpu_to_be32(xc->vp_cam | TM_QW1W2_VO);

	/* TODO: reset all queues to a clean state ? */
bail:
	mutex_unlock(&xive->lock);
	if (rc)
		kvmppc_xive_native_cleanup_vcpu(vcpu);

	return rc;
}
/* Context-After
 * /*
 *  * Device passthrough support
 *  */
 * static int kvmppc_xive_native_reset_mapped(struct kvm *kvm, unsigned long irq)
 * {
 * 	struct kvmppc_xive *xive = kvm->arch.xive;
 * 	pgoff_t esb_pgoff = KVM_XIVE_ESB_PAGE_OFFSET + irq * 2;
 * 
 * 	if (irq >= KVMPPC_XIVE_NR_IRQS)
 * 		return -EINVAL;
 * 
 * 	/*
 * 	 * Clear the ESB pages of the IRQ number being mapped (or
 * 	 * unmapped) into the guest and let the VM fault handler
 * 	 * repopulate with the appropriate ESB pages (device or IC)
 * 	 */
 * 	pr_debug("clearing esb pages for girq 0x%lx\n", irq);
 * 	mutex_lock(&xive->mapping_lock);
 * 	if (xive->mapping)
 */
