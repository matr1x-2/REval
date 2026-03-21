/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_032 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 24 */
/* NLOC: 107 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/bits.h>
 * #include <linux/clk.h>
 * #include <linux/delay.h>
 * #include <linux/err.h>
 * #include <linux/input.h>
 * #include <linux/interrupt.h>
 * #include <linux/io.h>
 * #include <linux/module.h>
 * #include <linux/platform_device.h>
 * #include <linux/pm.h>
 * #include <linux/pm_runtime.h>
 * #include <linux/slab.h>
 * #include <linux/of.h>
 * #include <linux/sched.h>
 * #include <linux/input/samsung-keypad.h>
 */
/* Context-Before
 * 	pdata->no_autorepeat = of_property_read_bool(np, "linux,input-no-autorepeat");
 * 
 * 	pdata->wakeup = of_property_read_bool(np, "wakeup-source") ||
 * 			/* legacy name */
 * 			of_property_read_bool(np, "linux,input-wakeup");
 * 
 * 
 * 	return pdata;
 * }
 * #else
 * static struct samsung_keypad_platdata *
 * samsung_keypad_parse_dt(struct device *dev)
 * {
 * 	dev_err(dev, "no platform data defined\n");
 * 
 * 	return ERR_PTR(-EINVAL);
 * }
 * #endif
 */
static int samsung_keypad_probe(struct platform_device *pdev)
{
	const struct samsung_keypad_platdata *pdata;
	const struct matrix_keymap_data *keymap_data;
	const struct platform_device_id *id;
	struct samsung_keypad *keypad;
	struct resource *res;
	struct input_dev *input_dev;
	unsigned int row_shift;
	int error;

	pdata = dev_get_platdata(&pdev->dev);
	if (!pdata) {
		pdata = samsung_keypad_parse_dt(&pdev->dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	}

	keymap_data = pdata->keymap_data;
	if (!keymap_data) {
		dev_err(&pdev->dev, "no keymap data defined\n");
		return -EINVAL;
	}

	if (!pdata->rows || pdata->rows > SAMSUNG_MAX_ROWS)
		return -EINVAL;

	if (!pdata->cols || pdata->cols > SAMSUNG_MAX_COLS)
		return -EINVAL;

	/* initialize the gpio */
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdata->rows, pdata->cols);

	row_shift = get_count_order(pdata->cols);

	keypad = devm_kzalloc(&pdev->dev,
			      struct_size(keypad, keycodes,
					  pdata->rows << row_shift),
			      GFP_KERNEL);
	if (!keypad)
		return -ENOMEM;

	input_dev = devm_input_allocate_device(&pdev->dev);
	if (!input_dev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	keypad->base = devm_ioremap(&pdev->dev, res->start, resource_size(res));
	if (!keypad->base)
		return -EBUSY;

	keypad->clk = devm_clk_get_prepared(&pdev->dev, "keypad");
	if (IS_ERR(keypad->clk)) {
		dev_err(&pdev->dev, "failed to get keypad clk\n");
		return PTR_ERR(keypad->clk);
	}

	keypad->input_dev = input_dev;
	keypad->pdev = pdev;
	keypad->row_shift = row_shift;
	keypad->rows = pdata->rows;
	keypad->cols = pdata->cols;
	keypad->stopped = true;
	init_waitqueue_head(&keypad->wait);

	keypad->chip = device_get_match_data(&pdev->dev);
	if (!keypad->chip) {
		id = platform_get_device_id(pdev);
		if (id)
			keypad->chip = (const void *)id->driver_data;
	}

	if (!keypad->chip) {
		dev_err(&pdev->dev, "Unable to determine chip type");
		return -EINVAL;
	}

	input_dev->name = pdev->name;
	input_dev->id.bustype = BUS_HOST;

	input_dev->open = samsung_keypad_open;
	input_dev->close = samsung_keypad_close;

	error = matrix_keypad_build_keymap(keymap_data, NULL,
					   pdata->rows, pdata->cols,
					   keypad->keycodes, input_dev);
	if (error) {
		dev_err(&pdev->dev, "failed to build keymap\n");
		return error;
	}

	input_set_capability(input_dev, EV_MSC, MSC_SCAN);
	if (!pdata->no_autorepeat)
		__set_bit(EV_REP, input_dev->evbit);

	input_set_drvdata(input_dev, keypad);

	keypad->irq = platform_get_irq(pdev, 0);
	if (keypad->irq < 0) {
		error = keypad->irq;
		return error;
	}

	error = devm_request_threaded_irq(&pdev->dev, keypad->irq, NULL,
					  samsung_keypad_irq, IRQF_ONESHOT,
					  dev_name(&pdev->dev), keypad);
	if (error) {
		dev_err(&pdev->dev, "failed to register keypad interrupt\n");
		return error;
	}

	device_init_wakeup(&pdev->dev, pdata->wakeup);
	platform_set_drvdata(pdev, keypad);

	error = devm_pm_runtime_enable(&pdev->dev);
	if (error)
		return error;

	error = input_register_device(keypad->input_dev);
	if (error)
		return error;

	if (pdev->dev.of_node) {
		devm_kfree(&pdev->dev, (void *)pdata->keymap_data->keymap);
		devm_kfree(&pdev->dev, (void *)pdata->keymap_data);
		devm_kfree(&pdev->dev, (void *)pdata);
	}
	return 0;
}
/* Context-After
 * static int samsung_keypad_runtime_suspend(struct device *dev)
 * {
 * 	struct platform_device *pdev = to_platform_device(dev);
 * 	struct samsung_keypad *keypad = platform_get_drvdata(pdev);
 * 	unsigned int val;
 * 	int error;
 * 
 * 	if (keypad->stopped)
 * 		return 0;
 * 
 * 	/* This may fail on some SoCs due to lack of controller support */
 * 	error = enable_irq_wake(keypad->irq);
 * 	if (!error)
 * 		keypad->wake_enabled = true;
 * 
 * 	val = readl(keypad->base + SAMSUNG_KEYIFCON);
 * 	val |= SAMSUNG_KEYIFCON_WAKEUPEN;
 * 	writel(val, keypad->base + SAMSUNG_KEYIFCON);
 */
