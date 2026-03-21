/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_031 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 52 */
/* NLOC: 148 */
/* Subsystem: arch */
/* Includes
 * #include <linux/kvm_host.h>
 * #include "irq.h"
 * #include "ioapic.h"
 * #include "mmu.h"
 * #include "i8254.h"
 * #include "tss.h"
 * #include "kvm_cache_regs.h"
 * #include "kvm_emulate.h"
 * #include "mmu/page_track.h"
 * #include "x86.h"
 * #include "cpuid.h"
 * #include "pmu.h"
 * #include "hyperv.h"
 * #include "lapic.h"
 * #include "xen.h"
 * #include "smm.h"
 * #include <linux/clocksource.h>
 * #include <linux/interrupt.h>
 * #include <linux/kvm.h>
 * #include <linux/fs.h>
 */
/* Context-Before
 *  * [*] Except #MC, which is higher priority, but KVM should never emulate in
 *  *     response to a machine check.
 *  */
 * int x86_decode_emulated_instruction(struct kvm_vcpu *vcpu, int emulation_type,
 * 				    void *insn, int insn_len)
 * {
 * 	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;
 * 	int r;
 * 
 * 	init_emulate_ctxt(vcpu);
 * 
 * 	r = x86_decode_insn(ctxt, insn, insn_len, emulation_type);
 * 
 * 	trace_kvm_emulate_insn_start(vcpu);
 * 	++vcpu->stat.insn_emulation;
 * 
 * 	return r;
 * }
 * EXPORT_SYMBOL_FOR_KVM_INTERNAL(x86_decode_emulated_instruction);
 */
