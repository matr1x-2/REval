/* C-REval Sample */
/* Sample-ID: 4_Multi_Pointers_037 */
/* Category: 4_Multi_Pointers */
/* Repo: linux */
/* Cyclomatic Complexity: 61 */
/* NLOC: 261 */
/* Subsystem: kernel */
/* Includes
 * #include <crypto/acompress.h>
 * #include <linux/module.h>
 * #include <linux/file.h>
 * #include <linux/delay.h>
 * #include <linux/bitops.h>
 * #include <linux/device.h>
 * #include <linux/bio.h>
 * #include <linux/blkdev.h>
 * #include <linux/swap.h>
 * #include <linux/swapops.h>
 * #include <linux/pm.h>
 * #include <linux/slab.h>
 * #include <linux/vmalloc.h>
 * #include <linux/cpumask.h>
 * #include <linux/atomic.h>
 * #include <linux/kthread.h>
 * #include <linux/crc32.h>
 * #include <linux/ktime.h>
 * #include "power.h"
 */
/* Context-Before
 * 		atomic_set(&d->ready, 0);
 * 
 * 		acomp_request_set_callback(d->cr, CRYPTO_TFM_REQ_MAY_SLEEP,
 * 					   NULL, NULL);
 * 		acomp_request_set_src_nondma(d->cr, d->cmp + CMP_HEADER,
 * 					     d->cmp_len);
 * 		acomp_request_set_dst_nondma(d->cr, d->unc, UNC_SIZE);
 * 		d->ret = crypto_acomp_decompress(d->cr);
 * 		d->unc_len = d->cr->dlen;
 * 
 * 		if (clean_pages_on_decompress)
 * 			flush_icache_range((unsigned long)d->unc,
 * 					   (unsigned long)d->unc + d->unc_len);
 * 
 * 		atomic_set_release(&d->stop, 1);
 * 		wake_up(&d->done);
 * 	}
 * 	return 0;
 * }
 */
