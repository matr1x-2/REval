/* C-REval Sample */
/* Sample-ID: 3_Kernel_Macros_026 */
/* Category: 3_Kernel_Macros */
/* Repo: linux */
/* Cyclomatic Complexity: 5 */
/* NLOC: 23 */
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
 * 			snd_hdac_stream_writeb(azx_dev, SD_STS, SD_INT_MASK);
 * 			handled |= 1 << azx_dev->index;
 * 			if ((!azx_dev->substream && !azx_dev->cstream) ||
 * 			    !azx_dev->running || !(sd_status & SD_INT_COMPLETE))
 * 				continue;
 * 			if (ack)
 * 				ack(bus, azx_dev);
 * 		}
 * 	}
 * 	return handled;
 * }
 * EXPORT_SYMBOL_GPL(snd_hdac_bus_handle_stream_irq);
 * 
 * /**
 *  * snd_hdac_bus_alloc_stream_pages - allocate BDL and other buffers
 *  * @bus: HD-audio core bus
 *  *
 *  * Call this after assigning the all streams.
 *  * Returns zero for success, or a negative error code.
 *  */
 */
int snd_hdac_bus_alloc_stream_pages(struct hdac_bus *bus)
{
	struct hdac_stream *s;
	int num_streams = 0;
	int dma_type = bus->dma_type ? bus->dma_type : SNDRV_DMA_TYPE_DEV;
	int err;

	list_for_each_entry(s, &bus->stream_list, list) {
		/* allocate memory for the BDL for each stream */
		err = snd_dma_alloc_pages(dma_type, bus->dev,
					  BDL_SIZE, &s->bdl);
		num_streams++;
		if (err < 0)
			return -ENOMEM;
	}

	if (WARN_ON(!num_streams))
		return -EINVAL;
	/* allocate memory for the position buffer */
	err = snd_dma_alloc_pages(dma_type, bus->dev,
				  num_streams * 8, &bus->posbuf);
	if (err < 0)
		return -ENOMEM;
	list_for_each_entry(s, &bus->stream_list, list)
		s->posbuf = (__le32 *)(bus->posbuf.area + s->index * 8);

	/* single page (at least 4096 bytes) must suffice for both ringbuffes */
	return snd_dma_alloc_pages(dma_type, bus->dev, PAGE_SIZE, &bus->rb);
}
/* Context-After
 * EXPORT_SYMBOL_GPL(snd_hdac_bus_alloc_stream_pages);
 * 
 * /**
 *  * snd_hdac_bus_free_stream_pages - release BDL and other buffers
 *  * @bus: HD-audio core bus
 *  */
 * void snd_hdac_bus_free_stream_pages(struct hdac_bus *bus)
 * {
 * 	struct hdac_stream *s;
 * 
 * 	list_for_each_entry(s, &bus->stream_list, list) {
 * 		if (s->bdl.area)
 * 			snd_dma_free_pages(&s->bdl);
 * 	}
 * 
 * 	if (bus->rb.area)
 * 		snd_dma_free_pages(&bus->rb);
 * 	if (bus->posbuf.area)
 * 		snd_dma_free_pages(&bus->posbuf);
 * }
 */
