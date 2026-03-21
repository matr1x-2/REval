/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_025 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 13 */
/* NLOC: 69 */
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
 *  */
 * void kvm_vgic_early_init(struct kvm *kvm)
 * {
 * 	struct vgic_dist *dist = &kvm->arch.vgic;
 * 
 * 	xa_init_flags(&dist->lpi_xa, XA_FLAGS_LOCK_IRQ);
 * }
 * 
 * /* CREATION */
 * 
 * static int vgic_allocate_private_irqs_locked(struct kvm_vcpu *vcpu, u32 type);
 * 
 * /**
 *  * kvm_vgic_create: triggered by the instantiation of the VGIC device by
 *  * user space, either through the legacy KVM_CREATE_IRQCHIP ioctl (v2 only)
 *  * or through the generic KVM_CREATE_DEVICE API ioctl.
 *  * irqchip_in_kernel() tells you if this function succeeded or not.
 *  * @kvm: kvm struct pointer
 *  * @type: KVM_DEV_TYPE_ARM_VGIC_V[23]
 *  */
 */
int kvm_vgic_create(struct kvm *kvm, u32 type)
{
	struct kvm_vcpu *vcpu;
	u64 aa64pfr0, pfr1;
	unsigned long i;
	int ret;

	/*
	 * This function is also called by the KVM_CREATE_IRQCHIP handler,
	 * which had no chance yet to check the availability of the GICv2
	 * emulation. So check this here again. KVM_CREATE_DEVICE does
	 * the proper checks already.
	 */
	if (type == KVM_DEV_TYPE_ARM_VGIC_V2 &&
		!kvm_vgic_global_state.can_emulate_gicv2)
		return -ENODEV;

	/*
	 * Ensure mutual exclusion with vCPU creation and any vCPU ioctls by:
	 *
	 *  - Holding kvm->lock to prevent KVM_CREATE_VCPU from reaching
	 *    kvm_arch_vcpu_precreate() and ensuring created_vcpus is stable.
	 *    This alone is insufficient, as kvm_vm_ioctl_create_vcpu() drops
	 *    the kvm->lock before completing the vCPU creation.
	 */
	lockdep_assert_held(&kvm->lock);

	/*
	 *  - Acquiring the vCPU mutex for every *online* vCPU to prevent
	 *    concurrent vCPU ioctls for vCPUs already visible to userspace.
	 */
	ret = -EBUSY;
	if (kvm_trylock_all_vcpus(kvm))
		return ret;

	/*
	 *  - Taking the config_lock which protects VGIC data structures such
	 *    as the per-vCPU arrays of private IRQs (SGIs, PPIs).
	 */
	mutex_lock(&kvm->arch.config_lock);

	/*
	 * - Bailing on the entire thing if a vCPU is in the middle of creation,
	 *   dropped the kvm->lock, but hasn't reached kvm_arch_vcpu_create().
	 *
	 * The whole combination of this guarantees that no vCPU can get into
	 * KVM with a VGIC configuration inconsistent with the VM's VGIC.
	 */
	if (kvm->created_vcpus != atomic_read(&kvm->online_vcpus))
		goto out_unlock;

	if (irqchip_in_kernel(kvm)) {
		ret = -EEXIST;
		goto out_unlock;
	}

	kvm_for_each_vcpu(i, vcpu, kvm) {
		if (vcpu_has_run_once(vcpu))
			goto out_unlock;
	}
	ret = 0;

	if (type == KVM_DEV_TYPE_ARM_VGIC_V2)
		kvm->max_vcpus = VGIC_V2_MAX_CPUS;
	else
		kvm->max_vcpus = VGIC_V3_MAX_CPUS;

	if (atomic_read(&kvm->online_vcpus) > kvm->max_vcpus) {
		ret = -E2BIG;
		goto out_unlock;
	}

	kvm->arch.vgic.in_kernel = true;
	kvm->arch.vgic.vgic_model = type;
	kvm->arch.vgic.implementation_rev = KVM_VGIC_IMP_REV_LATEST;
	kvm->arch.vgic.vgic_dist_base = VGIC_ADDR_UNDEF;

	aa64pfr0 = kvm_read_vm_id_reg(kvm, SYS_ID_AA64PFR0_EL1) & ~ID_AA64PFR0_EL1_GIC;
	pfr1 = kvm_read_vm_id_reg(kvm, SYS_ID_PFR1_EL1) & ~ID_PFR1_EL1_GIC;

	if (type == KVM_DEV_TYPE_ARM_VGIC_V2) {
		kvm->arch.vgic.vgic_cpu_base = VGIC_ADDR_UNDEF;
	} else {
		INIT_LIST_HEAD(&kvm->arch.vgic.rd_regions);
		aa64pfr0 |= SYS_FIELD_PREP_ENUM(ID_AA64PFR0_EL1, GIC, IMP);
		pfr1 |= SYS_FIELD_PREP_ENUM(ID_PFR1_EL1, GIC, GICv3);
	}

	kvm_set_vm_id_reg(kvm, SYS_ID_AA64PFR0_EL1, aa64pfr0);
	kvm_set_vm_id_reg(kvm, SYS_ID_PFR1_EL1, pfr1);

	kvm_for_each_vcpu(i, vcpu, kvm) {
		ret = vgic_allocate_private_irqs_locked(vcpu, type);
		if (ret)
			break;
	}

	if (ret) {
		kvm_for_each_vcpu(i, vcpu, kvm) {
			struct vgic_cpu *vgic_cpu = &vcpu->arch.vgic_cpu;
			kfree(vgic_cpu->private_irqs);
			vgic_cpu->private_irqs = NULL;
		}

		kvm->arch.vgic.vgic_model = 0;
		goto out_unlock;
	}

	if (type == KVM_DEV_TYPE_ARM_VGIC_V3)
		kvm->arch.vgic.nassgicap = system_supports_direct_sgis();

out_unlock:
	mutex_unlock(&kvm->arch.config_lock);
	kvm_unlock_all_vcpus(kvm);
	return ret;
}
/* Context-After
 * /* INIT/DESTROY */
 * 
 * /**
 *  * kvm_vgic_dist_init: initialize the dist data structures
 *  * @kvm: kvm struct pointer
 *  * @nr_spis: number of spis, frozen by caller
 *  */
 * static int kvm_vgic_dist_init(struct kvm *kvm, unsigned int nr_spis)
 * {
 * 	struct vgic_dist *dist = &kvm->arch.vgic;
 * 	struct kvm_vcpu *vcpu0 = kvm_get_vcpu(kvm, 0);
 * 	int i;
 * 
 * 	dist->active_spis = (atomic_t)ATOMIC_INIT(0);
 * 	dist->spis = kzalloc_objs(struct vgic_irq, nr_spis, GFP_KERNEL_ACCOUNT);
 * 	if (!dist->spis)
 * 		return  -ENOMEM;
 * 
 * 	/*
 */