int x86_emulate_instruction(struct kvm_vcpu *vcpu, gpa_t cr2_or_gpa,
			    int emulation_type, void *insn, int insn_len)
{
	int r;
	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;
	bool writeback = true;

	if ((emulation_type & EMULTYPE_ALLOW_RETRY_PF) &&
	    (WARN_ON_ONCE(is_guest_mode(vcpu)) ||
	     WARN_ON_ONCE(!(emulation_type & EMULTYPE_PF))))
		emulation_type &= ~EMULTYPE_ALLOW_RETRY_PF;

	r = kvm_check_emulate_insn(vcpu, emulation_type, insn, insn_len);
	if (r != X86EMUL_CONTINUE) {
		if (r == X86EMUL_RETRY_INSTR || r == X86EMUL_PROPAGATE_FAULT)
			return 1;

		if (kvm_unprotect_and_retry_on_failure(vcpu, cr2_or_gpa,
						       emulation_type))
			return 1;

		if (r == X86EMUL_UNHANDLEABLE_VECTORING) {
			kvm_prepare_event_vectoring_exit(vcpu, cr2_or_gpa);
			return 0;
		}

		WARN_ON_ONCE(r != X86EMUL_UNHANDLEABLE);
		return handle_emulation_failure(vcpu, emulation_type);
	}

	kvm_request_l1tf_flush_l1d();

	if (!(emulation_type & EMULTYPE_NO_DECODE)) {
		kvm_clear_exception_queue(vcpu);

		/*
		 * Return immediately if RIP hits a code breakpoint, such #DBs
		 * are fault-like and are higher priority than any faults on
		 * the code fetch itself.
		 */
		if (kvm_vcpu_check_code_breakpoint(vcpu, emulation_type, &r))
			return r;

		r = x86_decode_emulated_instruction(vcpu, emulation_type,
						    insn, insn_len);
		if (r != EMULATION_OK)  {
			if ((emulation_type & EMULTYPE_TRAP_UD) ||
			    (emulation_type & EMULTYPE_TRAP_UD_FORCED)) {
				kvm_queue_exception(vcpu, UD_VECTOR);
				return 1;
			}
			if (kvm_unprotect_and_retry_on_failure(vcpu, cr2_or_gpa,
							       emulation_type))
				return 1;

			if (ctxt->have_exception &&
			    !(emulation_type & EMULTYPE_SKIP)) {
				/*
				 * #UD should result in just EMULATION_FAILED, and trap-like
				 * exception should not be encountered during decode.
				 */
				WARN_ON_ONCE(ctxt->exception.vector == UD_VECTOR ||
					     exception_type(ctxt->exception.vector) == EXCPT_TRAP);
				inject_emulated_exception(vcpu);
				return 1;
			}
			return handle_emulation_failure(vcpu, emulation_type);
		}
	}

	if ((emulation_type & EMULTYPE_VMWARE_GP) &&
	    !is_vmware_backdoor_opcode(ctxt)) {
		kvm_queue_exception_e(vcpu, GP_VECTOR, 0);
		return 1;
	}

	/*
	 * EMULTYPE_SKIP without EMULTYPE_COMPLETE_USER_EXIT is intended for
	 * use *only* by vendor callbacks for kvm_skip_emulated_instruction().
	 * The caller is responsible for updating interruptibility state and
	 * injecting single-step #DBs.
	 */
	if (emulation_type & EMULTYPE_SKIP) {
		if (emulation_type & EMULTYPE_SKIP_SOFT_INT &&
		    !is_soft_int_instruction(ctxt, emulation_type))
			return 0;

		if (ctxt->mode != X86EMUL_MODE_PROT64)
			ctxt->eip = (u32)ctxt->_eip;
		else
			ctxt->eip = ctxt->_eip;

		if (emulation_type & EMULTYPE_COMPLETE_USER_EXIT) {
			r = 1;
			goto writeback;
		}

		kvm_rip_write(vcpu, ctxt->eip);
		if (ctxt->eflags & X86_EFLAGS_RF)
			kvm_set_rflags(vcpu, ctxt->eflags & ~X86_EFLAGS_RF);
		return 1;
	}

	/*
	 * If emulation was caused by a write-protection #PF on a non-page_table
	 * writing instruction, try to unprotect the gfn, i.e. zap shadow pages,
	 * and retry the instruction, as the vCPU is likely no longer using the
	 * gfn as a page table.
	 */
	if ((emulation_type & EMULTYPE_ALLOW_RETRY_PF) &&
	    !x86_page_table_writing_insn(ctxt) &&
	    kvm_mmu_unprotect_gfn_and_retry(vcpu, cr2_or_gpa))
		return 1;

	/* this is needed for vmware backdoor interface to work since it
	   changes registers values  during IO operation */
	if (vcpu->arch.emulate_regs_need_sync_from_vcpu) {
		vcpu->arch.emulate_regs_need_sync_from_vcpu = false;
		emulator_invalidate_register_cache(ctxt);
	}

restart:
	if (emulation_type & EMULTYPE_PF) {
		/* Save the faulting GPA (cr2) in the address field */
		ctxt->exception.address = cr2_or_gpa;

		/* With shadow page tables, cr2 contains a GVA or nGPA. */
		if (vcpu->arch.mmu->root_role.direct) {
			ctxt->gpa_available = true;
			ctxt->gpa_val = cr2_or_gpa;
		}
	} else {
		/* Sanitize the address out of an abundance of paranoia. */
		ctxt->exception.address = 0;
	}

	/*
	 * Check L1's instruction intercepts when emulating instructions for
	 * L2, unless KVM is re-emulating a previously decoded instruction,
	 * e.g. to complete userspace I/O, in which case KVM has already
	 * checked the intercepts.
	 */
	r = x86_emulate_insn(ctxt, is_guest_mode(vcpu) &&
				   !(emulation_type & EMULTYPE_NO_DECODE));

	if (r == EMULATION_INTERCEPTED)
		return 1;

	if (r == EMULATION_FAILED) {
		if (kvm_unprotect_and_retry_on_failure(vcpu, cr2_or_gpa,
						       emulation_type))
			return 1;

		return handle_emulation_failure(vcpu, emulation_type);
	}

	if (ctxt->have_exception) {
		WARN_ON_ONCE(vcpu->mmio_needed && !vcpu->mmio_is_write);
		vcpu->mmio_needed = false;
		r = 1;
		inject_emulated_exception(vcpu);
	} else if (vcpu->arch.pio.count) {
		if (!vcpu->arch.pio.in) {
			/* FIXME: return into emulator if single-stepping.  */
			vcpu->arch.pio.count = 0;
		} else {
			writeback = false;
			vcpu->arch.complete_userspace_io = complete_emulated_pio;
		}
		r = 0;
	} else if (vcpu->mmio_needed) {
		++vcpu->stat.mmio_exits;

		if (!vcpu->mmio_is_write)
			writeback = false;
		r = 0;
		vcpu->arch.complete_userspace_io = complete_emulated_mmio;
	} else if (vcpu->arch.complete_userspace_io) {
		writeback = false;
		r = 0;
	} else if (r == EMULATION_RESTART)
		goto restart;
	else
		r = 1;

writeback:
	if (writeback) {
		unsigned long rflags = kvm_x86_call(get_rflags)(vcpu);
		toggle_interruptibility(vcpu, ctxt->interruptibility);
		vcpu->arch.emulate_regs_need_sync_to_vcpu = false;

		/*
		 * Note, EXCPT_DB is assumed to be fault-like as the emulator
		 * only supports code breakpoints and general detect #DB, both
		 * of which are fault-like.
		 */
		if (!ctxt->have_exception ||
		    exception_type(ctxt->exception.vector) == EXCPT_TRAP) {
			kvm_pmu_instruction_retired(vcpu);
			if (ctxt->is_branch)
				kvm_pmu_branch_retired(vcpu);
			kvm_rip_write(vcpu, ctxt->eip);
			if (r && (ctxt->tf || (vcpu->guest_debug & KVM_GUESTDBG_SINGLESTEP)))
				r = kvm_vcpu_do_singlestep(vcpu);
			kvm_x86_call(update_emulated_instruction)(vcpu);
			__kvm_set_rflags(vcpu, ctxt->eflags);
		}

		/*
		 * For STI, interrupts are shadowed; so KVM_REQ_EVENT will
		 * do nothing, and it will be requested again as soon as
		 * the shadow expires.  But we still need to check here,
		 * because POPF has no interrupt shadow.
		 */
		if (unlikely((ctxt->eflags & ~rflags) & X86_EFLAGS_IF))
			kvm_make_request(KVM_REQ_EVENT, vcpu);
	} else
		vcpu->arch.emulate_regs_need_sync_to_vcpu = true;

	return r;
}
/* Context-After
 * int kvm_emulate_instruction(struct kvm_vcpu *vcpu, int emulation_type)
 * {
 * 	return x86_emulate_instruction(vcpu, 0, emulation_type, NULL, 0);
 * }
 * EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_emulate_instruction);
 * 
 * int kvm_emulate_instruction_from_buffer(struct kvm_vcpu *vcpu,
 * 					void *insn, int insn_len)
 * {
 * 	return x86_emulate_instruction(vcpu, 0, 0, insn, insn_len);
 * }
 * EXPORT_SYMBOL_FOR_KVM_INTERNAL(kvm_emulate_instruction_from_buffer);
 * 
 * static int complete_fast_pio_out_port_0x7e(struct kvm_vcpu *vcpu)
 * {
 * 	vcpu->arch.pio.count = 0;
 * 	return 1;
 * }
 */
