/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_014 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 46 */
/* NLOC: 195 */
/* Subsystem: mm */
/* Includes
 * #include <linux/mm.h>
 * #include <linux/sched/mm.h>
 * #include <linux/sched/task.h>
 * #include <linux/pagemap.h>
 * #include <linux/swap.h>
 * #include <linux/leafops.h>
 * #include <linux/slab.h>
 * #include <linux/init.h>
 * #include <linux/ksm.h>
 * #include <linux/rmap.h>
 * #include <linux/rcupdate.h>
 * #include <linux/export.h>
 * #include <linux/memcontrol.h>
 * #include <linux/mmu_notifier.h>
 * #include <linux/migrate.h>
 * #include <linux/hugetlb.h>
 * #include <linux/huge_mm.h>
 * #include <linux/backing-dev.h>
 * #include <linux/page_idle.h>
 * #include <linux/memremap.h>
 */
/* Context-Before
 * {
 * 	struct rmap_walk_control rwc = {
 * 		.rmap_one = try_to_unmap_one,
 * 		.arg = (void *)flags,
 * 		.done = folio_not_mapped,
 * 		.anon_lock = folio_lock_anon_vma_read,
 * 	};
 * 
 * 	if (flags & TTU_RMAP_LOCKED)
 * 		rmap_walk_locked(folio, &rwc);
 * 	else
 * 		rmap_walk(folio, &rwc);
 * }
 * 
 * /*
 *  * @arg: enum ttu_flags will be passed to this argument.
 *  *
 *  * If TTU_SPLIT_HUGE_PMD is specified any PMD mappings will be split into PTEs
 *  * containing migration entries.
 *  */
 */
