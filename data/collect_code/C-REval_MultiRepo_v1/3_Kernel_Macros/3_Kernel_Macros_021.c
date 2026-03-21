/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_021 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 7 */
/* NLOC: 21 */
/* Subsystem: sound */
/* Includes
 * #include <linux/kernel.h>
 * #include <linux/delay.h>
 * #include <linux/export.h>
 * #include <sound/core.h>
 * #include <sound/hdaudio.h>
 * #include <sound/hda_register.h>
 * #include "local.h"
 */
/* Context-Before
 * 	snd_hdac_bus_stop_cmd_io(bus);
 * 
 * 	/* disable position buffer */
 * 	if (bus->posbuf.addr) {
 * 		snd_hdac_chip_writel(bus, DPLBASE, 0);
 * 		snd_hdac_chip_writel(bus, DPUBASE, 0);
 * 	}
 * 
 * 	bus->chip_init = false;
 * }
 * EXPORT_SYMBOL_GPL(snd_hdac_bus_stop_chip);
 * 
 * /**
 *  * snd_hdac_bus_handle_stream_irq - interrupt handler for streams
 *  * @bus: HD-audio core bus
 *  * @status: INTSTS register value
 *  * @ack: callback to be called for woken streams
 *  *
 *  * Returns the bits of handled streams, or zero if no stream is handled.
 *  */
 */
int snd_hdac_bus_handle_stream_irq(struct hdac_bus *bus, unsigned int status,
				    void (*ack)(struct hdac_bus *,
						struct hdac_stream *))
{
	struct hdac_stream *azx_dev;
	u8 sd_status;
	int handled = 0;

	list_for_each_entry(azx_dev, &bus->stream_list, list) {
		if (status & azx_dev->sd_int_sta_mask) {
			sd_status = snd_hdac_stream_readb(azx_dev, SD_STS);
			snd_hdac_stream_writeb(azx_dev, SD_STS, SD_INT_MASK);
			handled |= 1 << azx_dev->index;
			if ((!azx_dev->substream && !azx_dev->cstream) ||
			    !azx_dev->running || !(sd_status & SD_INT_COMPLETE))
				continue;
			if (ack)
				ack(bus, azx_dev);
		}
	}
	return handled;
}
/* Context-After
 * EXPORT_SYMBOL_GPL(snd_hdac_bus_handle_stream_irq);
 * 
 * /**
 *  * snd_hdac_bus_alloc_stream_pages - allocate BDL and other buffers
 *  * @bus: HD-audio core bus
 *  *
 *  * Call this after assigning the all streams.
 *  * Returns zero for success, or a negative error code.
 *  */
 * int snd_hdac_bus_alloc_stream_pages(struct hdac_bus *bus)
 * {
 * 	struct hdac_stream *s;
 * 	int num_streams = 0;
 * 	int dma_type = bus->dma_type ? bus->dma_type : SNDRV_DMA_TYPE_DEV;
 * 	int err;
 * 
 * 	list_for_each_entry(s, &bus->stream_list, list) {
 * 		/* allocate memory for the BDL for each stream */
 * 		err = snd_dma_alloc_pages(dma_type, bus->dev,
 * 					  BDL_SIZE, &s->bdl);
 */
