/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_019 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 24 */
/* Subsystem: net */
/* Includes
 * #include <linux/tcp.h>
 * #include <linux/slab.h>
 * #include <linux/sunrpc/xprt.h>
 * #include <linux/export.h>
 * #include <linux/sunrpc/bc_xprt.h>
 */
/* Context-Before
 * 		 * to add back to the list because there is no need to
 * 		 * have anymore preallocated entries.
 * 		 */
 * 		dprintk("RPC:       Last session removed req=%p\n", req);
 * 		xprt_free_allocation(req);
 * 	}
 * 	xprt_put(xprt);
 * }
 * 
 * /*
 *  * One or more rpc_rqst structure have been preallocated during the
 *  * backchannel setup.  Buffer space for the send and private XDR buffers
 *  * has been preallocated as well.  Use xprt_alloc_bc_request to allocate
 *  * to this request.  Use xprt_free_bc_request to return it.
 *  *
 *  * We know that we're called in soft interrupt context, grab the spin_lock
 *  * since there is no need to grab the bottom half spin_lock.
 *  *
 *  * Return an available rpc_rqst, otherwise NULL if non are available.
 *  */
 */
struct rpc_rqst *xprt_lookup_bc_request(struct rpc_xprt *xprt, __be32 xid)
{
	struct rpc_rqst *req, *new = NULL;

	do {
		spin_lock(&xprt->bc_pa_lock);
		list_for_each_entry(req, &xprt->bc_pa_list, rq_bc_pa_list) {
			if (req->rq_connect_cookie != xprt->connect_cookie)
				continue;
			if (req->rq_xid == xid)
				goto found;
		}
		req = xprt_get_bc_request(xprt, xid, new);
found:
		spin_unlock(&xprt->bc_pa_lock);
		if (new) {
			if (req != new)
				xprt_free_allocation(new);
			break;
		} else if (req)
			break;
		new = xprt_alloc_bc_req(xprt);
	} while (new);
	return req;
}
/* Context-After
 * /*
 *  * Add callback request to callback list.  Wake a thread
 *  * on the first pool (usually the only pool) to handle it.
 *  */
 * void xprt_complete_bc_request(struct rpc_rqst *req, uint32_t copied)
 * {
 * 	struct rpc_xprt *xprt = req->rq_xprt;
 * 
 * 	spin_lock(&xprt->bc_pa_lock);
 * 	list_del(&req->rq_bc_pa_list);
 * 	xprt->bc_alloc_count--;
 * 	spin_unlock(&xprt->bc_pa_lock);
 * 
 * 	req->rq_private_buf.len = copied;
 * 	set_bit(RPC_BC_PA_IN_USE, &req->rq_bc_pa_state);
 * 
 * 	dprintk("RPC:       add callback request to list\n");
 * 	xprt_enqueue_bc_request(req);
 * }
 */