static int load_compressed_image(struct swap_map_handle *handle,
				 struct snapshot_handle *snapshot,
				 unsigned int nr_to_read)
{
	unsigned int m;
	int ret = 0;
	int eof = 0;
	struct hib_bio_batch hb;
	ktime_t start;
	ktime_t stop;
	unsigned nr_pages;
	size_t off;
	unsigned i, thr, run_threads, nr_threads;
	unsigned ring = 0, pg = 0, ring_size = 0,
	         have = 0, want, need, asked = 0;
	unsigned long read_pages = 0;
	unsigned char **page = NULL;
	struct dec_data *data = NULL;
	struct crc_data *crc = NULL;

	hib_init_batch(&hb);

	/*
	 * We'll limit the number of threads for decompression to limit memory
	 * footprint.
	 */
	nr_threads = num_online_cpus() - 1;
	nr_threads = clamp_val(nr_threads, 1, hibernate_compression_threads);

	page = vmalloc_array(CMP_MAX_RD_PAGES, sizeof(*page));
	if (!page) {
		pr_err("Failed to allocate %s page\n", hib_comp_algo);
		ret = -ENOMEM;
		goto out_clean;
	}

	data = vcalloc(nr_threads, sizeof(*data));
	if (!data) {
		pr_err("Failed to allocate %s data\n", hib_comp_algo);
		ret = -ENOMEM;
		goto out_clean;
	}

	crc = alloc_crc_data(nr_threads);
	if (!crc) {
		pr_err("Failed to allocate crc\n");
		ret = -ENOMEM;
		goto out_clean;
	}

	clean_pages_on_decompress = true;

	/*
	 * Start the decompression threads.
	 */
	for (thr = 0; thr < nr_threads; thr++) {
		init_waitqueue_head(&data[thr].go);
		init_waitqueue_head(&data[thr].done);

		data[thr].cc = crypto_alloc_acomp(hib_comp_algo, 0, CRYPTO_ALG_ASYNC);
		if (IS_ERR_OR_NULL(data[thr].cc)) {
			pr_err("Could not allocate comp stream %ld\n", PTR_ERR(data[thr].cc));
			ret = -EFAULT;
			goto out_clean;
		}

		data[thr].cr = acomp_request_alloc(data[thr].cc);
		if (!data[thr].cr) {
			pr_err("Could not allocate comp request\n");
			ret = -ENOMEM;
			goto out_clean;
		}

		data[thr].thr = kthread_run(decompress_threadfn,
		                            &data[thr],
		                            "image_decompress/%u", thr);
		if (IS_ERR(data[thr].thr)) {
			data[thr].thr = NULL;
			pr_err("Cannot start decompression threads\n");
			ret = -ENOMEM;
			goto out_clean;
		}
	}

	/*
	 * Start the CRC32 thread.
	 */
	init_waitqueue_head(&crc->go);
	init_waitqueue_head(&crc->done);

	handle->crc32 = 0;
	crc->crc32 = &handle->crc32;
	for (thr = 0; thr < nr_threads; thr++) {
		crc->unc[thr] = data[thr].unc;
		crc->unc_len[thr] = &data[thr].unc_len;
	}

	crc->thr = kthread_run(crc32_threadfn, crc, "image_crc32");
	if (IS_ERR(crc->thr)) {
		crc->thr = NULL;
		pr_err("Cannot start CRC32 thread\n");
		ret = -ENOMEM;
		goto out_clean;
	}

	/*
	 * Set the number of pages for read buffering.
	 * This is complete guesswork, because we'll only know the real
	 * picture once prepare_image() is called, which is much later on
	 * during the image load phase. We'll assume the worst case and
	 * say that none of the image pages are from high memory.
	 */
	if (low_free_pages() > snapshot_get_image_size())
		read_pages = (low_free_pages() - snapshot_get_image_size()) / 2;
	read_pages = clamp_val(read_pages, CMP_MIN_RD_PAGES, CMP_MAX_RD_PAGES);

	for (i = 0; i < read_pages; i++) {
		page[i] = (void *)__get_free_page(i < CMP_PAGES ?
						  GFP_NOIO | __GFP_HIGH :
						  GFP_NOIO | __GFP_NOWARN |
						  __GFP_NORETRY);

		if (!page[i]) {
			if (i < CMP_PAGES) {
				ring_size = i;
				pr_err("Failed to allocate %s pages\n", hib_comp_algo);
				ret = -ENOMEM;
				goto out_clean;
			} else {
				break;
			}
		}
	}
	want = ring_size = i;

	pr_info("Using %u thread(s) for %s decompression\n", nr_threads, hib_comp_algo);
	pr_info("Loading and decompressing image data (%u pages)...\n",
		nr_to_read);
	m = nr_to_read / 10;
	if (!m)
		m = 1;
	nr_pages = 0;
	start = ktime_get();

	ret = snapshot_write_next(snapshot);
	if (ret <= 0)
		goto out_finish;

	for(;;) {
		for (i = 0; !eof && i < want; i++) {
			ret = swap_read_page(handle, page[ring], &hb);
			if (ret) {
				/*
				 * On real read error, finish. On end of data,
				 * set EOF flag and just exit the read loop.
				 */
				if (handle->cur &&
				    handle->cur->entries[handle->k]) {
					goto out_finish;
				} else {
					eof = 1;
					break;
				}
			}
			if (++ring >= ring_size)
				ring = 0;
		}
		asked += i;
		want -= i;

		/*
		 * We are out of data, wait for some more.
		 */
		if (!have) {
			if (!asked)
				break;

			ret = hib_wait_io(&hb);
			if (ret)
				goto out_finish;
			have += asked;
			asked = 0;
			if (eof)
				eof = 2;
		}

		if (crc->run_threads) {
			wait_event(crc->done, atomic_read_acquire(&crc->stop));
			atomic_set(&crc->stop, 0);
			crc->run_threads = 0;
		}

		for (thr = 0; have && thr < nr_threads; thr++) {
			data[thr].cmp_len = *(size_t *)page[pg];
			if (unlikely(!data[thr].cmp_len ||
			             data[thr].cmp_len >
					bytes_worst_compress(UNC_SIZE))) {
				pr_err("Invalid %s compressed length\n", hib_comp_algo);
				ret = -1;
				goto out_finish;
			}

			need = DIV_ROUND_UP(data[thr].cmp_len + CMP_HEADER,
			                    PAGE_SIZE);
			if (need > have) {
				if (eof > 1) {
					ret = -1;
					goto out_finish;
				}
				break;
			}

			for (off = 0;
			     off < CMP_HEADER + data[thr].cmp_len;
			     off += PAGE_SIZE) {
				memcpy(data[thr].cmp + off,
				       page[pg], PAGE_SIZE);
				have--;
				want++;
				if (++pg >= ring_size)
					pg = 0;
			}

			atomic_set_release(&data[thr].ready, 1);
			wake_up(&data[thr].go);
		}

		/*
		 * Wait for more data while we are decompressing.
		 */
		if (have < CMP_PAGES && asked) {
			ret = hib_wait_io(&hb);
			if (ret)
				goto out_finish;
			have += asked;
			asked = 0;
			if (eof)
				eof = 2;
		}

		for (run_threads = thr, thr = 0; thr < run_threads; thr++) {
			wait_event(data[thr].done,
				atomic_read_acquire(&data[thr].stop));
			atomic_set(&data[thr].stop, 0);

			ret = data[thr].ret;

			if (ret < 0) {
				pr_err("%s decompression failed\n", hib_comp_algo);
				goto out_finish;
			}

			if (unlikely(!data[thr].unc_len ||
				data[thr].unc_len > UNC_SIZE ||
				data[thr].unc_len & (PAGE_SIZE - 1))) {
				pr_err("Invalid %s uncompressed length\n", hib_comp_algo);
				ret = -1;
				goto out_finish;
			}

			for (off = 0;
			     off < data[thr].unc_len; off += PAGE_SIZE) {
				memcpy(data_of(*snapshot),
				       data[thr].unc + off, PAGE_SIZE);

				if (!(nr_pages % m))
					pr_info("Image loading progress: %3d%%\n",
						nr_pages / m * 10);
				nr_pages++;

				ret = snapshot_write_next(snapshot);
				if (ret <= 0) {
					crc->run_threads = thr + 1;
					atomic_set_release(&crc->ready, 1);
					wake_up(&crc->go);
					goto out_finish;
				}
			}
		}

		crc->run_threads = thr;
		atomic_set_release(&crc->ready, 1);
		wake_up(&crc->go);
	}

out_finish:
	if (crc->run_threads) {
		wait_event(crc->done, atomic_read_acquire(&crc->stop));
		atomic_set(&crc->stop, 0);
	}
	stop = ktime_get();
	if (!ret) {
		pr_info("Image loading done\n");
		ret = snapshot_write_finalize(snapshot);
		if (!ret && !snapshot_image_loaded(snapshot))
			ret = -ENODATA;
		if (!ret) {
			if (swsusp_header->flags & SF_CRC32_MODE) {
				if(handle->crc32 != swsusp_header->crc32) {
					pr_err("Invalid image CRC32!\n");
					ret = -ENODATA;
				}
			}
		}
	}
	swsusp_show_speed(start, stop, nr_to_read, "Read");
out_clean:
	hib_finish_batch(&hb);
	for (i = 0; i < ring_size; i++)
		free_page((unsigned long)page[i]);
	free_crc_data(crc);
	if (data) {
		for (thr = 0; thr < nr_threads; thr++) {
			if (data[thr].thr)
				kthread_stop(data[thr].thr);

			acomp_request_free(data[thr].cr);

			if (!IS_ERR_OR_NULL(data[thr].cc))
				crypto_free_acomp(data[thr].cc);
		}
		vfree(data);
	}
	vfree(page);

	return ret;
}
/* Context-After
 * /**
 *  *	swsusp_read - read the hibernation image.
 *  *	@flags_p: flags passed by the "frozen" kernel in the image header should
 *  *		  be written into this memory location
 *  *
 *  *	Return: 0 on success, negative error code on failure.
 *  */
 * int swsusp_read(unsigned int *flags_p)
 * {
 * 	int error;
 * 	struct swap_map_handle handle;
 * 	struct snapshot_handle snapshot;
 * 	struct swsusp_info *header;
 * 
 * 	memset(&snapshot, 0, sizeof(struct snapshot_handle));
 * 	error = snapshot_write_next(&snapshot);
 * 	if (error < (int)PAGE_SIZE)
 * 		return error < 0 ? error : -EFAULT;
 * 	header = (struct swsusp_info *)data_of(snapshot);
 */
