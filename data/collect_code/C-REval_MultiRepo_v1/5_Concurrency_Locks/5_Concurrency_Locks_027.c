/* C-REval Sample */
/* Sample-ID: 5_Concurrency_Locks_027 */
/* Category: 5_Concurrency_Locks */
/* Repo: linux */
/* Cyclomatic Complexity: 8 */
/* NLOC: 28 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/backlight.h>
 * #include <linux/err.h>
 * #include <linux/export.h>
 * #include <linux/module.h>
 * #include <linux/of.h>
 * #include <drm/drm_crtc.h>
 * #include <drm/drm_panel.h>
 * #include <drm/drm_print.h>
 */
/* Context-Before
 *  * Removes a panel from the global registry.
 *  */
 * void drm_panel_remove(struct drm_panel *panel)
 * {
 * 	mutex_lock(&panel_lock);
 * 	list_del_init(&panel->list);
 * 	mutex_unlock(&panel_lock);
 * }
 * EXPORT_SYMBOL(drm_panel_remove);
 * 
 * /**
 *  * drm_panel_prepare - power on a panel
 *  * @panel: DRM panel
 *  *
 *  * Calling this function will enable power and deassert any reset signals to
 *  * the panel. After this has completed it is possible to communicate with any
 *  * integrated circuitry via a command bus. This function cannot fail (as it is
 *  * called from the pre_enable call chain). There will always be a call to
 *  * drm_panel_disable() afterwards.
 *  */
 */
void drm_panel_prepare(struct drm_panel *panel)
{
	struct drm_panel_follower *follower;
	int ret;

	if (!panel)
		return;

	if (panel->prepared) {
		dev_warn(panel->dev, "Skipping prepare of already prepared panel\n");
		return;
	}

	mutex_lock(&panel->follower_lock);

	if (panel->funcs && panel->funcs->prepare) {
		ret = panel->funcs->prepare(panel);
		if (ret < 0)
			goto exit;
	}
	panel->prepared = true;

	list_for_each_entry(follower, &panel->followers, list) {
		if (!follower->funcs->panel_prepared)
			continue;

		ret = follower->funcs->panel_prepared(follower);
		if (ret < 0)
			dev_info(panel->dev, "%ps failed: %d\n",
				 follower->funcs->panel_prepared, ret);
	}

exit:
	mutex_unlock(&panel->follower_lock);
}
/* Context-After
 * EXPORT_SYMBOL(drm_panel_prepare);
 * 
 * /**
 *  * drm_panel_unprepare - power off a panel
 *  * @panel: DRM panel
 *  *
 *  * Calling this function will completely power off a panel (assert the panel's
 *  * reset, turn off power supplies, ...). After this function has completed, it
 *  * is usually no longer possible to communicate with the panel until another
 *  * call to drm_panel_prepare().
 *  */
 * void drm_panel_unprepare(struct drm_panel *panel)
 * {
 * 	struct drm_panel_follower *follower;
 * 	int ret;
 * 
 * 	if (!panel)
 * 		return;
 * 
 * 	/*
 */
