/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_021 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 20 */
/* Subsystem: arch */
/* Includes
 * #include <linux/errno.h>
 * #include <linux/err.h>
 * #include <linux/kvm_host.h>
 * #include <linux/wordpart.h>
 * #include <asm/sbi.h>
 * #include <asm/kvm_vcpu_sbi.h>
 */
/* Context-Before
 * // SPDX-License-Identifier: GPL-2.0
 * /*
 *  * Copyright (c) 2021 Western Digital Corporation or its affiliates.
 *  *
 *  * Authors:
 *  *     Atish Patra <atish.patra@wdc.com>
 *  */
 * 
 * #include <linux/errno.h>
 * #include <linux/err.h>
 * #include <linux/kvm_host.h>
 * #include <linux/wordpart.h>
 * #include <asm/sbi.h>
 * #include <asm/kvm_vcpu_sbi.h>
 */
static int kvm_sbi_hsm_vcpu_start(struct kvm_vcpu *vcpu)
{
	struct kvm_cpu_context *cp = &vcpu->arch.guest_context;
	struct kvm_vcpu *target_vcpu;
	unsigned long target_vcpuid = cp->a0;
	int ret = 0;

	target_vcpu = kvm_get_vcpu_by_id(vcpu->kvm, target_vcpuid);
	if (!target_vcpu)
		return SBI_ERR_INVALID_PARAM;

	spin_lock(&target_vcpu->arch.mp_state_lock);

	if (!kvm_riscv_vcpu_stopped(target_vcpu)) {
		ret = SBI_ERR_ALREADY_AVAILABLE;
		goto out;
	}

	kvm_riscv_vcpu_sbi_request_reset(target_vcpu, cp->a1, cp->a2);

	__kvm_riscv_vcpu_power_on(target_vcpu);

out:
	spin_unlock(&target_vcpu->arch.mp_state_lock);

	return ret;
}
/* Context-After
 * static int kvm_sbi_hsm_vcpu_stop(struct kvm_vcpu *vcpu)
 * {
 * 	int ret = 0;
 * 
 * 	spin_lock(&vcpu->arch.mp_state_lock);
 * 
 * 	if (kvm_riscv_vcpu_stopped(vcpu)) {
 * 		ret = SBI_ERR_FAILURE;
 * 		goto out;
 * 	}
 * 
 * 	__kvm_riscv_vcpu_power_off(vcpu);
 * 
 * out:
 * 	spin_unlock(&vcpu->arch.mp_state_lock);
 * 
 * 	return ret;
 * }
 */
