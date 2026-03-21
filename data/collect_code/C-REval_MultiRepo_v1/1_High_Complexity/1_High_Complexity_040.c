/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_040 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 31 */
/* NLOC: 116 */
/* Subsystem: drivers */
/* Includes
 * #include <linux/bitops.h>
 * #include <linux/bitfield.h>
 * #include <linux/cleanup.h>
 * #include <linux/completion.h>
 * #include <linux/delay.h>
 * #include <linux/device.h>
 * #include <linux/gpio/consumer.h>
 * #include <linux/interrupt.h>
 * #include <linux/irq.h> /* For irq_get_irq_data() */
 * #include <linux/module.h>
 * #include <linux/nvmem-provider.h>
 * #include <linux/pm_runtime.h>
 * #include <linux/property.h>
 * #include <linux/random.h>
 * #include <linux/regmap.h>
 * #include <linux/regulator/consumer.h>
 * #include <linux/types.h>
 * #include <linux/iio/buffer.h>
 * #include <linux/iio/iio.h>
 * #include <linux/iio/trigger.h>
 */
/* Context-Before
 * static int bme280_read_humid(struct bmp280_data *data, u32 *comp_humidity)
 * {
 * 	u16 adc_humidity;
 * 	s32 t_fine;
 * 	int ret;
 * 
 * 	ret = bmp280_get_t_fine(data, &t_fine);
 * 	if (ret)
 * 		return ret;
 * 
 * 	ret = bme280_read_humid_adc(data, &adc_humidity);
 * 	if (ret)
 * 		return ret;
 * 
 * 	*comp_humidity = bme280_compensate_humidity(data, adc_humidity, t_fine);
 * 
 * 	return 0;
 * }
 */
static int bmp280_read_raw_impl(struct iio_dev *indio_dev,
				struct iio_chan_spec const *chan,
				int *val, int *val2, long mask)
{
	struct bmp280_data *data = iio_priv(indio_dev);
	int chan_value;
	int ret;

	guard(mutex)(&data->lock);

	switch (mask) {
	case IIO_CHAN_INFO_PROCESSED:
		ret = data->chip_info->set_mode(data, BMP280_FORCED);
		if (ret)
			return ret;

		ret = data->chip_info->wait_conv(data);
		if (ret)
			return ret;

		switch (chan->type) {
		case IIO_HUMIDITYRELATIVE:
			ret = data->chip_info->read_humid(data, &chan_value);
			if (ret)
				return ret;

			*val = data->chip_info->humid_coeffs[0] * chan_value;
			*val2 = data->chip_info->humid_coeffs[1];
			return data->chip_info->humid_coeffs_type;
		case IIO_PRESSURE:
			ret = data->chip_info->read_press(data, &chan_value);
			if (ret)
				return ret;

			*val = data->chip_info->press_coeffs[0] * chan_value;
			*val2 = data->chip_info->press_coeffs[1];
			return data->chip_info->press_coeffs_type;
		case IIO_TEMP:
			ret = data->chip_info->read_temp(data, &chan_value);
			if (ret)
				return ret;

			*val = data->chip_info->temp_coeffs[0] * chan_value;
			*val2 = data->chip_info->temp_coeffs[1];
			return data->chip_info->temp_coeffs_type;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_RAW:
		ret = data->chip_info->set_mode(data, BMP280_FORCED);
		if (ret)
			return ret;

		ret = data->chip_info->wait_conv(data);
		if (ret)
			return ret;

		switch (chan->type) {
		case IIO_HUMIDITYRELATIVE:
			ret = data->chip_info->read_humid(data, &chan_value);
			if (ret)
				return ret;

			*val = chan_value;
			return IIO_VAL_INT;
		case IIO_PRESSURE:
			ret = data->chip_info->read_press(data, &chan_value);
			if (ret)
				return ret;

			*val = chan_value;
			return IIO_VAL_INT;
		case IIO_TEMP:
			ret = data->chip_info->read_temp(data, &chan_value);
			if (ret)
				return ret;

			*val = chan_value;
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_SCALE:
		switch (chan->type) {
		case IIO_HUMIDITYRELATIVE:
			*val = data->chip_info->humid_coeffs[0];
			*val2 = data->chip_info->humid_coeffs[1];
			return data->chip_info->humid_coeffs_type;
		case IIO_PRESSURE:
			*val = data->chip_info->press_coeffs[0];
			*val2 = data->chip_info->press_coeffs[1];
			return data->chip_info->press_coeffs_type;
		case IIO_TEMP:
			*val = data->chip_info->temp_coeffs[0];
			*val2 = data->chip_info->temp_coeffs[1];
			return data->chip_info->temp_coeffs_type;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_OVERSAMPLING_RATIO:
		switch (chan->type) {
		case IIO_HUMIDITYRELATIVE:
			*val = 1 << data->oversampling_humid;
			return IIO_VAL_INT;
		case IIO_PRESSURE:
			*val = 1 << data->oversampling_press;
			return IIO_VAL_INT;
		case IIO_TEMP:
			*val = 1 << data->oversampling_temp;
			return IIO_VAL_INT;
		default:
			return -EINVAL;
		}
	case IIO_CHAN_INFO_SAMP_FREQ:
		if (!data->chip_info->sampling_freq_avail)
			return -EINVAL;

		*val = data->chip_info->sampling_freq_avail[data->sampling_freq][0];
		*val2 = data->chip_info->sampling_freq_avail[data->sampling_freq][1];
		return IIO_VAL_INT_PLUS_MICRO;
	case IIO_CHAN_INFO_LOW_PASS_FILTER_3DB_FREQUENCY:
		if (!data->chip_info->iir_filter_coeffs_avail)
			return -EINVAL;

		*val = (1 << data->iir_filter_coeff) - 1;
		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}
}
/* Context-After
 * static int bmp280_read_raw(struct iio_dev *indio_dev,
 * 			   struct iio_chan_spec const *chan,
 * 			   int *val, int *val2, long mask)
 * {
 * 	struct bmp280_data *data = iio_priv(indio_dev);
 * 	int ret;
 * 
 * 	pm_runtime_get_sync(data->dev);
 * 	ret = bmp280_read_raw_impl(indio_dev, chan, val, val2, mask);
 * 	pm_runtime_put_autosuspend(data->dev);
 * 
 * 	return ret;
 * }
 * 
 * static int bme280_write_oversampling_ratio_humid(struct bmp280_data *data,
 * 						 int val)
 * {
 * 	const int *avail = data->chip_info->oversampling_humid_avail;
 * 	const int n = data->chip_info->num_oversampling_humid_avail;
 */
