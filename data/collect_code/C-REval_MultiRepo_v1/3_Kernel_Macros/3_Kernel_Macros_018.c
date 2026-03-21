/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_018 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 3 */
/* NLOC: 20 */
/* Subsystem: fs */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/slab.h>
 * #include "internal.h"
 */
/* Context-Before
 * 	int i;
 * 
 * 	if (old->nr_servers != new->nr_servers ||
 * 	    old->ro_replicating != new->ro_replicating)
 * 		goto changed;
 * 
 * 	for (i = 0; i < old->nr_servers; i++) {
 * 		if (old->servers[i].server != new->servers[i].server)
 * 			goto changed;
 * 		if ((old->servers[i].flags & mask) != (new->servers[i].flags & mask))
 * 			goto changed;
 * 	}
 * 	return false;
 * changed:
 * 	return true;
 * }
 * 
 * /*
 *  * Attach a volume to the servers it is going to use.
 *  */
 */
void afs_attach_volume_to_servers(struct afs_volume *volume, struct afs_server_list *slist)
{
	struct afs_server_entry *se, *pe;
	struct afs_server *server;
	struct list_head *p;
	unsigned int i;

	down_write(&volume->cell->vs_lock);

	for (i = 0; i < slist->nr_servers; i++) {
		se = &slist->servers[i];
		server = se->server;

		list_for_each(p, &server->volumes) {
			pe = list_entry(p, struct afs_server_entry, slink);
			if (volume->vid <= pe->volume->vid)
				break;
		}
		list_add_tail(&se->slink, p);
	}

	slist->attached = true;
	up_write(&volume->cell->vs_lock);
}
/* Context-After
 * /*
 *  * Reattach a volume to the servers it is going to use when server list is
 *  * replaced.  We try to switch the attachment points to avoid rewalking the
 *  * lists.
 *  */
 * void afs_reattach_volume_to_servers(struct afs_volume *volume, struct afs_server_list *new,
 * 				    struct afs_server_list *old)
 * {
 * 	unsigned int n = 0, o = 0;
 * 
 * 	down_write(&volume->cell->vs_lock);
 * 
 * 	while (n < new->nr_servers || o < old->nr_servers) {
 * 		struct afs_server_entry *pn = n < new->nr_servers ? &new->servers[n] : NULL;
 * 		struct afs_server_entry *po = o < old->nr_servers ? &old->servers[o] : NULL;
 * 		struct afs_server_entry *s;
 * 		struct list_head *p;
 * 		int diff;
 */
