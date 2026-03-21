/* C-REval Sample */
/* Sample-ID: 2_Goto_Paths_037 */
/* Category: 2_Goto_Paths */
/* Repo: linux */
/* Cyclomatic Complexity: 10 */
/* NLOC: 67 */
/* Subsystem: arch */
/* Includes
 * #include <linux/err.h>
 * #include <linux/gpio/machine.h>
 * #include <linux/gpio/property.h>
 * #include <linux/input.h>
 * #include <linux/leds.h>
 * #include <linux/platform_device.h>
 * #include <linux/slab.h>
 * #include "geode-common.h"
 */
/* Context-Before
 * 	keys_info.fwnode = software_node_fwnode(&geode_gpio_keys_node);
 * 
 * 	pd = platform_device_register_full(&keys_info);
 * 	err = PTR_ERR_OR_ZERO(pd);
 * 	if (err) {
 * 		pr_err("failed to create gpio-keys device: %d\n", err);
 * 		software_node_unregister_node_group(geode_gpio_keys_swnodes);
 * 		return err;
 * 	}
 * 
 * 	return 0;
 * }
 * 
 * static const struct software_node geode_gpio_leds_node = {
 * 	.name = "geode-leds",
 * };
 * 
 * #define MAX_LEDS	3
 */
int __init geode_create_leds(const char *label, const struct geode_led *leds,
			      unsigned int n_leds)
{
	const struct software_node *group[MAX_LEDS + 2] = { 0 };
	struct software_node *swnodes;
	struct property_entry *props;
	struct platform_device_info led_info = {
		.name	= "leds-gpio",
		.id	= PLATFORM_DEVID_NONE,
	};
	struct platform_device *led_dev;
	const char *node_name;
	int err;
	int i;

	if (n_leds > MAX_LEDS) {
		pr_err("%s: too many LEDs\n", __func__);
		return -EINVAL;
	}

	swnodes = kzalloc_objs(*swnodes, n_leds);
	if (!swnodes)
		return -ENOMEM;

	/*
	 * Each LED is represented by 3 properties: "gpios",
	 * "linux,default-trigger", and am empty terminator.
	 */
	props = kzalloc_objs(*props, n_leds * 3);
	if (!props) {
		err = -ENOMEM;
		goto err_free_swnodes;
	}

	group[0] = &geode_gpio_leds_node;
	for (i = 0; i < n_leds; i++) {
		node_name = kasprintf(GFP_KERNEL, "%s:%d", label, i);
		if (!node_name) {
			err = -ENOMEM;
			goto err_free_names;
		}

		props[i * 3 + 0] =
			PROPERTY_ENTRY_GPIO("gpios", &geode_gpiochip_node,
					    leds[i].pin, GPIO_ACTIVE_LOW);
		props[i * 3 + 1] =
			PROPERTY_ENTRY_STRING("linux,default-trigger",
					      leds[i].default_on ?
					      "default-on" : "default-off");
		/* props[i * 3 + 2] is an empty terminator */

		swnodes[i] = SOFTWARE_NODE(node_name, &props[i * 3],
					   &geode_gpio_leds_node);
		group[i + 1] = &swnodes[i];
	}

	err = software_node_register_node_group(group);
	if (err) {
		pr_err("failed to register LED software nodes: %d\n", err);
		goto err_free_names;
	}

	led_info.fwnode = software_node_fwnode(&geode_gpio_leds_node);

	led_dev = platform_device_register_full(&led_info);
	err = PTR_ERR_OR_ZERO(led_dev);
	if (err) {
		pr_err("failed to create LED device: %d\n", err);
		goto err_unregister_group;
	}

	return 0;

err_unregister_group:
	software_node_unregister_node_group(group);
err_free_names:
	while (--i >= 0)
		kfree(swnodes[i].name);
	kfree(props);
err_free_swnodes:
	kfree(swnodes);
	return err;
}
/* Context-After: <empty> */
