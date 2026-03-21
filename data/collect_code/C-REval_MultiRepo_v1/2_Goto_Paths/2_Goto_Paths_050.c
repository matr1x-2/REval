/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_050 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 6 */
/* NLOC: 44 */
/* Subsystem: drivers */
/* Includes
 * #include "mpi3mr.h"
 * #include <linux/io-64-nonatomic-lo-hi.h>
 */
/* Context-Before
 * 	}
 * 	mem_desc = &mrioc->ioctl_resp_sge;
 * 	if (mem_desc->addr) {
 * 		dma_free_coherent(&mrioc->pdev->dev, mem_desc->size,
 * 				  mem_desc->addr, mem_desc->dma_addr);
 * 		mem_desc->addr = NULL;
 * 	}
 * 
 * 	mrioc->ioctl_sges_allocated = false;
 * }
 * 
 * /**
 *  * mpi3mr_alloc_ioctl_dma_memory - Alloc memory for ioctl dma
 *  * @mrioc: Adapter instance reference
 *  *
 *  * This function allocates dmaable memory required to handle the
 *  * application issued MPI3 IOCTL requests.
 *  *
 *  * Return: None
 *  */
 */
static void mpi3mr_alloc_ioctl_dma_memory(struct mpi3mr_ioc *mrioc)

{
	struct dma_memory_desc *mem_desc;
	u16 i;

	mrioc->ioctl_dma_pool = dma_pool_create("ioctl dma pool",
						&mrioc->pdev->dev,
						MPI3MR_IOCTL_SGE_SIZE,
						MPI3MR_PAGE_SIZE_4K, 0);

	if (!mrioc->ioctl_dma_pool) {
		ioc_err(mrioc, "ioctl_dma_pool: dma_pool_create failed\n");
		goto out_failed;
	}

	for (i = 0; i < MPI3MR_NUM_IOCTL_SGE; i++) {
		mem_desc = &mrioc->ioctl_sge[i];
		mem_desc->size = MPI3MR_IOCTL_SGE_SIZE;
		mem_desc->addr = dma_pool_zalloc(mrioc->ioctl_dma_pool,
						 GFP_KERNEL,
						 &mem_desc->dma_addr);
		if (!mem_desc->addr)
			goto out_failed;
	}

	mem_desc = &mrioc->ioctl_chain_sge;
	mem_desc->size = MPI3MR_PAGE_SIZE_4K;
	mem_desc->addr = dma_alloc_coherent(&mrioc->pdev->dev,
					    mem_desc->size,
					    &mem_desc->dma_addr,
					    GFP_KERNEL);
	if (!mem_desc->addr)
		goto out_failed;

	mem_desc = &mrioc->ioctl_resp_sge;
	mem_desc->size = MPI3MR_PAGE_SIZE_4K;
	mem_desc->addr = dma_alloc_coherent(&mrioc->pdev->dev,
					    mem_desc->size,
					    &mem_desc->dma_addr,
					    GFP_KERNEL);
	if (!mem_desc->addr)
		goto out_failed;

	mrioc->ioctl_sges_allocated = true;

	return;
out_failed:
	ioc_warn(mrioc, "cannot allocate DMA memory for the mpt commands\n"
		 "from the applications, application interface for MPT command is disabled\n");
	mpi3mr_free_ioctl_dma_memory(mrioc);
}
/* Context-After
 * /**
 *  * mpi3mr_fault_uevent_emit - Emit uevent for any controller
 *  * fault
 *  * @mrioc: Pointer to the mpi3mr_ioc structure for the controller instance
 *  *
 *  * This function is invoked when the controller undergoes any
 *  * type of fault.
 *  */
 * 
 * static void mpi3mr_fault_uevent_emit(struct mpi3mr_ioc *mrioc)
 * {
 * 	struct kobj_uevent_env *env;
 * 	int ret;
 * 
 * 	env = kzalloc_obj(*env);
 * 	if (!env)
 * 		return;
 * 
 * 	ret = add_uevent_var(env, "DRIVER=%s", mrioc->driver_name);
 */
