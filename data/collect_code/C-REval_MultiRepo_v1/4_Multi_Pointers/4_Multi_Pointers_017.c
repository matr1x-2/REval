/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_017 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 4 */
/* NLOC: 23 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/init.h>
 * #include <linux/module.h>
 * #include <linux/slab.h>
 * #include <linux/pci.h>
 * #include <linux/interrupt.h>
 * #include <linux/delay.h>
 * #include <linux/wait.h>
 * #include "../pci.h"
 * #include <asm/pci_x86.h>		/* for struct irq_routing_table */
 * #include <asm/io_apic.h>
 * #include "ibmphp.h"
 */
/* Context-Before
 * #define DRIVER_VERSION	"0.6"
 * #define DRIVER_DESC	"IBM Hot Plug PCI Controller Driver"
 * 
 * int ibmphp_debug;
 * 
 * static bool debug;
 * module_param(debug, bool, S_IRUGO | S_IWUSR);
 * MODULE_PARM_DESC(debug, "Debugging mode enabled or not");
 * MODULE_LICENSE("GPL");
 * MODULE_DESCRIPTION(DRIVER_DESC);
 * 
 * struct pci_bus *ibmphp_pci_bus;
 * static int max_slots;
 * 
 * static int irqs[16];    /* PIC mode IRQs we're using so far (in case MPS
 * 			 * tables don't provide default info for empty slots */
 * 
 * static int init_flag;
 */
static inline int get_cur_bus_info(struct slot **sl)
{
	int rc = 1;
	struct slot *slot_cur = *sl;

	debug("options = %x\n", slot_cur->ctrl->options);
	debug("revision = %x\n", slot_cur->ctrl->revision);

	if (READ_BUS_STATUS(slot_cur->ctrl))
		rc = ibmphp_hpc_readslot(slot_cur, READ_BUSSTATUS, NULL);

	if (rc)
		return rc;

	slot_cur->bus_on->current_speed = CURRENT_BUS_SPEED(slot_cur->busstatus);
	if (READ_BUS_MODE(slot_cur->ctrl))
		slot_cur->bus_on->current_bus_mode =
				CURRENT_BUS_MODE(slot_cur->busstatus);
	else
		slot_cur->bus_on->current_bus_mode = 0xFF;

	debug("busstatus = %x, bus_speed = %x, bus_mode = %x\n",
			slot_cur->busstatus,
			slot_cur->bus_on->current_speed,
			slot_cur->bus_on->current_bus_mode);

	*sl = slot_cur;
	return 0;
}
/* Context-After
 * static inline int slot_update(struct slot **sl)
 * {
 * 	int rc;
 * 	rc = ibmphp_hpc_readslot(*sl, READ_ALLSTAT, NULL);
 * 	if (rc)
 * 		return rc;
 * 	if (!init_flag)
 * 		rc = get_cur_bus_info(sl);
 * 	return rc;
 * }
 * 
 * static int __init get_max_slots(void)
 * {
 * 	struct slot *slot_cur;
 * 	u8 slot_count = 0;
 * 
 * 	list_for_each_entry(slot_cur, &ibmphp_slot_head, ibm_slot_list) {
 * 		/* sometimes the hot-pluggable slots start with 4 (not always from 1) */
 * 		slot_count = max(slot_count, slot_cur->number);
 */
