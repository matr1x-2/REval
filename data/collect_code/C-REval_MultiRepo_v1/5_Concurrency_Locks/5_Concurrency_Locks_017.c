/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_017 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 2 */
/* NLOC: 16 */
/* Subsystem: arch */
/* Includes
 * #include <linux/uaccess.h>
 * #include <linux/interrupt.h>
 * #include <linux/cpu.h>
 * #include <linux/kvm_host.h>
 * #include <kvm/arm_vgic.h>
 * #include <asm/kvm_emulate.h>
 * #include <asm/kvm_mmu.h>
 * #include "vgic.h"
 */
/* Context-Before
 * 		 * kvm->arch.config_lock and do not get unregistered in
 * 		 * kvm_vgic_destroy(), meaning it is both safe and necessary to
 * 		 * do so here.
 * 		 */
 * 		if (kvm_get_vcpu_by_id(vcpu->kvm, vcpu->vcpu_id) != vcpu)
 * 			vgic_unregister_redist_iodev(vcpu);
 * 
 * 		vgic_cpu->rd_iodev.base_addr = VGIC_ADDR_UNDEF;
 * 	}
 * }
 * 
 * void kvm_vgic_vcpu_destroy(struct kvm_vcpu *vcpu)
 * {
 * 	struct kvm *kvm = vcpu->kvm;
 * 
 * 	mutex_lock(&kvm->slots_lock);
 * 	__kvm_vgic_vcpu_destroy(vcpu);
 * 	mutex_unlock(&kvm->slots_lock);
 * }
 */
void kvm_vgic_destroy(struct kvm *kvm)
{
	struct kvm_vcpu *vcpu;
	unsigned long i;

	mutex_lock(&kvm->slots_lock);
	mutex_lock(&kvm->arch.config_lock);

	vgic_debug_destroy(kvm);

	kvm_for_each_vcpu(i, vcpu, kvm)
		__kvm_vgic_vcpu_destroy(vcpu);

	kvm_vgic_dist_destroy(kvm);

	mutex_unlock(&kvm->arch.config_lock);

	if (kvm->arch.vgic.vgic_model == KVM_DEV_TYPE_ARM_VGIC_V3)
		kvm_for_each_vcpu(i, vcpu, kvm)
			vgic_unregister_redist_iodev(vcpu);

	mutex_unlock(&kvm->slots_lock);
}
/* Context-After
 * /**
 *  * vgic_lazy_init: Lazy init is only allowed if the GIC exposed to the guest
 *  * is a GICv2. A GICv3 must be explicitly initialized by userspace using the
 *  * KVM_DEV_ARM_VGIC_GRP_CTRL KVM_DEVICE group.
 *  * @kvm: kvm struct pointer
 *  */
 * int vgic_lazy_init(struct kvm *kvm)
 * {
 * 	int ret = 0;
 * 
 * 	if (unlikely(!vgic_initialized(kvm))) {
 * 		/*
 * 		 * We only provide the automatic initialization of the VGIC
 * 		 * for the legacy case of a GICv2. Any other type must
 * 		 * be explicitly initialized once setup with the respective
 * 		 * KVM device call.
 * 		 */
 * 		if (kvm->arch.vgic.vgic_model != KVM_DEV_TYPE_ARM_VGIC_V2)
 * 			return -EBUSY;
 */