static bool try_to_migrate_one(struct folio *folio, struct vm_area_struct *vma,
		     unsigned long address, void *arg)
{
	struct mm_struct *mm = vma->vm_mm;
	DEFINE_FOLIO_VMA_WALK(pvmw, folio, vma, address, 0);
	bool anon_exclusive, writable, ret = true;
	pte_t pteval;
	struct page *subpage;
	struct mmu_notifier_range range;
	enum ttu_flags flags = (enum ttu_flags)(long)arg;
	unsigned long pfn;
	unsigned long hsz = 0;

	/*
	 * When racing against e.g. zap_pte_range() on another cpu,
	 * in between its ptep_get_and_clear_full() and folio_remove_rmap_*(),
	 * try_to_migrate() may return before page_mapped() has become false,
	 * if page table locking is skipped: use TTU_SYNC to wait for that.
	 */
	if (flags & TTU_SYNC)
		pvmw.flags = PVMW_SYNC;

	/*
	 * For THP, we have to assume the worse case ie pmd for invalidation.
	 * For hugetlb, it could be much worse if we need to do pud
	 * invalidation in the case of pmd sharing.
	 *
	 * Note that the page can not be free in this function as call of
	 * try_to_unmap() must hold a reference on the page.
	 */
	range.end = vma_address_end(&pvmw);
	mmu_notifier_range_init(&range, MMU_NOTIFY_CLEAR, 0, vma->vm_mm,
				address, range.end);
	if (folio_test_hugetlb(folio)) {
		/*
		 * If sharing is possible, start and end will be adjusted
		 * accordingly.
		 */
		adjust_range_if_pmd_sharing_possible(vma, &range.start,
						     &range.end);

		/* We need the huge page size for set_huge_pte_at() */
		hsz = huge_page_size(hstate_vma(vma));
	}
	mmu_notifier_invalidate_range_start(&range);

	while (page_vma_mapped_walk(&pvmw)) {
		/* PMD-mapped THP migration entry */
		if (!pvmw.pte) {
			__maybe_unused unsigned long pfn;
			__maybe_unused pmd_t pmdval;

			if (flags & TTU_SPLIT_HUGE_PMD) {
				/*
				 * split_huge_pmd_locked() might leave the
				 * folio mapped through PTEs. Retry the walk
				 * so we can detect this scenario and properly
				 * abort the walk.
				 */
				split_huge_pmd_locked(vma, pvmw.address,
						      pvmw.pmd, true);
				flags &= ~TTU_SPLIT_HUGE_PMD;
				page_vma_mapped_walk_restart(&pvmw);
				continue;
			}
#ifdef CONFIG_ARCH_ENABLE_THP_MIGRATION
			pmdval = pmdp_get(pvmw.pmd);
			if (likely(pmd_present(pmdval)))
				pfn = pmd_pfn(pmdval);
			else
				pfn = softleaf_to_pfn(softleaf_from_pmd(pmdval));

			subpage = folio_page(folio, pfn - folio_pfn(folio));

			VM_BUG_ON_FOLIO(folio_test_hugetlb(folio) ||
					!folio_test_pmd_mappable(folio), folio);

			if (set_pmd_migration_entry(&pvmw, subpage)) {
				ret = false;
				page_vma_mapped_walk_done(&pvmw);
				break;
			}
			continue;
#endif
		}

		/* Unexpected PMD-mapped THP? */
		VM_BUG_ON_FOLIO(!pvmw.pte, folio);

		/*
		 * Handle PFN swap PTEs, such as device-exclusive ones, that
		 * actually map pages.
		 */
		pteval = ptep_get(pvmw.pte);
		if (likely(pte_present(pteval))) {
			pfn = pte_pfn(pteval);
		} else {
			const softleaf_t entry = softleaf_from_pte(pteval);

			pfn = softleaf_to_pfn(entry);
			VM_WARN_ON_FOLIO(folio_test_hugetlb(folio), folio);
		}

		subpage = folio_page(folio, pfn - folio_pfn(folio));
		address = pvmw.address;
		anon_exclusive = folio_test_anon(folio) &&
				 PageAnonExclusive(subpage);

		if (folio_test_hugetlb(folio)) {
			bool anon = folio_test_anon(folio);

			/*
			 * huge_pmd_unshare may unmap an entire PMD page.
			 * There is no way of knowing exactly which PMDs may
			 * be cached for this mm, so we must flush them all.
			 * start/end were already adjusted above to cover this
			 * range.
			 */
			flush_cache_range(vma, range.start, range.end);

			/*
			 * To call huge_pmd_unshare, i_mmap_rwsem must be
			 * held in write mode.  Caller needs to explicitly
			 * do this outside rmap routines.
			 *
			 * We also must hold hugetlb vma_lock in write mode.
			 * Lock order dictates acquiring vma_lock BEFORE
			 * i_mmap_rwsem.  We can only try lock here and
			 * fail if unsuccessful.
			 */
			if (!anon) {
				struct mmu_gather tlb;

				VM_BUG_ON(!(flags & TTU_RMAP_LOCKED));
				if (!hugetlb_vma_trylock_write(vma)) {
					page_vma_mapped_walk_done(&pvmw);
					ret = false;
					break;
				}

				tlb_gather_mmu_vma(&tlb, vma);
				if (huge_pmd_unshare(&tlb, vma, address, pvmw.pte)) {
					hugetlb_vma_unlock_write(vma);
					huge_pmd_unshare_flush(&tlb, vma);
					tlb_finish_mmu(&tlb);
					/*
					 * The PMD table was unmapped,
					 * consequently unmapping the folio.
					 */
					page_vma_mapped_walk_done(&pvmw);
					break;
				}
				hugetlb_vma_unlock_write(vma);
				tlb_finish_mmu(&tlb);
			}
			/* Nuke the hugetlb page table entry */
			pteval = huge_ptep_clear_flush(vma, address, pvmw.pte);
			if (pte_dirty(pteval))
				folio_mark_dirty(folio);
			writable = pte_write(pteval);
		} else if (likely(pte_present(pteval))) {
			flush_cache_page(vma, address, pfn);
			/* Nuke the page table entry. */
			if (should_defer_flush(mm, flags)) {
				/*
				 * We clear the PTE but do not flush so potentially
				 * a remote CPU could still be writing to the folio.
				 * If the entry was previously clean then the
				 * architecture must guarantee that a clear->dirty
				 * transition on a cached TLB entry is written through
				 * and traps if the PTE is unmapped.
				 */
				pteval = ptep_get_and_clear(mm, address, pvmw.pte);

				set_tlb_ubc_flush_pending(mm, pteval, address, address + PAGE_SIZE);
			} else {
				pteval = ptep_clear_flush(vma, address, pvmw.pte);
			}
			if (pte_dirty(pteval))
				folio_mark_dirty(folio);
			writable = pte_write(pteval);
		} else {
			const softleaf_t entry = softleaf_from_pte(pteval);

			pte_clear(mm, address, pvmw.pte);

			writable = softleaf_is_device_private_write(entry);
		}

		VM_WARN_ON_FOLIO(writable && folio_test_anon(folio) &&
				!anon_exclusive, folio);

		/* Update high watermark before we lower rss */
		update_hiwater_rss(mm);

		if (PageHWPoison(subpage)) {
			VM_WARN_ON_FOLIO(folio_is_device_private(folio), folio);

			pteval = swp_entry_to_pte(make_hwpoison_entry(subpage));
			if (folio_test_hugetlb(folio)) {
				hugetlb_count_sub(folio_nr_pages(folio), mm);
				set_huge_pte_at(mm, address, pvmw.pte, pteval,
						hsz);
			} else {
				dec_mm_counter(mm, mm_counter(folio));
				set_pte_at(mm, address, pvmw.pte, pteval);
			}
		} else if (likely(pte_present(pteval)) && pte_unused(pteval) &&
			   !userfaultfd_armed(vma)) {
			/*
			 * The guest indicated that the page content is of no
			 * interest anymore. Simply discard the pte, vmscan
			 * will take care of the rest.
			 * A future reference will then fault in a new zero
			 * page. When userfaultfd is active, we must not drop
			 * this page though, as its main user (postcopy
			 * migration) will not expect userfaults on already
			 * copied pages.
			 */
			dec_mm_counter(mm, mm_counter(folio));
		} else {
			swp_entry_t entry;
			pte_t swp_pte;

			/*
			 * arch_unmap_one() is expected to be a NOP on
			 * architectures where we could have PFN swap PTEs,
			 * so we'll not check/care.
			 */
			if (arch_unmap_one(mm, vma, address, pteval) < 0) {
				if (folio_test_hugetlb(folio))
					set_huge_pte_at(mm, address, pvmw.pte,
							pteval, hsz);
				else
					set_pte_at(mm, address, pvmw.pte, pteval);
				ret = false;
				page_vma_mapped_walk_done(&pvmw);
				break;
			}

			/* See folio_try_share_anon_rmap_pte(): clear PTE first. */
			if (folio_test_hugetlb(folio)) {
				if (anon_exclusive &&
				    hugetlb_try_share_anon_rmap(folio)) {
					set_huge_pte_at(mm, address, pvmw.pte,
							pteval, hsz);
					ret = false;
					page_vma_mapped_walk_done(&pvmw);
					break;
				}
			} else if (anon_exclusive &&
				   folio_try_share_anon_rmap_pte(folio, subpage)) {
				set_pte_at(mm, address, pvmw.pte, pteval);
				ret = false;
				page_vma_mapped_walk_done(&pvmw);
				break;
			}

			/*
			 * Store the pfn of the page in a special migration
			 * pte. do_swap_page() will wait until the migration
			 * pte is removed and then restart fault handling.
			 */
			if (writable)
				entry = make_writable_migration_entry(
							page_to_pfn(subpage));
			else if (anon_exclusive)
				entry = make_readable_exclusive_migration_entry(
							page_to_pfn(subpage));
			else
				entry = make_readable_migration_entry(
							page_to_pfn(subpage));
			if (likely(pte_present(pteval))) {
				if (pte_young(pteval))
					entry = make_migration_entry_young(entry);
				if (pte_dirty(pteval))
					entry = make_migration_entry_dirty(entry);
				swp_pte = swp_entry_to_pte(entry);
				if (pte_soft_dirty(pteval))
					swp_pte = pte_swp_mksoft_dirty(swp_pte);
				if (pte_uffd_wp(pteval))
					swp_pte = pte_swp_mkuffd_wp(swp_pte);
			} else {
				swp_pte = swp_entry_to_pte(entry);
				if (pte_swp_soft_dirty(pteval))
					swp_pte = pte_swp_mksoft_dirty(swp_pte);
				if (pte_swp_uffd_wp(pteval))
					swp_pte = pte_swp_mkuffd_wp(swp_pte);
			}
			if (folio_test_hugetlb(folio))
				set_huge_pte_at(mm, address, pvmw.pte, swp_pte,
						hsz);
			else
				set_pte_at(mm, address, pvmw.pte, swp_pte);
			trace_set_migration_pte(address, pte_val(swp_pte),
						folio_order(folio));
			/*
			 * No need to invalidate here it will synchronize on
			 * against the special swap migration pte.
			 */
		}

		if (unlikely(folio_test_hugetlb(folio)))
			hugetlb_remove_rmap(folio);
		else
			folio_remove_rmap_pte(folio, subpage, vma);
		if (vma->vm_flags & VM_LOCKED)
			mlock_drain_local();
		folio_put(folio);
	}

	mmu_notifier_invalidate_range_end(&range);

	return ret;
}
/* Context-After
 * /**
 *  * try_to_migrate - try to replace all page table mappings with swap entries
 *  * @folio: the folio to replace page table entries for
 *  * @flags: action and flags
 *  *
 *  * Tries to remove all the page table entries which are mapping this folio and
 *  * replace them with special swap entries. Caller must hold the folio lock.
 *  */
 * void try_to_migrate(struct folio *folio, enum ttu_flags flags)
 * {
 * 	struct rmap_walk_control rwc = {
 * 		.rmap_one = try_to_migrate_one,
 * 		.arg = (void *)flags,
 * 		.done = folio_not_mapped,
 * 		.anon_lock = folio_lock_anon_vma_read,
 * 	};
 * 
 * 	/*
 * 	 * Migration always ignores mlock and only supports TTU_RMAP_LOCKED and
 */
