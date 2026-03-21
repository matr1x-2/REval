/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_023 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 5 */
/* NLOC: 36 */
/* Subsystem: net */
/* Includes
 * #include <linux/tcp.h>
 * #include <linux/slab.h>
 * #include <linux/sunrpc/xprt.h>
 * #include <linux/export.h>
 * #include <linux/sunrpc/bc_xprt.h>
 */
/* Context-Before
 *  * incoming callback request.  It's up to the higher levels in the
 *  * stack to enforce that the maximum number of session slots is not
 *  * being exceeded.
 *  *
 *  * Some callback arguments can be large.  For example, a pNFS server
 *  * using multiple deviceids.  The list can be unbound, but the client
 *  * has the ability to tell the server the maximum size of the callback
 *  * requests.  Each deviceID is 16 bytes, so allocate one page
 *  * for the arguments to have enough room to receive a number of these
 *  * deviceIDs.  The NFS client indicates to the pNFS server that its
 *  * callback requests can be up to 4096 bytes in size.
 *  */
 * int xprt_setup_backchannel(struct rpc_xprt *xprt, unsigned int min_reqs)
 * {
 * 	if (!xprt->ops->bc_setup)
 * 		return 0;
 * 	return xprt->ops->bc_setup(xprt, min_reqs);
 * }
 * EXPORT_SYMBOL_GPL(xprt_setup_backchannel);
 */
int xprt_setup_bc(struct rpc_xprt *xprt, unsigned int min_reqs)
{
	struct rpc_rqst *req;
	LIST_HEAD(tmp_list);
	int i;

	dprintk("RPC:       setup backchannel transport\n");

	if (min_reqs > BC_MAX_SLOTS)
		min_reqs = BC_MAX_SLOTS;

	/*
	 * We use a temporary list to keep track of the preallocated
	 * buffers.  Once we're done building the list we splice it
	 * into the backchannel preallocation list off of the rpc_xprt
	 * struct.  This helps minimize the amount of time the list
	 * lock is held on the rpc_xprt struct.  It also makes cleanup
	 * easier in case of memory allocation errors.
	 */
	for (i = 0; i < min_reqs; i++) {
		/* Pre-allocate one backchannel rpc_rqst */
		req = xprt_alloc_bc_req(xprt);
		if (req == NULL) {
			printk(KERN_ERR "Failed to create bc rpc_rqst\n");
			goto out_free;
		}

		/* Add the allocated buffer to the tmp list */
		dprintk("RPC:       adding req= %p\n", req);
		list_add(&req->rq_bc_pa_list, &tmp_list);
	}

	/*
	 * Add the temporary list to the backchannel preallocation list
	 */
	spin_lock(&xprt->bc_pa_lock);
	list_splice(&tmp_list, &xprt->bc_pa_list);
	xprt->bc_alloc_count += min_reqs;
	xprt->bc_alloc_max += min_reqs;
	atomic_add(min_reqs, &xprt->bc_slot_count);
	spin_unlock(&xprt->bc_pa_lock);

	dprintk("RPC:       setup backchannel transport done\n");
	return 0;

out_free:
	/*
	 * Memory allocation failed, free the temporary list
	 */
	while (!list_empty(&tmp_list)) {
		req = list_first_entry(&tmp_list,
				struct rpc_rqst,
				rq_bc_pa_list);
		list_del(&req->rq_bc_pa_list);
		xprt_free_allocation(req);
	}

	dprintk("RPC:       setup backchannel transport failed\n");
	return -ENOMEM;
}
/* Context-After
 * /**
 *  * xprt_destroy_backchannel - Destroys the backchannel preallocated structures.
 *  * @xprt:	the transport holding the preallocated strucures
 *  * @max_reqs:	the maximum number of preallocated structures to destroy
 *  *
 *  * Since these structures may have been allocated by multiple calls
 *  * to xprt_setup_backchannel, we only destroy up to the maximum number
 *  * of reqs specified by the caller.
 *  */
 * void xprt_destroy_backchannel(struct rpc_xprt *xprt, unsigned int max_reqs)
 * {
 * 	if (xprt->ops->bc_destroy)
 * 		xprt->ops->bc_destroy(xprt, max_reqs);
 * }
 * EXPORT_SYMBOL_GPL(xprt_destroy_backchannel);
 * 
 * void xprt_destroy_bc(struct rpc_xprt *xprt, unsigned int max_reqs)
 * {
 * 	struct rpc_rqst *req = NULL, *tmp = NULL;
 */
