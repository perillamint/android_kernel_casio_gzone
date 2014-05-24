/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
/***********************************************************************/
/* Modified by                                                         */
/* (C) NEC CASIO Mobile Communications, Ltd. 2013                      */
/***********************************************************************/
#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/power_supply.h>
#include <linux/mfd/pm8xxx/pm8921-bms.h>
#include <linux/mfd/pm8xxx/core.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/mfd/pm8xxx/pm8921-charger.h>
#include <linux/mfd/pm8xxx/ccadc.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/rtc.h>

#include <linux/mfd/pm8xxx/misc.h>

#define BMS_CONTROL		0x224
#define BMS_S1_DELAY		0x225
#define BMS_OUTPUT0		0x230
#define BMS_OUTPUT1		0x231
#define BMS_TOLERANCES		0x232
#define BMS_TEST1		0x237

#define ADC_ARB_SECP_CNTRL	0x190
#define ADC_ARB_SECP_AMUX_CNTRL	0x191
#define ADC_ARB_SECP_ANA_PARAM	0x192
#define ADC_ARB_SECP_DIG_PARAM	0x193
#define ADC_ARB_SECP_RSV	0x194
#define ADC_ARB_SECP_DATA1	0x195
#define ADC_ARB_SECP_DATA0	0x196

#define ADC_ARB_BMS_CNTRL	0x18D
#define AMUX_TRIM_2		0x322
#define TEST_PROGRAM_REV	0x339

#define TEMP_SOC_STORAGE	0x107

#define TEMP_IAVG_STORAGE	0x105
#define TEMP_IAVG_STORAGE_USE_MASK	0x0F

enum pmic_bms_interrupts {
	PM8921_BMS_SBI_WRITE_OK,
	PM8921_BMS_CC_THR,
	PM8921_BMS_VSENSE_THR,
	PM8921_BMS_VSENSE_FOR_R,
	PM8921_BMS_OCV_FOR_R,
	PM8921_BMS_GOOD_OCV,
	PM8921_BMS_VSENSE_AVG,
	PM_BMS_MAX_INTS,
};

struct pm8921_soc_params {
	uint16_t	last_good_ocv_raw;
	int		cc;

	int		last_good_ocv_uv;
};


struct ncm_bmsmon_params {
	int	soc_0;
	int	soc_1;
	int	soc_2;
	int	soc_3;
	int	soc_4;
	int	soc_5;
	int	soc;
	int	fcc_uah;
	int	ruc_uah;
	int	rc_uah;
	int	cc_uah;
	int	uuc_uah;
	int	pc_rc;
	int	pc_cc;
	int	pc_uuc;
	int	rbatt;
	int	iavg_ua;
	int	batt_temp;
	int	ibat_ua;
	int	vbat_uv;
	int	last_good_ocv_uv;
	int	vsense_uv;
	int	start_soc;
	int	start_ocv_uv;
	int	start_cc_uah;
	int	end_soc;
	int	end_ocv_uv;
	int	end_cc_uah;
	int	charging_end_cnt;
	int	battery_full_cnt;
	int	ocv_renew_cnt;
	int	ocv_est_uv;
	int	pc_est;
	int	soc_est;
};


/**
 * struct pm8921_bms_chip -
 * @bms_output_lock:	lock to prevent concurrent bms reads
 *
 * @last_ocv_uv_mutex:	mutex to protect simultaneous invocations of calculate
 *			state of charge, note that last_ocv_uv could be
 *			changed as soc is adjusted. This mutex protects
 *			simultaneous updates of last_ocv_uv as well. This mutex
 *			also protects changes to *_at_100 variables used in
 *			faking 100% SOC.
 */
struct pm8921_bms_chip {
	struct device		*dev;
	struct dentry		*dent;
	unsigned int		r_sense;
	unsigned int		v_cutoff;
	unsigned int		fcc;
	struct single_row_lut	*fcc_temp_lut;
	struct single_row_lut	*fcc_sf_lut;
	struct pc_temp_ocv_lut	*pc_temp_ocv_lut;
	struct sf_lut		*pc_sf_lut;
	struct sf_lut		*rbatt_sf_lut;
	int			delta_rbatt_mohm;
	struct work_struct	calib_hkadc_work;
	struct delayed_work	calib_hkadc_delayed_work;
	struct mutex		calib_mutex;
	unsigned int		revision;
	unsigned int		xoadc_v0625_usb_present;
	unsigned int		xoadc_v0625_usb_absent;
	unsigned int		xoadc_v0625;
	unsigned int		xoadc_v125;
	unsigned int		batt_temp_channel;
	unsigned int		vbat_channel;
	unsigned int		ref625mv_channel;
	unsigned int		ref1p25v_channel;
	unsigned int		batt_id_channel;
	unsigned int		pmic_bms_irq[PM_BMS_MAX_INTS];
	DECLARE_BITMAP(enabled_irqs, PM_BMS_MAX_INTS);
	struct mutex		bms_output_lock;
	struct single_row_lut	*adjusted_fcc_temp_lut;
	unsigned int		charging_began;
	unsigned int		start_percent;
	unsigned int		end_percent;
	int			charge_time_us;
	int			catch_up_time_us;
	enum battery_type	batt_type;
	uint16_t		ocv_reading_at_100;
	int			cc_reading_at_100;
	int			max_voltage_uv;

	int			chg_term_ua;
	int			default_rbatt_mohm;
	int			amux_2_trim_delta;
	uint16_t		prev_last_good_ocv_raw;
	unsigned int		rconn_mohm;
	struct mutex		last_ocv_uv_mutex;
	int			last_ocv_uv;
	int			pon_ocv_uv;
	int			last_cc_uah;
	unsigned long		tm_sec;
	int			enable_fcc_learning;
	int			shutdown_soc;
	int			shutdown_iavg_ua;
	struct delayed_work	calculate_soc_delayed_work;
	struct timespec		t_soc_queried;
	int			shutdown_soc_valid_limit;
	int			ignore_shutdown_soc;
	int			prev_iavg_ua;
	int			prev_uuc_iavg_ma;
	int			prev_pc_unusable;
	int			adjust_soc_low_threshold;

	int			ibat_at_cv_ua;
	int			soc_at_cv;
	int			prev_chg_soc;

	struct power_supply	*batt_psy;
	
	struct ncm_bmsmon_params	bmsmon_data;
	

	int			batt_id_min;
	int			batt_id_max;
	int			batt_id_min_ext;
	int			batt_id_max_ext;
	int64_t			batt_id_mean_mv;
	bool			is_mean_batt_id_ready;

};

/*
 * protects against simultaneous adjustment of ocv based on shutdown soc and
 * invalidating the shutdown soc
 */
static DEFINE_MUTEX(soc_invalidation_mutex);
static int shutdown_soc_invalid;
static struct pm8921_bms_chip *the_chip;

#define DEFAULT_RBATT_MOHMS		128
#define DEFAULT_OCV_MICROVOLTS		3900000
#define DEFAULT_CHARGE_CYCLES		0

static int last_usb_cal_delta_uv = 1800;
module_param(last_usb_cal_delta_uv, int, 0644);

static int last_chargecycles = DEFAULT_CHARGE_CYCLES;
static int last_charge_increase;
module_param(last_chargecycles, int, 0644);
module_param(last_charge_increase, int, 0644);

static int calculated_soc = -EINVAL;
static int last_soc = -EINVAL;
static int last_real_fcc_mah = -EINVAL;
static int last_real_fcc_batt_temp = -EINVAL;

static int bms_ops_set(const char *val, const struct kernel_param *kp)
{
	if (*(int *)kp->arg == -EINVAL)
		return param_set_int(val, kp);
	else
		return 0;
}

static struct kernel_param_ops bms_param_ops = {
	.set = bms_ops_set,
	.get = param_get_int,
};

module_param_cb(last_soc, &bms_param_ops, &last_soc, 0644);

/*
 * bms_fake_battery is set in setups where a battery emulator is used instead
 * of a real battery. This makes the bms driver report a different/fake value
 * regardless of the calculated state of charge.
 */
static int bms_fake_battery = -EINVAL;
module_param(bms_fake_battery, int, 0644);

/* bms_start_XXX and bms_end_XXX are read only */
static int bms_start_percent;
static int bms_start_ocv_uv;
static int bms_start_cc_uah;
static int bms_end_percent;
static int bms_end_ocv_uv;
static int bms_end_cc_uah;

static int bms_ro_ops_set(const char *val, const struct kernel_param *kp)
{
	return -EINVAL;
}

static struct kernel_param_ops bms_ro_param_ops = {
	.set = bms_ro_ops_set,
	.get = param_get_int,
};
module_param_cb(bms_start_percent, &bms_ro_param_ops, &bms_start_percent, 0644);
module_param_cb(bms_start_ocv_uv, &bms_ro_param_ops, &bms_start_ocv_uv, 0644);
module_param_cb(bms_start_cc_uah, &bms_ro_param_ops, &bms_start_cc_uah, 0644);

module_param_cb(bms_end_percent, &bms_ro_param_ops, &bms_end_percent, 0644);
module_param_cb(bms_end_ocv_uv, &bms_ro_param_ops, &bms_end_ocv_uv, 0644);
module_param_cb(bms_end_cc_uah, &bms_ro_param_ops, &bms_end_cc_uah, 0644);

static int interpolate_fcc(struct pm8921_bms_chip *chip, int batt_temp);
static void readjust_fcc_table(void)
{
	struct single_row_lut *temp, *old;
	int i, fcc, ratio;

	if (!the_chip->fcc_temp_lut) {
		pr_err("The static fcc lut table is NULL\n");
		return;
	}

	temp = kzalloc(sizeof(struct single_row_lut), GFP_KERNEL);
	if (!temp) {
		pr_err("Cannot allocate memory for adjusted fcc table\n");
		return;
	}

	fcc = interpolate_fcc(the_chip, last_real_fcc_batt_temp);

	temp->cols = the_chip->fcc_temp_lut->cols;
	for (i = 0; i < the_chip->fcc_temp_lut->cols; i++) {
		temp->x[i] = the_chip->fcc_temp_lut->x[i];
		ratio = div_u64(the_chip->fcc_temp_lut->y[i] * 1000, fcc);
		temp->y[i] =  (ratio * last_real_fcc_mah);
		temp->y[i] /= 1000;
		pr_debug("temp=%d, staticfcc=%d, adjfcc=%d, ratio=%d\n",
				temp->x[i], the_chip->fcc_temp_lut->y[i],
				temp->y[i], ratio);
	}

	old = the_chip->adjusted_fcc_temp_lut;
	the_chip->adjusted_fcc_temp_lut = temp;
	kfree(old);
}

static int bms_last_real_fcc_set(const char *val,
				const struct kernel_param *kp)
{
	int rc = 0;

	if (last_real_fcc_mah == -EINVAL)
		rc = param_set_int(val, kp);
	if (rc) {
		pr_err("Failed to set last_real_fcc_mah rc=%d\n", rc);
		return rc;
	}
	if (last_real_fcc_batt_temp != -EINVAL)
		readjust_fcc_table();
	return rc;
}
static struct kernel_param_ops bms_last_real_fcc_param_ops = {
	.set = bms_last_real_fcc_set,
	.get = param_get_int,
};
module_param_cb(last_real_fcc_mah, &bms_last_real_fcc_param_ops,
					&last_real_fcc_mah, 0644);

static int bms_last_real_fcc_batt_temp_set(const char *val,
				const struct kernel_param *kp)
{
	int rc = 0;

	if (last_real_fcc_batt_temp == -EINVAL)
		rc = param_set_int(val, kp);
	if (rc) {
		pr_err("Failed to set last_real_fcc_batt_temp rc=%d\n", rc);
		return rc;
	}
	if (last_real_fcc_mah != -EINVAL)
		readjust_fcc_table();
	return rc;
}

static struct kernel_param_ops bms_last_real_fcc_batt_temp_param_ops = {
	.set = bms_last_real_fcc_batt_temp_set,
	.get = param_get_int,
};
module_param_cb(last_real_fcc_batt_temp, &bms_last_real_fcc_batt_temp_param_ops,
					&last_real_fcc_batt_temp, 0644);

static int pm_bms_get_rt_status(struct pm8921_bms_chip *chip, int irq_id)
{
	return pm8xxx_read_irq_stat(chip->dev->parent,
					chip->pmic_bms_irq[irq_id]);
}

static void pm8921_bms_enable_irq(struct pm8921_bms_chip *chip, int interrupt)
{
	if (!__test_and_set_bit(interrupt, chip->enabled_irqs)) {
		dev_dbg(chip->dev, "%s %d\n", __func__,
						chip->pmic_bms_irq[interrupt]);
		enable_irq(chip->pmic_bms_irq[interrupt]);
	}
}

static void pm8921_bms_disable_irq(struct pm8921_bms_chip *chip, int interrupt)
{
	if (__test_and_clear_bit(interrupt, chip->enabled_irqs)) {
		pr_debug("%d\n", chip->pmic_bms_irq[interrupt]);
		disable_irq_nosync(chip->pmic_bms_irq[interrupt]);
	}
}

static int pm_bms_masked_write(struct pm8921_bms_chip *chip, u16 addr,
							u8 mask, u8 val)
{
	int rc;
	u8 reg;

	rc = pm8xxx_readb(chip->dev->parent, addr, &reg);
	if (rc) {
		pr_err("read failed addr = %03X, rc = %d\n", addr, rc);
		return rc;
	}
	reg &= ~mask;
	reg |= val & mask;
	rc = pm8xxx_writeb(chip->dev->parent, addr, reg);
	if (rc) {
		pr_err("write failed addr = %03X, rc = %d\n", addr, rc);
		return rc;
	}
	return 0;
}

static int usb_chg_plugged_in(struct pm8921_bms_chip *chip)
{
	int val = pm8921_is_usb_chg_plugged_in();

	/* if the charger driver was not initialized, use the restart reason */
	if (val == -EINVAL) {
		if (pm8xxx_restart_reason(chip->dev->parent)
				== PM8XXX_RESTART_CHG)
			val = 1;
		else
			val = 0;
	}

	return val;
}

#define HOLD_OREG_DATA		BIT(1)
static int pm_bms_lock_output_data(struct pm8921_bms_chip *chip)
{
	int rc;

	rc = pm_bms_masked_write(chip, BMS_CONTROL, HOLD_OREG_DATA,
					HOLD_OREG_DATA);
	if (rc) {
		pr_err("couldnt lock bms output rc = %d\n", rc);
		return rc;
	}
	return 0;
}

static int pm_bms_unlock_output_data(struct pm8921_bms_chip *chip)
{
	int rc;

	rc = pm_bms_masked_write(chip, BMS_CONTROL, HOLD_OREG_DATA, 0);
	if (rc) {
		pr_err("fail to unlock BMS_CONTROL rc = %d\n", rc);
		return rc;
	}
	return 0;
}

#define SELECT_OUTPUT_DATA	0x1C
#define SELECT_OUTPUT_TYPE_SHIFT	2
#define OCV_FOR_RBATT		0x0
#define VSENSE_FOR_RBATT	0x1
#define VBATT_FOR_RBATT		0x2
#define CC_MSB			0x3
#define CC_LSB			0x4
#define LAST_GOOD_OCV_VALUE	0x5
#define VSENSE_AVG		0x6
#define VBATT_AVG		0x7

static int pm_bms_read_output_data(struct pm8921_bms_chip *chip, int type,
						int16_t *result)
{
	int rc;
	u8 reg;

	if (!result) {
		pr_err("result pointer null\n");
		return -EINVAL;
	}
	*result = 0;
	if (type < OCV_FOR_RBATT || type > VBATT_AVG) {
		pr_err("invalid type %d asked to read\n", type);
		return -EINVAL;
	}

	rc = pm_bms_masked_write(chip, BMS_CONTROL, SELECT_OUTPUT_DATA,
					type << SELECT_OUTPUT_TYPE_SHIFT);
	if (rc) {
		pr_err("fail to select %d type in BMS_CONTROL rc = %d\n",
						type, rc);
		return rc;
	}

	rc = pm8xxx_readb(chip->dev->parent, BMS_OUTPUT0, &reg);
	if (rc) {
		pr_err("fail to read BMS_OUTPUT0 for type %d rc = %d\n",
			type, rc);
		return rc;
	}
	*result = reg;
	rc = pm8xxx_readb(chip->dev->parent, BMS_OUTPUT1, &reg);
	if (rc) {
		pr_err("fail to read BMS_OUTPUT1 for type %d rc = %d\n",
			type, rc);
		return rc;
	}
	*result |= reg << 8;
	pr_debug("type %d result %x", type, *result);
	return 0;
}

#define V_PER_BIT_MUL_FACTOR	97656
#define V_PER_BIT_DIV_FACTOR	1000
#define XOADC_INTRINSIC_OFFSET	0x6000
static int xoadc_reading_to_microvolt(unsigned int a)
{
	if (a <= XOADC_INTRINSIC_OFFSET)
		return 0;

	return (a - XOADC_INTRINSIC_OFFSET)
			* V_PER_BIT_MUL_FACTOR / V_PER_BIT_DIV_FACTOR;
}

#define XOADC_CALIB_UV		625000
#define VBATT_MUL_FACTOR	3
static int adjust_xo_vbatt_reading(struct pm8921_bms_chip *chip,
					int usb_chg, unsigned int uv)
{
	s64 numerator, denominator;
	int local_delta;

	if (uv == 0)
		return 0;

	/* dont adjust if not calibrated */
	if (chip->xoadc_v0625 == 0 || chip->xoadc_v125 == 0) {
		pr_debug("No cal yet return %d\n", VBATT_MUL_FACTOR * uv);
		return VBATT_MUL_FACTOR * uv;
	}

	if (usb_chg)
		local_delta = last_usb_cal_delta_uv;
	else
		local_delta = 0;

	pr_debug("using delta = %d\n", local_delta);
	numerator = ((s64)uv - chip->xoadc_v0625 - local_delta)
							* XOADC_CALIB_UV;
	denominator =  (s64)chip->xoadc_v125 - chip->xoadc_v0625 - local_delta;
	if (denominator == 0)
		return uv * VBATT_MUL_FACTOR;
	return (XOADC_CALIB_UV + local_delta + div_s64(numerator, denominator))
						* VBATT_MUL_FACTOR;
}

#define CC_RESOLUTION_N		868056
#define CC_RESOLUTION_D		10000

static s64 cc_to_microvolt(struct pm8921_bms_chip *chip, s64 cc)
{
	return div_s64(cc * CC_RESOLUTION_N, CC_RESOLUTION_D);
}

#define CC_READING_TICKS	56
#define SLEEP_CLK_HZ		32764
#define SECONDS_PER_HOUR	3600
/**
 * ccmicrovolt_to_nvh -
 * @cc_uv:  coulumb counter converted to uV
 *
 * RETURNS:	coulumb counter based charge in nVh
 *		(nano Volt Hour)
 */
static s64 ccmicrovolt_to_nvh(s64 cc_uv)
{
	return div_s64(cc_uv * CC_READING_TICKS * 1000,
			SLEEP_CLK_HZ * SECONDS_PER_HOUR);
}

/* returns the signed value read from the hardware */
static int read_cc(struct pm8921_bms_chip *chip, int *result)
{
	int rc;
	uint16_t msw, lsw;

	rc = pm_bms_read_output_data(chip, CC_LSB, &lsw);
	if (rc) {
		pr_err("fail to read CC_LSB rc = %d\n", rc);
		return rc;
	}
	rc = pm_bms_read_output_data(chip, CC_MSB, &msw);
	if (rc) {
		pr_err("fail to read CC_MSB rc = %d\n", rc);
		return rc;
	}
	*result = msw << 16 | lsw;
	pr_debug("msw = %04x lsw = %04x cc = %d\n", msw, lsw, *result);
	return 0;
}

static int adjust_xo_vbatt_reading_for_mbg(struct pm8921_bms_chip *chip,
						int result)
{
	int64_t numerator;
	int64_t denominator;

	if (chip->amux_2_trim_delta == 0)
		return result;

	numerator = (s64)result * 1000000;
	denominator = (1000000 + (410 * (s64)chip->amux_2_trim_delta));
	return div_s64(numerator, denominator);
}

static int convert_vbatt_raw_to_uv(struct pm8921_bms_chip *chip,
					int usb_chg,
					uint16_t reading, int *result)
{
	*result = xoadc_reading_to_microvolt(reading);
	pr_debug("raw = %04x vbatt = %u\n", reading, *result);
	*result = adjust_xo_vbatt_reading(chip, usb_chg, *result);
	pr_debug("after adj vbatt = %u\n", *result);
	*result = adjust_xo_vbatt_reading_for_mbg(chip, *result);
	return 0;
}

static int convert_vsense_to_uv(struct pm8921_bms_chip *chip,
					int16_t reading, int *result)
{
	*result = pm8xxx_ccadc_reading_to_microvolt(chip->revision, reading);
	pr_debug("raw = %04x vsense = %d\n", reading, *result);
	*result = pm8xxx_cc_adjust_for_gain(*result);
	pr_debug("after adj vsense = %d\n", *result);
	return 0;
}

static int read_vsense_avg(struct pm8921_bms_chip *chip, int *result)
{
	int rc;
	int16_t reading;

	rc = pm_bms_read_output_data(chip, VSENSE_AVG, &reading);
	if (rc) {
		pr_err("fail to read VSENSE_AVG rc = %d\n", rc);
		return rc;
	}

	convert_vsense_to_uv(chip, reading, result);
	return 0;
}

static int linear_interpolate(int y0, int x0, int y1, int x1, int x)
{
	if (y0 == y1 || x == x0)
		return y0;
	if (x1 == x0 || x == x1)
		return y1;

	return y0 + ((y1 - y0) * (x - x0) / (x1 - x0));
}

static int interpolate_single_lut(struct single_row_lut *lut, int x)
{
	int i, result;

	if (x < lut->x[0]) {
		pr_debug("x %d less than known range return y = %d lut = %pS\n",
							x, lut->y[0], lut);
		return lut->y[0];
	}
	if (x > lut->x[lut->cols - 1]) {
		pr_debug("x %d more than known range return y = %d lut = %pS\n",
						x, lut->y[lut->cols - 1], lut);
		return lut->y[lut->cols - 1];
	}

	for (i = 0; i < lut->cols; i++)
		if (x <= lut->x[i])
			break;
	if (x == lut->x[i]) {
		result = lut->y[i];
	} else {
		result = linear_interpolate(
			lut->y[i - 1],
			lut->x[i - 1],
			lut->y[i],
			lut->x[i],
			x);
	}
	return result;
}

static int interpolate_fcc(struct pm8921_bms_chip *chip, int batt_temp)
{
	/* batt_temp is in tenths of degC - convert it to degC for lookups */
	batt_temp = batt_temp/10;
	return interpolate_single_lut(chip->fcc_temp_lut, batt_temp);
}

static int interpolate_fcc_adjusted(struct pm8921_bms_chip *chip, int batt_temp)
{
	/* batt_temp is in tenths of degC - convert it to degC for lookups */
	batt_temp = batt_temp/10;
	return interpolate_single_lut(chip->adjusted_fcc_temp_lut, batt_temp);
}

static int interpolate_scalingfactor_fcc(struct pm8921_bms_chip *chip,
								int cycles)
{
	/*
	 * sf table could be null when no battery aging data is available, in
	 * that case return 100%
	 */
	if (chip->fcc_sf_lut)
		return interpolate_single_lut(chip->fcc_sf_lut, cycles);
	else
		return 100;
}

static int interpolate_scalingfactor(struct pm8921_bms_chip *chip,
				struct sf_lut *sf_lut,
				int row_entry, int pc)
{
	int i, scalefactorrow1, scalefactorrow2, scalefactor;
	int rows, cols;
	int row1 = 0;
	int row2 = 0;

	/*
	 * sf table could be null when no battery aging data is available, in
	 * that case return 100%
	 */
	if (!sf_lut)
		return 100;

	rows = sf_lut->rows;
	cols = sf_lut->cols;
	if (pc > sf_lut->percent[0]) {
		pr_debug("pc %d greater than known pc ranges for sfd\n", pc);
		row1 = 0;
		row2 = 0;
	}
	if (pc < sf_lut->percent[rows - 1]) {
		pr_debug("pc %d less than known pc ranges for sf", pc);
		row1 = rows - 1;
		row2 = rows - 1;
	}
	for (i = 0; i < rows; i++) {
		if (pc == sf_lut->percent[i]) {
			row1 = i;
			row2 = i;
			break;
		}
		if (pc > sf_lut->percent[i]) {
			row1 = i - 1;
			row2 = i;
			break;
		}
	}

	if (row_entry < sf_lut->row_entries[0])
		row_entry = sf_lut->row_entries[0];
	if (row_entry > sf_lut->row_entries[cols - 1])
		row_entry = sf_lut->row_entries[cols - 1];

	for (i = 0; i < cols; i++)
		if (row_entry <= sf_lut->row_entries[i])
			break;
	if (row_entry == sf_lut->row_entries[i]) {
		scalefactor = linear_interpolate(
				sf_lut->sf[row1][i],
				sf_lut->percent[row1],
				sf_lut->sf[row2][i],
				sf_lut->percent[row2],
				pc);
		return scalefactor;
	}

	scalefactorrow1 = linear_interpolate(
				sf_lut->sf[row1][i - 1],
				sf_lut->row_entries[i - 1],
				sf_lut->sf[row1][i],
				sf_lut->row_entries[i],
				row_entry);

	scalefactorrow2 = linear_interpolate(
				sf_lut->sf[row2][i - 1],
				sf_lut->row_entries[i - 1],
				sf_lut->sf[row2][i],
				sf_lut->row_entries[i],
				row_entry);

	scalefactor = linear_interpolate(
				scalefactorrow1,
				sf_lut->percent[row1],
				scalefactorrow2,
				sf_lut->percent[row2],
				pc);

	return scalefactor;
}

static int is_between(int left, int right, int value)
{
	if (left >= right && left >= value && value >= right)
		return 1;
	if (left <= right && left <= value && value <= right)
		return 1;

	return 0;
}

/* get ocv given a soc  -- reverse lookup */
static int interpolate_ocv(struct pm8921_bms_chip *chip,
				int batt_temp_degc, int pc)
{
	int i, ocvrow1, ocvrow2, ocv;
	int rows, cols;
	int row1 = 0;
	int row2 = 0;

	rows = chip->pc_temp_ocv_lut->rows;
	cols = chip->pc_temp_ocv_lut->cols;
	if (pc > chip->pc_temp_ocv_lut->percent[0]) {
		pr_debug("pc %d greater than known pc ranges for sfd\n", pc);
		row1 = 0;
		row2 = 0;
	}
	if (pc < chip->pc_temp_ocv_lut->percent[rows - 1]) {
		pr_debug("pc %d less than known pc ranges for sf\n", pc);
		row1 = rows - 1;
		row2 = rows - 1;
	}
	for (i = 0; i < rows; i++) {
		if (pc == chip->pc_temp_ocv_lut->percent[i]) {
			row1 = i;
			row2 = i;
			break;
		}
		if (pc > chip->pc_temp_ocv_lut->percent[i]) {
			row1 = i - 1;
			row2 = i;
			break;
		}
	}

	if (batt_temp_degc < chip->pc_temp_ocv_lut->temp[0])
		batt_temp_degc = chip->pc_temp_ocv_lut->temp[0];
	if (batt_temp_degc > chip->pc_temp_ocv_lut->temp[cols - 1])
		batt_temp_degc = chip->pc_temp_ocv_lut->temp[cols - 1];

	for (i = 0; i < cols; i++)
		if (batt_temp_degc <= chip->pc_temp_ocv_lut->temp[i])
			break;
	if (batt_temp_degc == chip->pc_temp_ocv_lut->temp[i]) {
		ocv = linear_interpolate(
				chip->pc_temp_ocv_lut->ocv[row1][i],
				chip->pc_temp_ocv_lut->percent[row1],
				chip->pc_temp_ocv_lut->ocv[row2][i],
				chip->pc_temp_ocv_lut->percent[row2],
				pc);
		return ocv;
	}

	ocvrow1 = linear_interpolate(
				chip->pc_temp_ocv_lut->ocv[row1][i - 1],
				chip->pc_temp_ocv_lut->temp[i - 1],
				chip->pc_temp_ocv_lut->ocv[row1][i],
				chip->pc_temp_ocv_lut->temp[i],
				batt_temp_degc);

	ocvrow2 = linear_interpolate(
				chip->pc_temp_ocv_lut->ocv[row2][i - 1],
				chip->pc_temp_ocv_lut->temp[i - 1],
				chip->pc_temp_ocv_lut->ocv[row2][i],
				chip->pc_temp_ocv_lut->temp[i],
				batt_temp_degc);

	ocv = linear_interpolate(
				ocvrow1,
				chip->pc_temp_ocv_lut->percent[row1],
				ocvrow2,
				chip->pc_temp_ocv_lut->percent[row2],
				pc);

	return ocv;
}

static int interpolate_pc(struct pm8921_bms_chip *chip,
				int batt_temp_degc, int ocv)
{
	int i, j, pcj, pcj_minus_one, pc;
	int rows = chip->pc_temp_ocv_lut->rows;
	int cols = chip->pc_temp_ocv_lut->cols;


	if (batt_temp_degc < chip->pc_temp_ocv_lut->temp[0]) {
		pr_debug("batt_temp %d < known temp range\n", batt_temp_degc);
		batt_temp_degc = chip->pc_temp_ocv_lut->temp[0];
	}
	if (batt_temp_degc > chip->pc_temp_ocv_lut->temp[cols - 1]) {
		pr_debug("batt_temp %d > known temp range\n", batt_temp_degc);
		batt_temp_degc = chip->pc_temp_ocv_lut->temp[cols - 1];
	}

	for (j = 0; j < cols; j++)
		if (batt_temp_degc <= chip->pc_temp_ocv_lut->temp[j])
			break;
	if (batt_temp_degc == chip->pc_temp_ocv_lut->temp[j]) {
		/* found an exact match for temp in the table */
		if (ocv >= chip->pc_temp_ocv_lut->ocv[0][j])
			return chip->pc_temp_ocv_lut->percent[0];
		if (ocv <= chip->pc_temp_ocv_lut->ocv[rows - 1][j])
			return chip->pc_temp_ocv_lut->percent[rows - 1];
		for (i = 0; i < rows; i++) {
			if (ocv >= chip->pc_temp_ocv_lut->ocv[i][j]) {
				if (ocv == chip->pc_temp_ocv_lut->ocv[i][j])
					return
					chip->pc_temp_ocv_lut->percent[i];
				pc = linear_interpolate(
					chip->pc_temp_ocv_lut->percent[i],
					chip->pc_temp_ocv_lut->ocv[i][j],
					chip->pc_temp_ocv_lut->percent[i - 1],
					chip->pc_temp_ocv_lut->ocv[i - 1][j],
					ocv);
				return pc;
			}
		}
	}

	/*
	 * batt_temp_degc is within temperature for
	 * column j-1 and j
	 */
	if (ocv >= chip->pc_temp_ocv_lut->ocv[0][j])
		return chip->pc_temp_ocv_lut->percent[0];
	if (ocv <= chip->pc_temp_ocv_lut->ocv[rows - 1][j - 1])
		return chip->pc_temp_ocv_lut->percent[rows - 1];

	pcj_minus_one = 0;
	pcj = 0;
	for (i = 0; i < rows-1; i++) {
		if (pcj == 0
			&& is_between(chip->pc_temp_ocv_lut->ocv[i][j],
				chip->pc_temp_ocv_lut->ocv[i+1][j], ocv)) {
			pcj = linear_interpolate(
				chip->pc_temp_ocv_lut->percent[i],
				chip->pc_temp_ocv_lut->ocv[i][j],
				chip->pc_temp_ocv_lut->percent[i + 1],
				chip->pc_temp_ocv_lut->ocv[i+1][j],
				ocv);
		}

		if (pcj_minus_one == 0
			&& is_between(chip->pc_temp_ocv_lut->ocv[i][j-1],
				chip->pc_temp_ocv_lut->ocv[i+1][j-1], ocv)) {

			pcj_minus_one = linear_interpolate(
				chip->pc_temp_ocv_lut->percent[i],
				chip->pc_temp_ocv_lut->ocv[i][j-1],
				chip->pc_temp_ocv_lut->percent[i + 1],
				chip->pc_temp_ocv_lut->ocv[i+1][j-1],
				ocv);
		}

		if (pcj && pcj_minus_one) {
			pc = linear_interpolate(
				pcj_minus_one,
				chip->pc_temp_ocv_lut->temp[j-1],
				pcj,
				chip->pc_temp_ocv_lut->temp[j],
				batt_temp_degc);
			return pc;
		}
	}

	if (pcj)
		return pcj;

	if (pcj_minus_one)
		return pcj_minus_one;

	pr_debug("%d ocv wasn't found for temp %d in the LUT returning 100%%",
							ocv, batt_temp_degc);
	return 100;
}

#define BMS_MODE_BIT	BIT(6)
#define EN_VBAT_BIT	BIT(5)
#define OVERRIDE_MODE_DELAY_MS	20
int override_mode_simultaneous_battery_voltage_and_current(int *ibat_ua,
								int *vbat_uv)
{
	int16_t vsense_raw;
	int16_t vbat_raw;
	int vsense_uv;
	int usb_chg;

	mutex_lock(&the_chip->bms_output_lock);

	pm8xxx_writeb(the_chip->dev->parent, BMS_S1_DELAY, 0x00);
	pm_bms_masked_write(the_chip, BMS_CONTROL,
			BMS_MODE_BIT | EN_VBAT_BIT, BMS_MODE_BIT | EN_VBAT_BIT);

	msleep(OVERRIDE_MODE_DELAY_MS);

	pm_bms_lock_output_data(the_chip);
	pm_bms_read_output_data(the_chip, VSENSE_AVG, &vsense_raw);
	pm_bms_read_output_data(the_chip, VBATT_AVG, &vbat_raw);
	pm_bms_unlock_output_data(the_chip);
	pm_bms_masked_write(the_chip, BMS_CONTROL,
			BMS_MODE_BIT | EN_VBAT_BIT, 0);

	pm8xxx_writeb(the_chip->dev->parent, BMS_S1_DELAY, 0x0B);

	mutex_unlock(&the_chip->bms_output_lock);

	usb_chg = usb_chg_plugged_in(the_chip);

	convert_vbatt_raw_to_uv(the_chip, usb_chg, vbat_raw, vbat_uv);
	convert_vsense_to_uv(the_chip, vsense_raw, &vsense_uv);
	*ibat_ua = vsense_uv * 1000 / (int)the_chip->r_sense;

	pr_debug("vsense_raw = 0x%x vbat_raw = 0x%x"
			" ibat_ua = %d vbat_uv = %d\n",
			(uint16_t)vsense_raw, (uint16_t)vbat_raw,
			*ibat_ua, *vbat_uv);

	
	the_chip->bmsmon_data.ibat_ua = *ibat_ua;
	the_chip->bmsmon_data.vbat_uv = *vbat_uv;
	the_chip->bmsmon_data.vsense_uv = vsense_uv;
	

	return 0;
}

#define MBG_TRANSIENT_ERROR_RAW 51
static void adjust_pon_ocv_raw(struct pm8921_bms_chip *chip,
				struct pm8921_soc_params *raw)
{
	/* in 8921 parts the PON ocv is taken when the MBG is not settled.
	 * decrease the pon ocv by 15mV raw value to account for it
	 * Since a 1/3rd  of vbatt is supplied to the adc the raw value
	 * needs to be adjusted by 5mV worth bits
	 */
	if (raw->last_good_ocv_raw >= MBG_TRANSIENT_ERROR_RAW)
		raw->last_good_ocv_raw -= MBG_TRANSIENT_ERROR_RAW;
}







#define CC_RAW_5MAH		0x00110000
#define MIN_OCV_UV		2000000
#define OCV_RAW_UNINITIALIZED	0xFFFF
static int read_soc_params_raw(struct pm8921_bms_chip *chip,
				struct pm8921_soc_params *raw)
{
	int usb_chg;

	mutex_lock(&chip->bms_output_lock);
	pm_bms_lock_output_data(chip);

	pm_bms_read_output_data(chip,
			LAST_GOOD_OCV_VALUE, &raw->last_good_ocv_raw);
	read_cc(chip, &raw->cc);

	pm_bms_unlock_output_data(chip);
	mutex_unlock(&chip->bms_output_lock);

	usb_chg =  usb_chg_plugged_in(chip);

	if (chip->prev_last_good_ocv_raw == OCV_RAW_UNINITIALIZED) {
		chip->prev_last_good_ocv_raw = raw->last_good_ocv_raw;
		adjust_pon_ocv_raw(chip, raw);
		convert_vbatt_raw_to_uv(chip, usb_chg,
			raw->last_good_ocv_raw, &raw->last_good_ocv_uv);
		chip->last_ocv_uv = raw->last_good_ocv_uv;
		pr_debug("PON_OCV_UV = %d\n", chip->last_ocv_uv);
	} else if (chip->prev_last_good_ocv_raw != raw->last_good_ocv_raw) {
		
		pr_debug("prev_last_good_ocv_raw != last_good_ocv_raw\n");
		
		
		the_chip->bmsmon_data.ocv_renew_cnt++;
		
		chip->prev_last_good_ocv_raw = raw->last_good_ocv_raw;
		convert_vbatt_raw_to_uv(chip, usb_chg,
			raw->last_good_ocv_raw, &raw->last_good_ocv_uv);
		chip->last_ocv_uv = raw->last_good_ocv_uv;
		/* forget the old cc value upon ocv */
		chip->last_cc_uah = 0;
	} else {
		raw->last_good_ocv_uv = chip->last_ocv_uv;
		
		
		pr_debug("last_good_ocv_uv = last_ocv_uv\n");
		
		
	}

	/* fake a high OCV if we are just done charging */
	if (chip->ocv_reading_at_100 != raw->last_good_ocv_raw) {
		chip->ocv_reading_at_100 = OCV_RAW_UNINITIALIZED;
		chip->cc_reading_at_100 = 0;
	} else {
		/*
		 * force 100% ocv by selecting the highest voltage the
		 * battery could ever reach
		 */
		raw->last_good_ocv_uv = chip->max_voltage_uv;
		chip->last_ocv_uv = chip->max_voltage_uv;
	}
	pr_debug("0p625 = %duV\n", chip->xoadc_v0625);
	pr_debug("1p25 = %duV\n", chip->xoadc_v125);
	pr_debug("last_good_ocv_raw= 0x%x, last_good_ocv_uv= %duV\n",
			raw->last_good_ocv_raw, raw->last_good_ocv_uv);
	pr_debug("cc_raw= 0x%x\n", raw->cc);
	return 0;
}

static int get_rbatt(struct pm8921_bms_chip *chip, int soc_rbatt, int batt_temp)
{
	int rbatt, scalefactor;

	rbatt = chip->default_rbatt_mohm;
	pr_debug("rbatt before scaling = %d\n", rbatt);
	if (chip->rbatt_sf_lut == NULL)  {
		pr_debug("RBATT = %d\n", rbatt);
		return rbatt;
	}
	/* Convert the batt_temp to DegC from deciDegC */
	batt_temp = batt_temp / 10;
	scalefactor = interpolate_scalingfactor(chip, chip->rbatt_sf_lut,
							batt_temp, soc_rbatt);
	pr_debug("rbatt sf = %d for batt_temp = %d, soc_rbatt = %d\n",
				scalefactor, batt_temp, soc_rbatt);
	rbatt = (rbatt * scalefactor) / 100;

	rbatt += the_chip->rconn_mohm;
	pr_debug("adding rconn_mohm = %d rbatt = %d\n",
				the_chip->rconn_mohm, rbatt);

	if (is_between(20, 10, soc_rbatt))
		rbatt = rbatt
			+ ((20 - soc_rbatt) * chip->delta_rbatt_mohm) / 10;
	else
		if (is_between(10, 0, soc_rbatt))
			rbatt = rbatt + chip->delta_rbatt_mohm;

	pr_debug("RBATT = %d\n", rbatt);
	return rbatt;
}

static int calculate_fcc_uah(struct pm8921_bms_chip *chip, int batt_temp,
							int chargecycles)
{
	int initfcc, result, scalefactor = 0;

	if (chip->adjusted_fcc_temp_lut == NULL) {
		initfcc = interpolate_fcc(chip, batt_temp);

		scalefactor = interpolate_scalingfactor_fcc(chip, chargecycles);

		/* Multiply the initial FCC value by the scale factor. */
		result = (initfcc * scalefactor * 1000) / 100;
		pr_debug("fcc = %d uAh\n", result);
		return result;
	} else {
		return 1000 * interpolate_fcc_adjusted(chip, batt_temp);
	}
}

static int get_battery_uvolts(struct pm8921_bms_chip *chip, int *uvolts)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(chip->vbat_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->vbat_channel, rc);
		return rc;
	}
	pr_debug("mvolts phy = %lld meas = 0x%llx", result.physical,
						result.measurement);
	*uvolts = (int)result.physical;
	return 0;
}

static int adc_based_ocv(struct pm8921_bms_chip *chip, int *ocv)
{
	int vbatt, rbatt, ibatt_ua, rc;

	rc = get_battery_uvolts(chip, &vbatt);
	if (rc) {
		pr_err("failed to read vbatt from adc rc = %d\n", rc);
		return rc;
	}

	rc =  pm8921_bms_get_battery_current(&ibatt_ua);
	if (rc) {
		pr_err("failed to read batt current rc = %d\n", rc);
		return rc;
	}

	rbatt = chip->default_rbatt_mohm;
	*ocv = vbatt + (ibatt_ua * rbatt)/1000;
	return 0;
}

static int calculate_pc(struct pm8921_bms_chip *chip, int ocv_uv, int batt_temp,
							int chargecycles)
{
	int pc, scalefactor;

	pc = interpolate_pc(chip, batt_temp / 10, ocv_uv / 1000);
	pr_debug("pc = %u for ocv = %dmicroVolts batt_temp = %d\n",
					pc, ocv_uv, batt_temp);

	scalefactor = interpolate_scalingfactor(chip,
					chip->pc_sf_lut, chargecycles, pc);
	pr_debug("scalefactor = %u batt_temp = %d\n", scalefactor, batt_temp);

	/* Multiply the initial FCC value by the scale factor. */
	pc = (pc * scalefactor) / 100;
	return pc;
}

/**
 * calculate_cc_uah -
 * @chip:		the bms chip pointer
 * @cc:			the cc reading from bms h/w
 * @val:		return value
 * @coulumb_counter:	adjusted coulumb counter for 100%
 *
 * RETURNS: in val pointer coulumb counter based charger in uAh
 *          (micro Amp hour)
 */
static void calculate_cc_uah(struct pm8921_bms_chip *chip, int cc, int *val)
{
	int64_t cc_voltage_uv, cc_nvh, cc_uah;

	cc_voltage_uv = cc;
	cc_voltage_uv -= chip->cc_reading_at_100;
	pr_debug("cc = %d. after subtracting 0x%x cc = %lld\n",
					cc, chip->cc_reading_at_100,
					cc_voltage_uv);
	cc_voltage_uv = cc_to_microvolt(chip, cc_voltage_uv);
	cc_voltage_uv = pm8xxx_cc_adjust_for_gain(cc_voltage_uv);
	pr_debug("cc_voltage_uv = %lld microvolts\n", cc_voltage_uv);
	cc_nvh = ccmicrovolt_to_nvh(cc_voltage_uv);
	pr_debug("cc_nvh = %lld nano_volt_hour\n", cc_nvh);
	cc_uah = div_s64(cc_nvh, chip->r_sense);
	*val = cc_uah;
}

static int calculate_termination_uuc(struct pm8921_bms_chip *chip,
				 int batt_temp, int chargecycles,
				int fcc_uah, int i_ma,
				int *ret_pc_unusable)
{
	int unusable_uv, pc_unusable, uuc;
	int i = 0;
	int ocv_mv;
	int batt_temp_degc = batt_temp / 10;
	int rbatt_mohm;
	int delta_uv;
	int prev_delta_uv = 0;
	int prev_rbatt_mohm = 0;
	int prev_ocv_mv = 0;
	int uuc_rbatt_uv;

	for (i = 0; i <= 100; i++) {
		ocv_mv = interpolate_ocv(chip, batt_temp_degc, i);
		rbatt_mohm = get_rbatt(chip, i, batt_temp);
		unusable_uv = (rbatt_mohm * i_ma) + (chip->v_cutoff * 1000);
		delta_uv = ocv_mv * 1000 - unusable_uv;

		pr_debug("soc = %d ocv = %d rbat = %d u_uv = %d delta_v = %d\n",
				i, ocv_mv, rbatt_mohm, unusable_uv, delta_uv);

		if (delta_uv > 0)
			break;

		prev_delta_uv = delta_uv;
		prev_rbatt_mohm = rbatt_mohm;
		prev_ocv_mv = ocv_mv;
	}

	uuc_rbatt_uv = linear_interpolate(rbatt_mohm, delta_uv,
					prev_rbatt_mohm, prev_delta_uv,
					0);

	unusable_uv = (uuc_rbatt_uv * i_ma) + (chip->v_cutoff * 1000);

	pc_unusable = calculate_pc(chip, unusable_uv, batt_temp, chargecycles);
	
	chip->bmsmon_data.pc_uuc = pc_unusable;
	
	uuc = (fcc_uah * pc_unusable) / 100;
	pr_debug("For i_ma = %d, unusable_rbatt = %d unusable_uv = %d unusable_pc = %d uuc = %d\n",
					i_ma, uuc_rbatt_uv, unusable_uv,
					pc_unusable, uuc);
	*ret_pc_unusable = pc_unusable;
	return uuc;
}

static int adjust_uuc(struct pm8921_bms_chip *chip, int fcc_uah,
			int new_pc_unusable,
			int new_uuc,
			int batt_temp,
			int rbatt,
			int *iavg_ma)
{
	int new_unusable_mv;
	int batt_temp_degc = batt_temp / 10;

	if (chip->prev_pc_unusable == -EINVAL
		|| abs(chip->prev_pc_unusable - new_pc_unusable) <= 1) {
		chip->prev_pc_unusable = new_pc_unusable;
		return new_uuc;
	}

	/* the uuc is trying to change more than 1% restrict it */
	if (new_pc_unusable > chip->prev_pc_unusable)
		chip->prev_pc_unusable++;
	else
		chip->prev_pc_unusable--;

	new_uuc = (fcc_uah * chip->prev_pc_unusable) / 100;

	/* also find update the iavg_ma accordingly */
	new_unusable_mv = interpolate_ocv(chip, batt_temp_degc,
						chip->prev_pc_unusable);
	if (new_unusable_mv < chip->v_cutoff)
		new_unusable_mv = chip->v_cutoff;

	*iavg_ma = (new_unusable_mv - chip->v_cutoff) * 1000 / rbatt;
	if (*iavg_ma == 0)
		*iavg_ma = 1;
	pr_debug("Restricting UUC to %d (%d%%) unusable_mv = %d iavg_ma = %d\n",
					new_uuc, chip->prev_pc_unusable,
					new_unusable_mv, *iavg_ma);

	return new_uuc;
}

static void calculate_iavg_ua(struct pm8921_bms_chip *chip, int cc_uah,
				int *iavg_ua, int *delta_time_s)
{
	int delta_cc_uah;
	struct rtc_time tm;
	struct rtc_device *rtc;
	unsigned long now_tm_sec = 0;
	int rc = 0;

	/* if anything fails report the previous iavg_ua */
	*iavg_ua = chip->prev_iavg_ua;

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (rtc == NULL) {
		pr_err("%s: unable to open rtc device (%s)\n",
			__FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		goto out;
	}

	rc = rtc_read_time(rtc, &tm);
	if (rc) {
		pr_err("Error reading rtc device (%s) : %d\n",
			CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto out;
	}

	rc = rtc_valid_tm(&tm);
	if (rc) {
		pr_err("Invalid RTC time (%s): %d\n",
			CONFIG_RTC_HCTOSYS_DEVICE, rc);
		goto out;
	}
	rtc_tm_to_time(&tm, &now_tm_sec);

	if (chip->tm_sec == 0) {
		*delta_time_s = 0;
		pm8921_bms_get_battery_current(iavg_ua);
		goto out;
	}

	*delta_time_s = (now_tm_sec - chip->tm_sec);

	/* use the previous iavg if called within 15 seconds */
	if (*delta_time_s < 15) {
		*iavg_ua = chip->prev_iavg_ua;
		goto out;
	}

	delta_cc_uah = cc_uah - chip->last_cc_uah;

	*iavg_ua = div_s64((s64)delta_cc_uah * 3600, *delta_time_s);

	pr_debug("tm_sec = %ld, now_tm_sec = %ld delta_s = %d delta_cc = %d iavg_ua = %d\n",
				chip->tm_sec, now_tm_sec,
				*delta_time_s, delta_cc_uah, (int)*iavg_ua);

out:
	/* remember the iavg */
	chip->prev_iavg_ua = *iavg_ua;

	/* remember cc_uah */
	chip->last_cc_uah = cc_uah;

	/* remember this time */
	chip->tm_sec = now_tm_sec;

	
	chip->bmsmon_data.iavg_ua = *iavg_ua;
	
}

#define IAVG_SAMPLES 16
#define CHARGING_IAVG_MA 250
#define MIN_SECONDS_FOR_VALID_SAMPLE	20
static int calculate_unusable_charge_uah(struct pm8921_bms_chip *chip,
				int rbatt, int fcc_uah, int cc_uah,
				int soc_rbatt, int batt_temp, int chargecycles,
				int iavg_ua, int delta_time_s)
{
	int uuc_uah_iavg;
	int i;
	int iavg_ma = iavg_ua / 1000;
	static int iavg_samples[IAVG_SAMPLES];
	static int iavg_index;
	static int iavg_num_samples;
	static int firsttime = 1;
	int pc_unusable;

	/*
	 * if we are called first time fill all the
	 * samples with the the shutdown_iavg_ua
	 */
	if (firsttime && chip->shutdown_iavg_ua != 0) {
		pr_emerg("Using shutdown_iavg_ua = %d in all samples\n",
				chip->shutdown_iavg_ua);
		for (i = 0; i < IAVG_SAMPLES; i++)
			iavg_samples[i] = chip->shutdown_iavg_ua;

		iavg_index = 0;
		iavg_num_samples = IAVG_SAMPLES;
	}

	/*
	 * if we are charging use a nominal avg current so that we keep
	 * a reasonable UUC while charging
	 */
	if (iavg_ma < 0)
		iavg_ma = CHARGING_IAVG_MA;
	iavg_samples[iavg_index] = iavg_ma;
	iavg_index = (iavg_index + 1) % IAVG_SAMPLES;
	iavg_num_samples++;
	if (iavg_num_samples >= IAVG_SAMPLES)
		iavg_num_samples = IAVG_SAMPLES;

	/* now that this sample is added calcualte the average */
	iavg_ma = 0;
	if (iavg_num_samples != 0) {
		for (i = 0; i < iavg_num_samples; i++) {
			pr_debug("iavg_samples[%d] = %d\n", i, iavg_samples[i]);
			iavg_ma += iavg_samples[i];
		}

		iavg_ma = DIV_ROUND_CLOSEST(iavg_ma, iavg_num_samples);
	}

	uuc_uah_iavg = calculate_termination_uuc(chip,
					batt_temp, chargecycles,
					fcc_uah, iavg_ma,
					&pc_unusable);
	pr_debug("iavg = %d uuc_iavg = %d\n", iavg_ma, uuc_uah_iavg);

	/* restrict the uuc such that it can increase only by one percent */
	uuc_uah_iavg = adjust_uuc(chip, fcc_uah, pc_unusable, uuc_uah_iavg,
					batt_temp, rbatt, &iavg_ma);

	/* find out what the avg current should be for this uuc */
	chip->prev_uuc_iavg_ma = iavg_ma;

	firsttime = 0;
	return uuc_uah_iavg;
}

/* calculate remainging charge at the time of ocv */
static int calculate_remaining_charge_uah(struct pm8921_bms_chip *chip,
						struct pm8921_soc_params *raw,
						int fcc_uah, int batt_temp,
						int chargecycles)
{
	int  ocv, pc;

	ocv = raw->last_good_ocv_uv;
	pc = calculate_pc(chip, ocv, batt_temp, chargecycles);
	pr_debug("ocv = %d pc = %d\n", ocv, pc);
	
	chip->bmsmon_data.pc_rc = pc;
	
	return (fcc_uah * pc) / 100;
}

static void calculate_soc_params(struct pm8921_bms_chip *chip,
						struct pm8921_soc_params *raw,
						int batt_temp, int chargecycles,
						int *fcc_uah,
						int *unusable_charge_uah,
						int *remaining_charge_uah,
						int *cc_uah,
						int *rbatt,
						int *iavg_ua,
						int *delta_time_s)
{
	int soc_rbatt;

	*fcc_uah = calculate_fcc_uah(chip, batt_temp, chargecycles);
	pr_debug("FCC = %uuAh batt_temp = %d, cycles = %d\n",
					*fcc_uah, batt_temp, chargecycles);


	/* calculate remainging charge */
	*remaining_charge_uah = calculate_remaining_charge_uah(chip, raw,
					*fcc_uah, batt_temp, chargecycles);
	pr_debug("RC = %uuAh\n", *remaining_charge_uah);

	/* calculate cc micro_volt_hour */
	calculate_cc_uah(chip, raw->cc, cc_uah);
	pr_debug("cc_uah = %duAh raw->cc = %x cc = %lld after subtracting %x\n",
				*cc_uah, raw->cc,
				(int64_t)raw->cc - chip->cc_reading_at_100,
				chip->cc_reading_at_100);

	soc_rbatt = ((*remaining_charge_uah - *cc_uah) * 100) / *fcc_uah;
	if (soc_rbatt < 0)
		soc_rbatt = 0;
	*rbatt = get_rbatt(chip, soc_rbatt, batt_temp);

	calculate_iavg_ua(chip, *cc_uah, iavg_ua, delta_time_s);

	*unusable_charge_uah = calculate_unusable_charge_uah(chip, *rbatt,
					*fcc_uah, *cc_uah, soc_rbatt,
					batt_temp, chargecycles, *iavg_ua,
					*delta_time_s);
	pr_debug("UUC = %uuAh\n", *unusable_charge_uah);
	
	chip->bmsmon_data.batt_temp = batt_temp;
	chip->bmsmon_data.last_good_ocv_uv = raw->last_good_ocv_uv;
	chip->bmsmon_data.fcc_uah = *fcc_uah;
	chip->bmsmon_data.uuc_uah = *unusable_charge_uah;
	chip->bmsmon_data.rc_uah = *remaining_charge_uah;
	chip->bmsmon_data.cc_uah = *cc_uah;
	chip->bmsmon_data.pc_cc = (*cc_uah * 100) / *fcc_uah;
	chip->bmsmon_data.rbatt = *rbatt;
	
}

static int calculate_real_fcc_uah(struct pm8921_bms_chip *chip,
				struct pm8921_soc_params *raw,
				int batt_temp, int chargecycles,
				int *ret_fcc_uah)
{
	int fcc_uah, unusable_charge_uah;
	int remaining_charge_uah;
	int cc_uah;
	int real_fcc_uah;
	int rbatt;
	int iavg_ua;
	int delta_time_s;

	calculate_soc_params(chip, raw, batt_temp, chargecycles,
						&fcc_uah,
						&unusable_charge_uah,
						&remaining_charge_uah,
						&cc_uah,
						&rbatt,
						&iavg_ua,
						&delta_time_s);

	real_fcc_uah = remaining_charge_uah - cc_uah;
	*ret_fcc_uah = fcc_uah;
	pr_debug("real_fcc = %d, RC = %d CC = %d fcc = %d\n",
			real_fcc_uah, remaining_charge_uah, cc_uah, fcc_uah);
	return real_fcc_uah;
}

int pm8921_bms_get_simultaneous_battery_voltage_and_current(int *ibat_ua,
								int *vbat_uv)
{
	int rc;

	if (the_chip == NULL) {
		pr_err("Called too early\n");
		return -EINVAL;
	}

	if (pm8921_is_batfet_closed()) {
		return override_mode_simultaneous_battery_voltage_and_current(
								ibat_ua,
								vbat_uv);
	} else {
		pr_debug("batfet is open using separate vbat and ibat meas\n");
		rc = get_battery_uvolts(the_chip, vbat_uv);
		if (rc < 0) {
			pr_err("adc vbat failed err = %d\n", rc);
			return rc;
		}
		rc = pm8921_bms_get_battery_current(ibat_ua);
		if (rc < 0) {
			pr_err("bms ibat failed err = %d\n", rc);
			return rc;
		}
		
		the_chip->bmsmon_data.ibat_ua = *ibat_ua;
		the_chip->bmsmon_data.vbat_uv = *vbat_uv;
		
	}

	return 0;
}
EXPORT_SYMBOL(pm8921_bms_get_simultaneous_battery_voltage_and_current);

static void find_ocv_for_soc(struct pm8921_bms_chip *chip,
			int batt_temp,
			int chargecycles,
			int fcc_uah,
			int uuc_uah,
			int cc_uah,
			int shutdown_soc,
			int *rc_uah,
			int *ocv_uv)
{
	s64 rc;
	int pc, new_pc;
	int batt_temp_degc = batt_temp / 10;
	int ocv;

	int new_ocv = 0;
	int delta_mv = 5;
	int prev_delta_pc = 0;


	int cnt = 0;
	int ocv_renew_cnt = 0;


	rc = (s64)shutdown_soc * (fcc_uah - uuc_uah);
	rc = div_s64(rc, 100) + cc_uah + uuc_uah;
	pc = DIV_ROUND_CLOSEST((int)rc * 100, fcc_uah);
	pc = clamp(pc, 0, 100);

	ocv = interpolate_ocv(chip, batt_temp_degc, pc);

	
	pr_debug("batt_temp=%d, chargecycles=%d, fcc_uah=%d, "
		"uuc_uah=%d, cc_uah=%d, shutdown_soc=%d\n",
		batt_temp,
		chargecycles,
		fcc_uah,
		uuc_uah,
		cc_uah,
		shutdown_soc);
	

	pr_debug("s_soc = %d, fcc = %d uuc = %d rc = %d, pc = %d, ocv mv = %d\n",
			shutdown_soc, fcc_uah, uuc_uah, (int)rc, pc, ocv);
	new_pc = interpolate_pc(chip, batt_temp_degc, ocv);
	pr_debug("test revlookup pc = %d for ocv = %d\n", new_pc, ocv);


	while (abs(new_pc - pc) > 1) {










		if (new_pc > pc) {
			delta_mv = -1 * abs(delta_mv);
		} else {
			delta_mv = abs(delta_mv);
		}

		prev_delta_pc = abs(new_pc - pc);

		new_ocv = ocv + delta_mv;
		new_pc = interpolate_pc(chip, batt_temp_degc, new_ocv);

		if (abs(new_pc - pc) > prev_delta_pc) {
			if (abs(delta_mv) != 1) {
				delta_mv = 1;
				pr_debug("ocv is not renewed\n");
			} else {
				pr_debug("pc = %d, ocv = %d, delta_mv = %d\n",
						new_pc, ocv, delta_mv);
				break;
			}
		} else {
			ocv_renew_cnt++;
			ocv = new_ocv;
			pr_debug("pc = %d, ocv = %d, delta_mv = %d\n",
					new_pc, ocv, delta_mv);
		}

		
		pr_debug("new_pc=%d, pc=%d, ocv=%d, new_ocv=%d, delta_mv=%d, prev_delta_pc=%d\n",
			new_pc, pc, ocv, new_ocv, delta_mv, prev_delta_pc);
		

		
		cnt++;

		if (cnt >= 1000) {
			pr_warn("can't determine value: new_pc=%d, pc=%d, ocv=%d, delta_mv=%d, ocv_renew_cnt=%d\n",
					new_pc, pc, ocv, delta_mv, ocv_renew_cnt);
			break;
		}
		

	}


	*ocv_uv = ocv * 1000;
	*rc_uah = (int)rc;
}

static void adjust_rc_and_uuc_for_specific_soc(
						struct pm8921_bms_chip *chip,
						int batt_temp,
						int chargecycles,
						int soc,
						int fcc_uah,
						int uuc_uah,
						int cc_uah,
						int rc_uah,
						int rbatt,
						int *ret_ocv,
						int *ret_rc,
						int *ret_uuc,
						int *ret_rbatt)
{
	int ocv_uv;

	find_ocv_for_soc(chip, batt_temp, chargecycles,
					fcc_uah, uuc_uah, cc_uah,
					soc,
					&rc_uah, &ocv_uv);

	*ret_ocv = ocv_uv;
	*ret_rbatt = rbatt;
	*ret_rc = rc_uah;
	*ret_uuc = uuc_uah;
}
static int bound_soc(int soc)
{
	soc = max(0, soc);
	soc = min(100, soc);
	return soc;
}

static int charging_adjustments(struct pm8921_bms_chip *chip,
				int soc, int vbat_uv, int ibat_ua,
				int batt_temp, int chargecycles,
				int fcc_uah, int cc_uah, int uuc_uah)
{
	int chg_soc;

	if (chip->soc_at_cv == -EINVAL) {
		/* In constant current charging return the calc soc */
		if (vbat_uv <= chip->max_voltage_uv)
			pr_debug("CC CHG SOC %d\n", soc);

		/* Note the CC to CV point */
		if (vbat_uv >= chip->max_voltage_uv) {
			chip->soc_at_cv = soc;
			chip->prev_chg_soc = soc;
			chip->ibat_at_cv_ua = ibat_ua;
			pr_debug("CC_TO_CV ibat_ua = %d CHG SOC %d\n",
					ibat_ua, soc);
		}
		return soc;
	}

	/*
	 * battery is in CV phase - begin liner inerpolation of soc based on
	 * battery charge current
	 */

	/*
	 * if voltage lessened (possibly because of a system load)
	 * keep reporting the prev chg soc
	 */
	if (vbat_uv <= chip->max_voltage_uv) {
		pr_debug("vbat %d < max = %d CC CHG SOC %d\n",
			vbat_uv, chip->max_voltage_uv, chip->prev_chg_soc);
		return chip->prev_chg_soc;
	}

	chg_soc = linear_interpolate(chip->soc_at_cv, chip->ibat_at_cv_ua,
					100, -100000,
					ibat_ua);

	
	pr_debug("chg_soc=%d, soc_at_cv=%d, ibat_at_cv_ua=%d, ibat_ua=%d\n",
		chg_soc, chip->soc_at_cv, chip->ibat_at_cv_ua, ibat_ua);
	

	/* always report a higher soc */
	if (chg_soc > chip->prev_chg_soc) {
		int new_ocv_uv;
		int new_rc;

		chip->prev_chg_soc = chg_soc;

		find_ocv_for_soc(chip, batt_temp, chargecycles,
				fcc_uah, uuc_uah, cc_uah,
				chg_soc,
				&new_rc, &new_ocv_uv);
		the_chip->last_ocv_uv = new_ocv_uv;
		pr_debug("CC CHG ADJ OCV = %d CHG SOC %d\n",
				new_ocv_uv,
				chip->prev_chg_soc);
	}

	pr_debug("Reporting CHG SOC %d\n", chip->prev_chg_soc);
	return chip->prev_chg_soc;
}

static int last_soc_est = -EINVAL;
static int adjust_soc(struct pm8921_bms_chip *chip, int soc,
		int batt_temp, int chargecycles,
		int rbatt, int fcc_uah, int uuc_uah, int cc_uah)
{
	int ibat_ua = 0, vbat_uv = 0;
	int ocv_est_uv = 0, soc_est = 0, pc_est = 0, pc = 0;
	int delta_ocv_uv = 0;
	int n = 0;
	int rc_new_uah = 0;
	int pc_new = 0;
	int soc_new = 0;
	int m = 0;
	int rc = 0;
	int delta_ocv_uv_limit = 0;

	int cnt = 0;


	rc = pm8921_bms_get_simultaneous_battery_voltage_and_current(
							&ibat_ua,
							&vbat_uv);
	if (rc < 0) {
		pr_err("simultaneous vbat ibat failed err = %d\n", rc);
		goto out;
	}


	delta_ocv_uv_limit = DIV_ROUND_CLOSEST(ibat_ua, 1000);

	ocv_est_uv = vbat_uv + (ibat_ua * rbatt)/1000;
	pc_est = calculate_pc(chip, ocv_est_uv, batt_temp, last_chargecycles);
	soc_est = div_s64((s64)fcc_uah * pc_est - uuc_uah*100,
						(s64)fcc_uah - uuc_uah);
	soc_est = bound_soc(soc_est);

	
	the_chip->bmsmon_data.ocv_est_uv = ocv_est_uv;
	the_chip->bmsmon_data.pc_est = pc_est;
	the_chip->bmsmon_data.soc_est = soc_est;
	

	if (ibat_ua < 0) {
		soc = charging_adjustments(chip, soc, vbat_uv, ibat_ua,
				batt_temp, chargecycles,
				fcc_uah, cc_uah, uuc_uah);
		goto out;
	}

	/*
	 * do not adjust
	 * if soc is same as what bms calculated
	 * if soc_est is between 45 and 25, this is the flat portion of the
	 * curve where soc_est is not so accurate. We generally don't want to
	 * adjust when soc_est is inaccurate except for the cases when soc is
	 * way far off (higher than 50 or lesser than 20).
	 * Also don't adjust soc if it is above 90 becuase we might pull it low
	 * and  cause a bad user experience
	 */
	if (soc_est == soc
		|| (is_between(45, chip->adjust_soc_low_threshold, soc_est)
		&& is_between(50, chip->adjust_soc_low_threshold - 5, soc))
		|| soc >= 90)
		goto out;

	if (last_soc_est == -EINVAL)
		last_soc_est = soc;

	n = min(200, max(1 , soc + soc_est + last_soc_est));
	/* remember the last soc_est in last_soc_est */
	last_soc_est = soc_est;

	pc = calculate_pc(chip, chip->last_ocv_uv,
				batt_temp, last_chargecycles);
	if (pc > 0) {
		pc_new = calculate_pc(chip, chip->last_ocv_uv - (++m * 1000),
						batt_temp, last_chargecycles);
		while (pc_new == pc) {
			/* start taking 10mV steps */
			m = m + 10;
			pc_new = calculate_pc(chip,
						chip->last_ocv_uv - (m * 1000),
						batt_temp, last_chargecycles);

			
			cnt++;

			if (cnt >= 150) {
				pr_warn("can't determine value: pc_new=%d, pc=%d, m=%d\n",
						pc_new, pc, m);
				break;
			}
			
		}
	} else {
		/*
		 * pc is already at the lowest point,
		 * assume 1 millivolt translates to 1% pc
		 */
		pc = 1;
		pc_new = 0;
		m = 1;
	}

	delta_ocv_uv = div_s64((soc - soc_est) * (s64)m * 1000,
							n * (pc - pc_new));

	if (abs(delta_ocv_uv) > delta_ocv_uv_limit) {
		pr_debug("limiting delta ocv %d limit = %d\n", delta_ocv_uv,
				delta_ocv_uv_limit);

		if (delta_ocv_uv > 0)
			delta_ocv_uv = delta_ocv_uv_limit;
		else
			delta_ocv_uv = -1 * delta_ocv_uv_limit;
		pr_debug("new delta ocv = %d\n", delta_ocv_uv);
	}

	chip->last_ocv_uv -= delta_ocv_uv;

	if (chip->last_ocv_uv >= chip->max_voltage_uv)
		chip->last_ocv_uv = chip->max_voltage_uv;

	/* calculate the soc based on this new ocv */
	pc_new = calculate_pc(chip, chip->last_ocv_uv,
						batt_temp, last_chargecycles);
	rc_new_uah = (fcc_uah * pc_new) / 100;
	soc_new = (rc_new_uah - cc_uah - uuc_uah)*100 / (fcc_uah - uuc_uah);
	soc_new = bound_soc(soc_new);

	/*
	 * if soc_new is ZERO force it higher so that phone doesnt report soc=0
	 * soc = 0 should happen only when soc_est == 0
	 */
	if (soc_new == 0 && soc_est != 0)
		soc_new = 1;

	soc = soc_new;

out:
	pr_debug("ibat_ua = %d, vbat_uv = %d, ocv_est_uv = %d, pc_est = %d, "
		"soc_est = %d, n = %d, delta_ocv_uv = %d, last_ocv_uv = %d, "
		"pc_new = %d, soc_new = %d, rbatt = %d, m = %d\n",
		ibat_ua, vbat_uv, ocv_est_uv, pc_est,
		soc_est, n, delta_ocv_uv, chip->last_ocv_uv,
		pc_new, soc_new, rbatt, m);

	return soc;
}

#define IGNORE_SOC_TEMP_DECIDEG		50
#define IAVG_STEP_SIZE_MA	50
#define IAVG_START		400
#define SOC_ZERO		0xFF
static void backup_soc_and_iavg(struct pm8921_bms_chip *chip, int batt_temp,
				int soc)
{
	u8 temp;
	int iavg_ma = chip->prev_uuc_iavg_ma;

	if (iavg_ma > IAVG_START)
		temp = (iavg_ma - IAVG_START) / IAVG_STEP_SIZE_MA;
	else
		temp = 0;

	pm_bms_masked_write(chip, TEMP_IAVG_STORAGE,
			TEMP_IAVG_STORAGE_USE_MASK, temp);

	/* since only 6 bits are available for SOC, we store half the soc */
	if (soc == 0)
		temp = SOC_ZERO;
	else
		temp = soc;

	/* don't store soc if temperature is below 5degC */
	if (batt_temp > IGNORE_SOC_TEMP_DECIDEG)
		pm8xxx_writeb(the_chip->dev->parent, TEMP_SOC_STORAGE, temp);
}

static void read_shutdown_soc_and_iavg(struct pm8921_bms_chip *chip)
{
	int rc;
	u8 temp;

	rc = pm8xxx_readb(chip->dev->parent, TEMP_IAVG_STORAGE, &temp);
	if (rc) {
		pr_err("failed to read addr = %d %d assuming %d\n",
				TEMP_IAVG_STORAGE, rc, IAVG_START);
		chip->shutdown_iavg_ua = IAVG_START;
	} else {
		temp &= TEMP_IAVG_STORAGE_USE_MASK;

		if (temp == 0) {
			chip->shutdown_iavg_ua = IAVG_START;
		} else {
			chip->shutdown_iavg_ua = IAVG_START
					+ IAVG_STEP_SIZE_MA * (temp + 1);
		}
	}

	rc = pm8xxx_readb(chip->dev->parent, TEMP_SOC_STORAGE, &temp);
	if (rc) {
		pr_err("failed to read addr = %d %d\n", TEMP_SOC_STORAGE, rc);
	} else {
		chip->shutdown_soc = temp;

		if (chip->shutdown_soc == 0) {
			pr_debug("No shutdown soc available\n");
			shutdown_soc_invalid = 1;
			chip->shutdown_iavg_ua = 0;
		} else if (chip->shutdown_soc == SOC_ZERO) {
			chip->shutdown_soc = 0;
		}
	}

	if (chip->ignore_shutdown_soc) {
		shutdown_soc_invalid = 1;
		chip->shutdown_soc = 0;
		chip->shutdown_iavg_ua = 0;
	}

	pr_debug("shutdown_soc = %d shutdown_iavg = %d shutdown_soc_invalid = %d\n",
			chip->shutdown_soc,
			chip->shutdown_iavg_ua,
			shutdown_soc_invalid);
}

#define SOC_CATCHUP_SEC_MAX		600
#define SOC_CATCHUP_SEC_PER_PERCENT	60
#define MAX_CATCHUP_SOC	(SOC_CATCHUP_SEC_MAX/SOC_CATCHUP_SEC_PER_PERCENT)
static int scale_soc_while_chg(struct pm8921_bms_chip *chip,
				int delta_time_us, int new_soc, int prev_soc)
{
	int chg_time_sec;
	int catch_up_sec;
	int scaled_soc;
	int numerator;

	/*
	 * The device must be charging for reporting a higher soc, if
	 * not ignore this soc and continue reporting the prev_soc.
	 * Also don't report a high value immediately slowly scale the
	 * value from prev_soc to the new soc based on a charge time
	 * weighted average
	 */

	/* if we are not charging return last soc */
	if (the_chip->start_percent == -EINVAL)
		return prev_soc;

	chg_time_sec = DIV_ROUND_UP(the_chip->charge_time_us, USEC_PER_SEC);
	catch_up_sec = DIV_ROUND_UP(the_chip->catch_up_time_us, USEC_PER_SEC);
	pr_debug("cts= %d catch_up_sec = %d\n", chg_time_sec, catch_up_sec);

	/*
	 * if we have been charging for more than catch_up time simply return
	 * new soc
	 */
	if (chg_time_sec > catch_up_sec)
		return new_soc;

	numerator = (catch_up_sec - chg_time_sec) * prev_soc
			+ chg_time_sec * new_soc;
	scaled_soc = numerator / catch_up_sec;

	pr_debug("cts = %d new_soc = %d prev_soc = %d scaled_soc = %d\n",
			chg_time_sec, new_soc, prev_soc, scaled_soc);

	return scaled_soc;
}

static bool is_shutdown_soc_within_limits(struct pm8921_bms_chip *chip, int soc)
{
	if (shutdown_soc_invalid) {
		pr_debug("NOT forcing shutdown soc = %d\n", chip->shutdown_soc);
		return 0;
	}

	if (abs(chip->shutdown_soc - soc) > chip->shutdown_soc_valid_limit) {
		pr_debug("rejecting shutdown soc = %d, soc = %d limit = %d\n",
			chip->shutdown_soc, soc,
			chip->shutdown_soc_valid_limit);
		shutdown_soc_invalid = 1;
		return 0;
	}

	return 1;
}

static void update_power_supply(struct pm8921_bms_chip *chip)
{
	if (chip->batt_psy == NULL || chip->batt_psy < 0)
		chip->batt_psy = power_supply_get_by_name("battery");

	if (chip->batt_psy > 0)
		power_supply_changed(chip->batt_psy);
}

/*
 * Remaining Usable Charge = remaining_charge (charge at ocv instance)
 *				- coloumb counter charge
 *				- unusable charge (due to battery resistance)
 * SOC% = (remaining usable charge/ fcc - usable_charge);
 */
static int calculate_state_of_charge(struct pm8921_bms_chip *chip,
					struct pm8921_soc_params *raw,
					int batt_temp, int chargecycles)
{
	int remaining_usable_charge_uah, fcc_uah, unusable_charge_uah;
	int remaining_charge_uah, soc;
	int cc_uah;
	int rbatt;
	int iavg_ua;
	int delta_time_s;
	int new_ocv;
	int new_rc_uah;
	int new_ucc_uah;
	int new_rbatt;
	int shutdown_soc;
	int new_calculated_soc;
	static int firsttime = 1;

	
	pr_debug(" chip->rbatt_sf_lut->sf[0][0] = %d\n",
			chip->rbatt_sf_lut->sf[0][0]);
	pr_debug(" chip->pc_temp_ocv_lut->ocv[0][0] = %d\n",
			chip->pc_temp_ocv_lut->ocv[0][0]);
	pr_debug(" chip->pc_temp_ocv_lut->ocv[28][0] = %d\n",
			chip->pc_temp_ocv_lut->ocv[28][0]);
	
	calculate_soc_params(chip, raw, batt_temp, chargecycles,
						&fcc_uah,
						&unusable_charge_uah,
						&remaining_charge_uah,
						&cc_uah,
						&rbatt,
						&iavg_ua,
						&delta_time_s);

	/* calculate remaining usable charge */
	remaining_usable_charge_uah = remaining_charge_uah
					- cc_uah
					- unusable_charge_uah;

	pr_debug("RUC = %duAh\n", remaining_usable_charge_uah);
	
	chip->bmsmon_data.ruc_uah
			= remaining_usable_charge_uah;
	pr_debug("remaining_charge_uah = %duAh, cc_uah = %duAh\n",
			remaining_charge_uah, cc_uah);
	pr_debug("unusable_charge_uah = %duAh, RUC = %duAh\n",
			unusable_charge_uah, remaining_usable_charge_uah);
	
	if (fcc_uah - unusable_charge_uah <= 0) {
		pr_warn("FCC = %duAh, UUC = %duAh forcing soc = 0\n",
						fcc_uah, unusable_charge_uah);
		soc = 0;
	} else {
		soc = DIV_ROUND_CLOSEST((remaining_usable_charge_uah * 100),
					(fcc_uah - unusable_charge_uah));
	}

	
	chip->bmsmon_data.soc_0 = soc;
	pr_debug("SOC_0 = %d, RC = %d, FCC = %d, UUC = %d\n",
		chip->bmsmon_data.soc_0, remaining_usable_charge_uah,
		fcc_uah, unusable_charge_uah);
	

	if (firsttime && soc < 0) {
		/*
		 * first time calcualtion and the pon ocv  is too low resulting
		 * in a bad soc. Adjust ocv such that we get 0 soc
		 */
		pr_debug("soc is %d, adjusting pon ocv to make it 0\n", soc);
		adjust_rc_and_uuc_for_specific_soc(
						chip,
						batt_temp, chargecycles,
						0,
						fcc_uah, unusable_charge_uah,
						cc_uah, remaining_charge_uah,
						rbatt,
						&new_ocv,
						&new_rc_uah, &new_ucc_uah,
						&new_rbatt);
		chip->last_ocv_uv = new_ocv;
		remaining_charge_uah = new_rc_uah;
		unusable_charge_uah = new_ucc_uah;
		rbatt = new_rbatt;

		remaining_usable_charge_uah = remaining_charge_uah
					- cc_uah
					- unusable_charge_uah;

		soc = DIV_ROUND_CLOSEST((remaining_usable_charge_uah * 100),
					(fcc_uah - unusable_charge_uah));
		pr_debug("DONE for O soc is %d, pon ocv adjusted to %duV\n",
				soc, chip->last_ocv_uv);
	}

	
	chip->bmsmon_data.soc_1 = soc;
	

	if (soc > 100)
		soc = 100;

	if (soc < 0) {
		pr_err("bad rem_usb_chg = %d rem_chg %d,"
				"cc_uah %d, unusb_chg %d\n",
				remaining_usable_charge_uah,
				remaining_charge_uah,
				cc_uah, unusable_charge_uah);

		pr_err("for bad rem_usb_chg last_ocv_uv = %d"
				"chargecycles = %d, batt_temp = %d"
				"fcc = %d soc =%d\n",
				chip->last_ocv_uv, chargecycles, batt_temp,
				fcc_uah, soc);
		soc = 0;
	}

	
	chip->bmsmon_data.soc_2 = soc;
	

	mutex_lock(&soc_invalidation_mutex);
	shutdown_soc = chip->shutdown_soc;

	if (firsttime && soc != shutdown_soc
			&& is_shutdown_soc_within_limits(chip, soc)) {
		/*
		 * soc for the first time - use shutdown soc
		 * to adjust pon ocv since it is a small percent away from
		 * the real soc
		 */
		pr_debug("soc = %d before forcing shutdown_soc = %d\n",
							soc, shutdown_soc);
		adjust_rc_and_uuc_for_specific_soc(
						chip,
						batt_temp, chargecycles,
						shutdown_soc,
						fcc_uah, unusable_charge_uah,
						cc_uah, remaining_charge_uah,
						rbatt,
						&new_ocv,
						&new_rc_uah, &new_ucc_uah,
						&new_rbatt);

		chip->pon_ocv_uv = chip->last_ocv_uv;
		chip->last_ocv_uv = new_ocv;
		remaining_charge_uah = new_rc_uah;
		unusable_charge_uah = new_ucc_uah;
		rbatt = new_rbatt;

		remaining_usable_charge_uah = remaining_charge_uah
					- cc_uah
					- unusable_charge_uah;

		soc = DIV_ROUND_CLOSEST((remaining_usable_charge_uah * 100),
					(fcc_uah - unusable_charge_uah));

		pr_debug("DONE for shutdown_soc = %d soc is %d, adjusted ocv to %duV\n",
				shutdown_soc, soc, chip->last_ocv_uv);
	}
	mutex_unlock(&soc_invalidation_mutex);

	
	chip->bmsmon_data.soc_3 = soc;
	

	pr_debug("SOC before adjustment = %d\n", soc);
	new_calculated_soc = adjust_soc(chip, soc, batt_temp, chargecycles,
			rbatt, fcc_uah, unusable_charge_uah, cc_uah);

	pr_debug("calculated SOC = %d\n", new_calculated_soc);
	if (new_calculated_soc != calculated_soc)
		update_power_supply(chip);

	calculated_soc = new_calculated_soc;

	
	chip->bmsmon_data.soc_4 = calculated_soc;
	

	firsttime = 0;
	return calculated_soc;
}

#define CALCULATE_SOC_MS	20000
static void calculate_soc_work(struct work_struct *work)
{
	struct pm8921_bms_chip *chip = container_of(work,
				struct pm8921_bms_chip,
				calculate_soc_delayed_work.work);
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_soc_params raw;
	int soc;

	rc = pm8xxx_adc_read(chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->batt_temp_channel, rc);
		return;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;

	mutex_lock(&chip->last_ocv_uv_mutex);
	read_soc_params_raw(chip, &raw);

	soc = calculate_state_of_charge(chip, &raw,
					batt_temp, last_chargecycles);
	mutex_unlock(&chip->last_ocv_uv_mutex);

	schedule_delayed_work(&chip->calculate_soc_delayed_work,
			round_jiffies_relative(msecs_to_jiffies
			(CALCULATE_SOC_MS)));
}

static int report_state_of_charge(struct pm8921_bms_chip *chip)
{
	int soc = calculated_soc;
	int delta_time_us;
	struct timespec now;
	struct pm8xxx_adc_chan_result result;
	int batt_temp;
	int rc;

	if (bms_fake_battery != -EINVAL) {
		pr_debug("Returning Fake SOC = %d%%\n", bms_fake_battery);
		return bms_fake_battery;
	}

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_temp_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;

	do_posix_clock_monotonic_gettime(&now);
	if (chip->t_soc_queried.tv_sec != 0) {
		delta_time_us
		= (now.tv_sec - chip->t_soc_queried.tv_sec) * USEC_PER_SEC
			+ (now.tv_nsec - chip->t_soc_queried.tv_nsec) / 1000;
	} else {
		/* calculation for the first time */
		delta_time_us = 0;
	}

	/*
	 * account for charge time - limit it to SOC_CATCHUP_SEC to
	 * avoid overflows when charging continues for extended periods
	 */
	if (the_chip->start_percent != -EINVAL) {
		if (the_chip->charge_time_us == 0) {
			/*
			 * calculating soc for the first time
			 * after start of chg. Initialize catchup time
			 */
			if (abs(soc - last_soc) < MAX_CATCHUP_SOC)
				the_chip->catch_up_time_us =
				(soc - last_soc) * SOC_CATCHUP_SEC_PER_PERCENT
					 * USEC_PER_SEC;
			else
				the_chip->catch_up_time_us =
				SOC_CATCHUP_SEC_MAX * USEC_PER_SEC;

			if (the_chip->catch_up_time_us < 0)
				the_chip->catch_up_time_us = 0;
		}

		/* add charge time */
		if (the_chip->charge_time_us
				< SOC_CATCHUP_SEC_MAX * USEC_PER_SEC)
			chip->charge_time_us += delta_time_us;

		/* end catchup if calculated soc and last soc are same */
		if (last_soc == soc)
			the_chip->catch_up_time_us = 0;
	}

	/* last_soc < soc  ... scale and catch up */
	if (last_soc != -EINVAL && last_soc < soc && soc != 100)
		soc = scale_soc_while_chg(chip, delta_time_us, soc, last_soc);

	
	chip->bmsmon_data.soc_5 = soc;
	

	last_soc = soc;
	backup_soc_and_iavg(chip, batt_temp, last_soc);
	pr_debug("Reported SOC = %d\n", last_soc);
	
	chip->bmsmon_data.soc = last_soc;
	
	chip->t_soc_queried = now;

	return last_soc;
}

void pm8921_bms_invalidate_shutdown_soc(void)
{
	int calculate_soc = 0;
	struct pm8921_bms_chip *chip = the_chip;
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_soc_params raw;
	int soc;

	pr_debug("Invalidating shutdown soc - the battery was removed\n");
	if (shutdown_soc_invalid)
		return;

	mutex_lock(&soc_invalidation_mutex);
	shutdown_soc_invalid = 1;
	last_soc = -EINVAL;
	if (the_chip) {
		/* reset to pon ocv undoing what the adjusting did */
		if (the_chip->pon_ocv_uv) {
			the_chip->last_ocv_uv = the_chip->pon_ocv_uv;
			calculate_soc = 1;
			pr_debug("resetting ocv to pon_ocv = %d\n",
						the_chip->pon_ocv_uv);
		}
	}
	mutex_unlock(&soc_invalidation_mutex);
	if (!calculate_soc)
		return;

	rc = pm8xxx_adc_read(chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					chip->batt_temp_channel, rc);
		return;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;

	mutex_lock(&chip->last_ocv_uv_mutex);
	read_soc_params_raw(chip, &raw);

	soc = calculate_state_of_charge(chip, &raw,
					batt_temp, last_chargecycles);
	mutex_unlock(&chip->last_ocv_uv_mutex);
}
EXPORT_SYMBOL(pm8921_bms_invalidate_shutdown_soc);

#define MIN_DELTA_625_UV	1000
static void calib_hkadc(struct pm8921_bms_chip *chip)
{
	int voltage, rc;
	struct pm8xxx_adc_chan_result result;
	int usb_chg;
	int this_delta;

	mutex_lock(&chip->calib_mutex);
	rc = pm8xxx_adc_read(the_chip->ref1p25v_channel, &result);
	if (rc) {
		pr_err("ADC failed for 1.25volts rc = %d\n", rc);
		goto out;
	}
	voltage = xoadc_reading_to_microvolt(result.adc_code);

	pr_debug("result 1.25v = 0x%x, voltage = %duV adc_meas = %lld\n",
				result.adc_code, voltage, result.measurement);

	chip->xoadc_v125 = voltage;

	rc = pm8xxx_adc_read(the_chip->ref625mv_channel, &result);
	if (rc) {
		pr_err("ADC failed for 1.25volts rc = %d\n", rc);
		goto out;
	}
	voltage = xoadc_reading_to_microvolt(result.adc_code);

	usb_chg = usb_chg_plugged_in(chip);
	pr_debug("result 0.625V = 0x%x, voltage = %duV adc_meas = %lld "
				"usb_chg = %d\n",
				result.adc_code, voltage, result.measurement,
				usb_chg);

	if (usb_chg)
		chip->xoadc_v0625_usb_present = voltage;
	else
		chip->xoadc_v0625_usb_absent = voltage;

	chip->xoadc_v0625 = voltage;
	if (chip->xoadc_v0625_usb_present && chip->xoadc_v0625_usb_absent) {
		this_delta = chip->xoadc_v0625_usb_present
						- chip->xoadc_v0625_usb_absent;
		pr_debug("this_delta= %duV\n", this_delta);
		if (this_delta > MIN_DELTA_625_UV)
			last_usb_cal_delta_uv = this_delta;
		pr_debug("625V_present= %d, 625V_absent= %d, delta = %duV\n",
			chip->xoadc_v0625_usb_present,
			chip->xoadc_v0625_usb_absent,
			last_usb_cal_delta_uv);
	}
out:
	mutex_unlock(&chip->calib_mutex);
}

static void calibrate_hkadc_work(struct work_struct *work)
{
	struct pm8921_bms_chip *chip = container_of(work,
				struct pm8921_bms_chip, calib_hkadc_work);

	calib_hkadc(chip);
}

void pm8921_bms_calibrate_hkadc(void)
{
	schedule_work(&the_chip->calib_hkadc_work);
}

#define HKADC_CALIB_DELAY_MS	600000
static void calibrate_hkadc_delayed_work(struct work_struct *work)
{
	struct pm8921_bms_chip *chip = container_of(work,
				struct pm8921_bms_chip,
				calib_hkadc_delayed_work.work);

	calib_hkadc(chip);
	schedule_delayed_work(&chip->calib_hkadc_delayed_work,
			round_jiffies_relative(msecs_to_jiffies
			(HKADC_CALIB_DELAY_MS)));
}

int pm8921_bms_get_vsense_avg(int *result)
{
	int rc = -EINVAL;

	if (the_chip) {
		mutex_lock(&the_chip->bms_output_lock);
		pm_bms_lock_output_data(the_chip);
		rc = read_vsense_avg(the_chip, result);
		pm_bms_unlock_output_data(the_chip);
		mutex_unlock(&the_chip->bms_output_lock);
	}

	pr_err("called before initialization\n");
	return rc;
}
EXPORT_SYMBOL(pm8921_bms_get_vsense_avg);

int pm8921_bms_get_battery_current(int *result_ua)
{
	int vsense;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}
	if (the_chip->r_sense == 0) {
		pr_err("r_sense is zero\n");
		return -EINVAL;
	}

	mutex_lock(&the_chip->bms_output_lock);
	pm_bms_lock_output_data(the_chip);
	read_vsense_avg(the_chip, &vsense);
	pm_bms_unlock_output_data(the_chip);
	mutex_unlock(&the_chip->bms_output_lock);
	pr_debug("vsense=%duV\n", vsense);
	/* cast for signed division */
	*result_ua = vsense * 1000 / (int)the_chip->r_sense;
	pr_debug("ibat=%duA\n", *result_ua);
	return 0;
}
EXPORT_SYMBOL(pm8921_bms_get_battery_current);

int pm8921_bms_get_percent_charge(void)
{
	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}

	return report_state_of_charge(the_chip);
}
EXPORT_SYMBOL_GPL(pm8921_bms_get_percent_charge);

int pm8921_bms_get_rbatt(void)
{
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_soc_params raw;
	int fcc_uah;
	int unusable_charge_uah;
	int remaining_charge_uah;
	int cc_uah;
	int rbatt;
	int iavg_ua;
	int delta_time_s;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_temp_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;

	mutex_lock(&the_chip->last_ocv_uv_mutex);

	read_soc_params_raw(the_chip, &raw);

	calculate_soc_params(the_chip, &raw, batt_temp, last_chargecycles,
						&fcc_uah,
						&unusable_charge_uah,
						&remaining_charge_uah,
						&cc_uah,
						&rbatt,
						&iavg_ua,
						&delta_time_s);
	mutex_unlock(&the_chip->last_ocv_uv_mutex);

	return rbatt;
}
EXPORT_SYMBOL_GPL(pm8921_bms_get_rbatt);

int pm8921_bms_get_fcc(void)
{
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;

	if (!the_chip) {
		pr_err("called before initialization\n");
		return -EINVAL;
	}

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
					the_chip->batt_temp_channel, rc);
		return rc;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;
	return calculate_fcc_uah(the_chip, batt_temp, last_chargecycles);
}
EXPORT_SYMBOL_GPL(pm8921_bms_get_fcc);

#define IBAT_TOL_MASK		0x0F
#define OCV_TOL_MASK			0xF0
#define IBAT_TOL_DEFAULT	0x03
#define IBAT_TOL_NOCHG		0x0F
#define OCV_TOL_DEFAULT		0x20
#define OCV_TOL_NO_OCV		0x00
void pm8921_bms_charging_began(void)
{
	struct pm8921_soc_params raw;

	mutex_lock(&the_chip->last_ocv_uv_mutex);
	read_soc_params_raw(the_chip, &raw);
	mutex_unlock(&the_chip->last_ocv_uv_mutex);

	the_chip->start_percent = report_state_of_charge(the_chip);

	bms_start_percent = the_chip->start_percent;
	bms_start_ocv_uv = raw.last_good_ocv_uv;
	calculate_cc_uah(the_chip, raw.cc, &bms_start_cc_uah);
	pm_bms_masked_write(the_chip, BMS_TOLERANCES,
			IBAT_TOL_MASK, IBAT_TOL_DEFAULT);
	the_chip->charge_time_us = 0;
	the_chip->catch_up_time_us = 0;

	the_chip->soc_at_cv = -EINVAL;
	the_chip->prev_chg_soc = -EINVAL;
	pr_debug("start_percent = %u%%\n", the_chip->start_percent);
	
	the_chip->bmsmon_data.start_soc = bms_start_percent;
	the_chip->bmsmon_data.start_ocv_uv = bms_start_ocv_uv;
	the_chip->bmsmon_data.start_cc_uah = bms_start_cc_uah;
	
}
EXPORT_SYMBOL_GPL(pm8921_bms_charging_began);

#define DELTA_FCC_PERCENT	3
#define MIN_START_PERCENT_FOR_LEARNING	30
void pm8921_bms_charging_end(int is_battery_full)
{
	int batt_temp, rc;
	struct pm8xxx_adc_chan_result result;
	struct pm8921_soc_params raw;

	if (the_chip == NULL)
		return;

	
	the_chip->bmsmon_data.charging_end_cnt++;
	if (is_battery_full == 1) {
		the_chip->bmsmon_data.battery_full_cnt++;
	}
	

	rc = pm8xxx_adc_read(the_chip->batt_temp_channel, &result);
	if (rc) {
		pr_err("error reading adc channel = %d, rc = %d\n",
				the_chip->batt_temp_channel, rc);
		return;
	}
	pr_debug("batt_temp phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	batt_temp = (int)result.physical;

	mutex_lock(&the_chip->last_ocv_uv_mutex);

	read_soc_params_raw(the_chip, &raw);

	calculate_cc_uah(the_chip, raw.cc, &bms_end_cc_uah);

	bms_end_ocv_uv = raw.last_good_ocv_uv;

	
	pr_debug("is_battery_full = %d, enable_fcc_learning = %d,"
		"start_percent = %d\n",
		is_battery_full, the_chip->enable_fcc_learning,
		the_chip->start_percent);
	

	if (is_battery_full && the_chip->enable_fcc_learning
		&& the_chip->start_percent <= MIN_START_PERCENT_FOR_LEARNING) {
		int fcc_uah, new_fcc_uah, delta_fcc_uah;

		new_fcc_uah = calculate_real_fcc_uah(the_chip, &raw,
						batt_temp, last_chargecycles,
						&fcc_uah);
		delta_fcc_uah = new_fcc_uah - fcc_uah;
		if (delta_fcc_uah < 0)
			delta_fcc_uah = -delta_fcc_uah;

		if (delta_fcc_uah * 100  > (DELTA_FCC_PERCENT * fcc_uah)) {
			/* new_fcc_uah is outside the scope limit it */
			if (new_fcc_uah > fcc_uah)
				new_fcc_uah
				= (fcc_uah +
					(DELTA_FCC_PERCENT * fcc_uah) / 100);
			else
				new_fcc_uah
				= (fcc_uah -
					(DELTA_FCC_PERCENT * fcc_uah) / 100);

			pr_debug("delta_fcc=%d > %d percent of fcc=%d"
					"restring it to %d\n",
					delta_fcc_uah, DELTA_FCC_PERCENT,
					fcc_uah, new_fcc_uah);
		}

		last_real_fcc_mah = new_fcc_uah/1000;
		last_real_fcc_batt_temp = batt_temp;
		readjust_fcc_table();
	}

	if (is_battery_full) {
		the_chip->ocv_reading_at_100 = raw.last_good_ocv_raw;
		the_chip->cc_reading_at_100 = raw.cc;

		the_chip->last_ocv_uv = the_chip->max_voltage_uv;
		raw.last_good_ocv_uv = the_chip->max_voltage_uv;
		/*
		 * since we are treating this as an ocv event
		 * forget the old cc value
		 */
		the_chip->last_cc_uah = 0;
		pr_debug("EOC BATT_FULL ocv_reading = 0x%x cc = 0x%x\n",
				the_chip->ocv_reading_at_100,
				the_chip->cc_reading_at_100);
	}

	the_chip->end_percent = calculate_state_of_charge(the_chip, &raw,
					batt_temp, last_chargecycles);
	mutex_unlock(&the_chip->last_ocv_uv_mutex);

	bms_end_percent = the_chip->end_percent;

	if (the_chip->end_percent > the_chip->start_percent) {
		last_charge_increase +=
			the_chip->end_percent - the_chip->start_percent;
		if (last_charge_increase > 100) {
			last_chargecycles++;
			last_charge_increase = last_charge_increase % 100;
		}
	}
	
	the_chip->bmsmon_data.end_soc = bms_end_percent;
	the_chip->bmsmon_data.end_ocv_uv = bms_end_ocv_uv;
	the_chip->bmsmon_data.end_cc_uah = bms_end_cc_uah;
	
	pr_debug("end_percent = %u%% last_charge_increase = %d"
			"last_chargecycles = %d\n",
			the_chip->end_percent,
			last_charge_increase,
			last_chargecycles);
	the_chip->start_percent = -EINVAL;
	the_chip->end_percent = -EINVAL;
	the_chip->charge_time_us = 0;
	the_chip->catch_up_time_us = 0;
	the_chip->soc_at_cv = -EINVAL;
	the_chip->prev_chg_soc = -EINVAL;
	pm_bms_masked_write(the_chip, BMS_TOLERANCES,
				IBAT_TOL_MASK, IBAT_TOL_NOCHG);
}
EXPORT_SYMBOL_GPL(pm8921_bms_charging_end);

int pm8921_bms_stop_ocv_updates(struct pm8921_bms_chip *chip)
{
	pr_debug("stopping ocv updates\n");
	return pm_bms_masked_write(chip, BMS_TOLERANCES,
			OCV_TOL_MASK, OCV_TOL_NO_OCV);
}
EXPORT_SYMBOL_GPL(pm8921_bms_stop_ocv_updates);

int pm8921_bms_start_ocv_updates(struct pm8921_bms_chip *chip)
{
	pr_debug("stopping ocv updates\n");
	return pm_bms_masked_write(chip, BMS_TOLERANCES,
			OCV_TOL_MASK, OCV_TOL_DEFAULT);
}
EXPORT_SYMBOL_GPL(pm8921_bms_start_ocv_updates);


int64_t pm8921_bms_get_batt_id_mean_mv(void)
{
	pr_debug("batt_id_mean_mv=%lld\n", the_chip->batt_id_mean_mv);
	return the_chip->batt_id_mean_mv;
}
EXPORT_SYMBOL_GPL(pm8921_bms_get_batt_id_mean_mv);

bool pm8921_bms_is_mean_batt_id_ready(void)
{
	pr_debug("is_mean_batt_id_ready=%d\n",
			the_chip->is_mean_batt_id_ready);
	return the_chip->is_mean_batt_id_ready;
}
EXPORT_SYMBOL_GPL(pm8921_bms_is_mean_batt_id_ready);


static irqreturn_t pm8921_bms_sbi_write_ok_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_cc_thr_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_vsense_thr_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_vsense_for_r_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_ocv_for_r_handler(int irq, void *data)
{
	struct pm8921_bms_chip *chip = data;

	pr_debug("irq = %d triggered", irq);
	schedule_work(&chip->calib_hkadc_work);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_good_ocv_handler(int irq, void *data)
{
	struct pm8921_bms_chip *chip = data;

	pr_debug("irq = %d triggered", irq);
	schedule_work(&chip->calib_hkadc_work);
	return IRQ_HANDLED;
}

static irqreturn_t pm8921_bms_vsense_avg_handler(int irq, void *data)
{
	pr_debug("irq = %d triggered", irq);
	return IRQ_HANDLED;
}

struct pm_bms_irq_init_data {
	unsigned int	irq_id;
	char		*name;
	unsigned long	flags;
	irqreturn_t	(*handler)(int, void *);
};

#define BMS_IRQ(_id, _flags, _handler) \
{ \
	.irq_id		= _id, \
	.name		= #_id, \
	.flags		= _flags, \
	.handler	= _handler, \
}

struct pm_bms_irq_init_data bms_irq_data[] = {
	BMS_IRQ(PM8921_BMS_SBI_WRITE_OK, IRQF_TRIGGER_RISING,
				pm8921_bms_sbi_write_ok_handler),
	BMS_IRQ(PM8921_BMS_CC_THR, IRQF_TRIGGER_RISING,
				pm8921_bms_cc_thr_handler),
	BMS_IRQ(PM8921_BMS_VSENSE_THR, IRQF_TRIGGER_RISING,
				pm8921_bms_vsense_thr_handler),
	BMS_IRQ(PM8921_BMS_VSENSE_FOR_R, IRQF_TRIGGER_RISING,
				pm8921_bms_vsense_for_r_handler),
	BMS_IRQ(PM8921_BMS_OCV_FOR_R, IRQF_TRIGGER_RISING,
				pm8921_bms_ocv_for_r_handler),
	BMS_IRQ(PM8921_BMS_GOOD_OCV, IRQF_TRIGGER_RISING,
				pm8921_bms_good_ocv_handler),
	BMS_IRQ(PM8921_BMS_VSENSE_AVG, IRQF_TRIGGER_RISING,
				pm8921_bms_vsense_avg_handler),
};

static void free_irqs(struct pm8921_bms_chip *chip)
{
	int i;

	for (i = 0; i < PM_BMS_MAX_INTS; i++)
		if (chip->pmic_bms_irq[i]) {
			free_irq(chip->pmic_bms_irq[i], NULL);
			chip->pmic_bms_irq[i] = 0;
		}
}

static int __devinit request_irqs(struct pm8921_bms_chip *chip,
					struct platform_device *pdev)
{
	struct resource *res;
	int ret, i;

	ret = 0;
	bitmap_fill(chip->enabled_irqs, PM_BMS_MAX_INTS);

	for (i = 0; i < ARRAY_SIZE(bms_irq_data); i++) {
		res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,
				bms_irq_data[i].name);
		if (res == NULL) {
			pr_err("couldn't find %s\n", bms_irq_data[i].name);
			goto err_out;
		}
		ret = request_irq(res->start, bms_irq_data[i].handler,
			bms_irq_data[i].flags,
			bms_irq_data[i].name, chip);
		if (ret < 0) {
			pr_err("couldn't request %d (%s) %d\n", res->start,
					bms_irq_data[i].name, ret);
			goto err_out;
		}
		chip->pmic_bms_irq[bms_irq_data[i].irq_id] = res->start;
		pm8921_bms_disable_irq(chip, bms_irq_data[i].irq_id);
	}
	return 0;

err_out:
	free_irqs(chip);
	return -EINVAL;
}

#define EN_BMS_BIT	BIT(7)
#define EN_PON_HS_BIT	BIT(0)
static int __devinit pm8921_bms_hw_init(struct pm8921_bms_chip *chip)
{
	int rc;

	rc = pm_bms_masked_write(chip, BMS_CONTROL,
			EN_BMS_BIT | EN_PON_HS_BIT, EN_BMS_BIT | EN_PON_HS_BIT);
	if (rc) {
		pr_err("failed to enable pon and bms addr = %d %d",
				BMS_CONTROL, rc);
	}

	/* The charger will call start charge later if usb is present */
	pm_bms_masked_write(chip, BMS_TOLERANCES,
				IBAT_TOL_MASK, IBAT_TOL_NOCHG);
	return 0;
}

static void check_initial_ocv(struct pm8921_bms_chip *chip)
{
	int ocv_uv, rc;
	int16_t ocv_raw;
	int usb_chg;

	/*
	 * Check if a ocv is available in bms hw,
	 * if not compute it here at boot time and save it
	 * in the last_ocv_uv.
	 */
	ocv_uv = 0;
	pm_bms_read_output_data(chip, LAST_GOOD_OCV_VALUE, &ocv_raw);
	usb_chg = usb_chg_plugged_in(chip);
	rc = convert_vbatt_raw_to_uv(chip, usb_chg, ocv_raw, &ocv_uv);
	if (rc || ocv_uv == 0) {
		rc = adc_based_ocv(chip, &ocv_uv);
		if (rc) {
			pr_err("failed to read adc based ocv_uv rc = %d\n", rc);
			ocv_uv = DEFAULT_OCV_MICROVOLTS;
		}
	}
	chip->last_ocv_uv = ocv_uv;
	pr_debug("ocv_uv = %d last_ocv_uv = %d\n", ocv_uv, chip->last_ocv_uv);
}


static bool get_mean_value(int64_t *mean_value, int64_t data_array[],
			   unsigned int array_size)
{
	unsigned int count;
	unsigned int max_index = 0;
	unsigned int min_index = 0;
	int64_t average;

	if (array_size <= 3) {
		pr_warn("Invalid array. Need over 3 items\n");
		return false;
	}
	
	
	for (count = 1; count < array_size; ++count) {
		if (data_array[count] > data_array[max_index]) {
			max_index = count;
		}
		if (data_array[count] < data_array[min_index]) {
			min_index = count;
		}
	}

	
	if (max_index == min_index) {
		average = data_array[max_index];
	} else {
		average = 0;
		for (count = 0; count < array_size; ++count) {
			pr_debug("data_array[%d]=%lld\n", count, data_array[count]);
			if ((count != max_index) && (count != min_index)) {
				average += data_array[count];
			}
		}
		average /= (array_size - 2);
	}
	*mean_value = average;

	pr_debug("max: index=%d, val=%lld\n", max_index, data_array[max_index]);
	pr_debug("min: index=%d, val=%lld\n", min_index, data_array[min_index]);
	pr_debug("average=%lld\n", average);

	return true;
}

#define BATT_ID_SAMPLE_NUM		10
#define BATT_ID_SAMPLING_WAIT_MS	10
static int64_t read_battery_id_wrapper(struct pm8921_bms_chip *chip)
{
	int i, rc;
	int64_t mean_mv;
	int64_t batt_id_mv[BATT_ID_SAMPLE_NUM];
	struct pm8xxx_adc_chan_result result;

	for(i=0; i < BATT_ID_SAMPLE_NUM; i++) {
		rc = pm8xxx_adc_read(chip->batt_id_channel, &result);
		if (rc) {
			pr_err("error reading batt id channel = %d, rc = %d\n",
						chip->vbat_channel, rc);
			return rc;
		}else {
			batt_id_mv[i] = result.physical;
		}
		pr_debug("batt_id_mv[%d]=%lld \n", i, batt_id_mv[i]);
		msleep(BATT_ID_SAMPLING_WAIT_MS);
	}

	
	get_mean_value(&mean_mv, batt_id_mv, BATT_ID_SAMPLE_NUM);

	
	chip->batt_id_mean_mv = mean_mv;
	chip->is_mean_batt_id_ready = true;
	return mean_mv;
}


static int64_t read_battery_id(struct pm8921_bms_chip *chip)
{


	return read_battery_id_wrapper(chip);















}








static int set_battery_data(struct pm8921_bms_chip *chip)
{
	int64_t battery_id;






	battery_id = div_u64(read_battery_id(chip),1000);
	if (battery_id < 0) {
		pr_err("cannot read battery id err = %lld\n", battery_id);
		return battery_id;
	}


	if (is_between(chip->batt_id_min, chip->batt_id_max, battery_id)) {
		goto c811_std;
	} else if (is_between(chip->batt_id_min_ext, chip->batt_id_max_ext, battery_id)) {
		goto c811_ext;

	} else {
		pr_warn("invalid battid, c811 1800 assumed batt_id %llx\n",
				battery_id);
		goto c811_std;
	}

c811_std:
		chip->fcc = c811_1800_data.fcc;
		chip->fcc_temp_lut = c811_1800_data.fcc_temp_lut;
		chip->fcc_sf_lut = c811_1800_data.fcc_sf_lut;
		chip->pc_temp_ocv_lut = c811_1800_data.pc_temp_ocv_lut;
		chip->pc_sf_lut = c811_1800_data.pc_sf_lut;
		chip->rbatt_sf_lut = c811_1800_data.rbatt_sf_lut;
		chip->default_rbatt_mohm = c811_1800_data.default_rbatt_mohm;
		chip->delta_rbatt_mohm = c811_1800_data.delta_rbatt_mohm;
		return 0;
c811_ext:
		chip->fcc = c811_2920_data.fcc;
		chip->fcc_temp_lut = c811_2920_data.fcc_temp_lut;
		chip->pc_temp_ocv_lut = c811_2920_data.pc_temp_ocv_lut;
		chip->pc_sf_lut = c811_2920_data.pc_sf_lut;
		chip->rbatt_sf_lut = c811_2920_data.rbatt_sf_lut;
		chip->default_rbatt_mohm = c811_2920_data.default_rbatt_mohm;
		chip->delta_rbatt_mohm = c811_2920_data.delta_rbatt_mohm;
		return 0;
}

enum bms_request_operation {
	CALC_FCC,
	CALC_PC,
	CALC_SOC,
	CALIB_HKADC,
	CALIB_CCADC,
	GET_VBAT_VSENSE_SIMULTANEOUS,
	STOP_OCV,
	START_OCV,
};

static int test_batt_temp = 5;
static int test_chargecycle = 150;
static int test_ocv = 3900000;
enum {
	TEST_BATT_TEMP,
	TEST_CHARGE_CYCLE,
	TEST_OCV,
};


enum {
	SOC_0,
	SOC_1,
	SOC_2,
	SOC_3,
	SOC_4,
	SOC_5,
	SOC,
	FCC,
	RUC,
	RC,
	CC,
	UUC,
	PC_RC,
	PC_CC,
	PC_UUC,
	RBATT,
	IAVG,
	BATT_TEMP,
	IBAT,
	VBAT,
	LAST_GOOD_OCV,
	VSENSE,
	CHARGECYCLES,
	CHARGE_INCREASE,
	BMS_START_SOC,
	BMS_START_OCV,
	BMS_START_CC,
	BMS_END_SOC,
	BMS_END_OCV,
	BMS_END_CC,
	CHARGING_END_CNT,
	BATT_FULL_CNT,
	OCV_RENEW_CNT,
	
	RSENSE,
	MAX_VOLTAGE,
	OCV_EST,
	PC_EST,
	SOC_EST,
	CHG_IS_BATT_FULL_CNT,
	
};


static int get_test_param(void *data, u64 * val)
{
	switch ((int)data) {
	case TEST_BATT_TEMP:
		*val = test_batt_temp;
		break;
	case TEST_CHARGE_CYCLE:
		*val = test_chargecycle;
		break;
	case TEST_OCV:
		*val = test_ocv;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
static int set_test_param(void *data, u64  val)
{
	switch ((int)data) {
	case TEST_BATT_TEMP:
		test_batt_temp = (int)val;
		break;
	case TEST_CHARGE_CYCLE:
		test_chargecycle = (int)val;
		break;
	case TEST_OCV:
		test_ocv = (int)val;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(temp_fops, get_test_param, set_test_param, "%llu\n");

static int get_calc(void *data, u64 * val)
{
	int param = (int)data;
	int ret = 0;
	int ibat_ua, vbat_uv;
	struct pm8921_soc_params raw;

	read_soc_params_raw(the_chip, &raw);

	*val = 0;

	/* global irq number passed in via data */
	switch (param) {
	case CALC_FCC:
		*val = calculate_fcc_uah(the_chip, test_batt_temp,
							test_chargecycle);
		break;
	case CALC_PC:
		*val = calculate_pc(the_chip, test_ocv, test_batt_temp,
							test_chargecycle);
		break;
	case CALC_SOC:
		*val = calculate_state_of_charge(the_chip, &raw,
					test_batt_temp, test_chargecycle);
		break;
	case CALIB_HKADC:
		/* reading this will trigger calibration */
		*val = 0;
		calib_hkadc(the_chip);
		break;
	case CALIB_CCADC:
		/* reading this will trigger calibration */
		*val = 0;
		pm8xxx_calib_ccadc();
		break;
	case GET_VBAT_VSENSE_SIMULTANEOUS:
		/* reading this will call simultaneous vbat and vsense */
		*val =
		pm8921_bms_get_simultaneous_battery_voltage_and_current(
			&ibat_ua,
			&vbat_uv);
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int set_calc(void *data, u64 val)
{
	int param = (int)data;
	int ret = 0;

	switch (param) {
	case STOP_OCV:
		pm8921_bms_stop_ocv_updates(the_chip);
		break;
	case START_OCV:
		pm8921_bms_start_ocv_updates(the_chip);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}
DEFINE_SIMPLE_ATTRIBUTE(calc_fops, get_calc, set_calc, "%llu\n");


static int get_debugfs_data(void *data, u64 * val)
{
	int param = (int)data;
	int ret = 0;
	int soc;

	soc = pm8921_bms_get_percent_charge();

	switch (param) {
	case SOC_0:
		*val = the_chip->bmsmon_data.soc_0;
		break;
	case SOC_1:
		*val = the_chip->bmsmon_data.soc_1;
		break;
	case SOC_2:
		*val = the_chip->bmsmon_data.soc_2;
		break;
	case SOC_3:
		*val = the_chip->bmsmon_data.soc_3;
		break;
	case SOC_4:
		*val = the_chip->bmsmon_data.soc_4;
		break;
	case SOC_5:
		*val = the_chip->bmsmon_data.soc_5;
		break;
	case SOC:
		*val = the_chip->bmsmon_data.soc;
		break;
	case FCC:
		*val = the_chip->bmsmon_data.fcc_uah;
		break;
	case RUC:
		*val = the_chip->bmsmon_data.ruc_uah;
		break;
	case RC:
		*val = the_chip->bmsmon_data.rc_uah;
		break;
	case CC:
		*val = the_chip->bmsmon_data.cc_uah;
		break;
	case UUC:
		*val = the_chip->bmsmon_data.uuc_uah;
		break;
	case PC_RC:
		*val = the_chip->bmsmon_data.pc_rc;
		break;
	case PC_CC:
		*val = the_chip->bmsmon_data.pc_cc;
		break;
	case PC_UUC:
		*val = the_chip->bmsmon_data.pc_uuc;
		break;
	case RBATT:
		*val = the_chip->bmsmon_data.rbatt;
		break;
	case IAVG:
		*val = the_chip->bmsmon_data.iavg_ua;
		break;
	case BATT_TEMP:
		*val = the_chip->bmsmon_data.batt_temp;
		break;
	case IBAT:
		*val = the_chip->bmsmon_data.ibat_ua;
		break;
	case VBAT:
		*val = the_chip->bmsmon_data.vbat_uv;
		break;
	case LAST_GOOD_OCV:
		*val = the_chip->bmsmon_data.last_good_ocv_uv;
		break;
	case VSENSE:
		*val = the_chip->bmsmon_data.vsense_uv;
		break;
	case CHARGECYCLES:
		*val = last_chargecycles;
		break;
	case CHARGE_INCREASE:
		*val = last_charge_increase;
		break;
	case BMS_START_SOC:
		*val = the_chip->bmsmon_data.start_soc;
		break;
	case BMS_START_OCV:
		*val = the_chip->bmsmon_data.start_ocv_uv;
		break;
	case BMS_START_CC:
		*val = the_chip->bmsmon_data.start_cc_uah;
		break;
	case BMS_END_SOC:
		*val = the_chip->bmsmon_data.end_soc;
		break;
	case BMS_END_OCV:
		*val = the_chip->bmsmon_data.end_ocv_uv;
		break;
	case BMS_END_CC:
		*val = the_chip->bmsmon_data.end_cc_uah;
		break;
	case CHARGING_END_CNT:
		*val = the_chip->bmsmon_data.charging_end_cnt;
		break;
	case BATT_FULL_CNT:
		*val = the_chip->bmsmon_data.battery_full_cnt;
		break;
	case OCV_RENEW_CNT:
		*val = the_chip->bmsmon_data.ocv_renew_cnt;
		break;
	
	case RSENSE:
		*val = the_chip->r_sense;
		break;
	case MAX_VOLTAGE:
		*val = the_chip->max_voltage_uv;
		break;
	case OCV_EST:
		*val = the_chip->bmsmon_data.ocv_est_uv;
		break;
	case PC_EST:
		*val = the_chip->bmsmon_data.pc_est;
		break;
	case SOC_EST:
		*val = the_chip->bmsmon_data.soc_est;
		break;
	case CHG_IS_BATT_FULL_CNT:
		*val = pm8921_is_battery_full();
		break;
	
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}
DEFINE_SIMPLE_ATTRIBUTE(debugfs_fops, get_debugfs_data, NULL, "%lld\n");


static int get_reading(void *data, u64 * val)
{
	int param = (int)data;
	int ret = 0;
	struct pm8921_soc_params raw;

	read_soc_params_raw(the_chip, &raw);

	*val = 0;

	switch (param) {
	case CC_MSB:
	case CC_LSB:
		*val = raw.cc;
		break;
	case LAST_GOOD_OCV_VALUE:
		*val = raw.last_good_ocv_uv;
		break;
	case VSENSE_AVG:
		read_vsense_avg(the_chip, (uint *)val);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}
DEFINE_SIMPLE_ATTRIBUTE(reading_fops, get_reading, NULL, "%lld\n");

static int get_rt_status(void *data, u64 * val)
{
	int i = (int)data;
	int ret;

	/* global irq number passed in via data */
	ret = pm_bms_get_rt_status(the_chip, i);
	*val = ret;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(rt_fops, get_rt_status, NULL, "%llu\n");

static int get_reg(void *data, u64 * val)
{
	int addr = (int)data;
	int ret;
	u8 temp;

	ret = pm8xxx_readb(the_chip->dev->parent, addr, &temp);
	if (ret) {
		pr_err("pm8xxx_readb to %x value = %d errored = %d\n",
			addr, temp, ret);
		return -EAGAIN;
	}
	*val = temp;
	return 0;
}

static int set_reg(void *data, u64 val)
{
	int addr = (int)data;
	int ret;
	u8 temp;

	temp = (u8) val;
	ret = pm8xxx_writeb(the_chip->dev->parent, addr, temp);
	if (ret) {
		pr_err("pm8xxx_writeb to %x value = %d errored = %d\n",
			addr, temp, ret);
		return -EAGAIN;
	}
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(reg_fops, get_reg, set_reg, "0x%02llx\n");

static void create_debugfs_entries(struct pm8921_bms_chip *chip)
{
	int i;

	chip->dent = debugfs_create_dir("pm8921-bms", NULL);

	if (IS_ERR(chip->dent)) {
		pr_err("pmic bms couldnt create debugfs dir\n");
		return;
	}

	debugfs_create_file("BMS_CONTROL", 0644, chip->dent,
			(void *)BMS_CONTROL, &reg_fops);
	debugfs_create_file("BMS_OUTPUT0", 0644, chip->dent,
			(void *)BMS_OUTPUT0, &reg_fops);
	debugfs_create_file("BMS_OUTPUT1", 0644, chip->dent,
			(void *)BMS_OUTPUT1, &reg_fops);
	debugfs_create_file("BMS_TEST1", 0644, chip->dent,
			(void *)BMS_TEST1, &reg_fops);

	debugfs_create_file("test_batt_temp", 0644, chip->dent,
				(void *)TEST_BATT_TEMP, &temp_fops);
	debugfs_create_file("test_chargecycle", 0644, chip->dent,
				(void *)TEST_CHARGE_CYCLE, &temp_fops);
	debugfs_create_file("test_ocv", 0644, chip->dent,
				(void *)TEST_OCV, &temp_fops);

	debugfs_create_file("read_cc", 0644, chip->dent,
				(void *)CC_MSB, &reading_fops);
	debugfs_create_file("read_last_good_ocv", 0644, chip->dent,
				(void *)LAST_GOOD_OCV_VALUE, &reading_fops);
	debugfs_create_file("read_vbatt_for_rbatt", 0644, chip->dent,
				(void *)VBATT_FOR_RBATT, &reading_fops);
	debugfs_create_file("read_vsense_for_rbatt", 0644, chip->dent,
				(void *)VSENSE_FOR_RBATT, &reading_fops);
	debugfs_create_file("read_ocv_for_rbatt", 0644, chip->dent,
				(void *)OCV_FOR_RBATT, &reading_fops);
	debugfs_create_file("read_vsense_avg", 0644, chip->dent,
				(void *)VSENSE_AVG, &reading_fops);

	debugfs_create_file("show_fcc", 0644, chip->dent,
				(void *)CALC_FCC, &calc_fops);
	debugfs_create_file("show_pc", 0644, chip->dent,
				(void *)CALC_PC, &calc_fops);
	debugfs_create_file("show_soc", 0644, chip->dent,
				(void *)CALC_SOC, &calc_fops);
	debugfs_create_file("calib_hkadc", 0644, chip->dent,
				(void *)CALIB_HKADC, &calc_fops);
	debugfs_create_file("calib_ccadc", 0644, chip->dent,
				(void *)CALIB_CCADC, &calc_fops);
	debugfs_create_file("stop_ocv", 0644, chip->dent,
				(void *)STOP_OCV, &calc_fops);
	debugfs_create_file("start_ocv", 0644, chip->dent,
				(void *)START_OCV, &calc_fops);

	debugfs_create_file("simultaneous", 0644, chip->dent,
			(void *)GET_VBAT_VSENSE_SIMULTANEOUS, &calc_fops);

	for (i = 0; i < ARRAY_SIZE(bms_irq_data); i++) {
		if (chip->pmic_bms_irq[bms_irq_data[i].irq_id])
			debugfs_create_file(bms_irq_data[i].name, 0444,
				chip->dent,
				(void *)bms_irq_data[i].irq_id,
				&rt_fops);
	}
}


static void create_eval_debugfs_entries(struct pm8921_bms_chip *chip)
{
	chip->dent = debugfs_create_dir("pm8921-bms-eval", NULL);

	if (IS_ERR(chip->dent)) {
		pr_err("pmic bms couldnt create debugfs dir\n");
		return;
	}
	
	debugfs_create_file("chg_batt_full_cnt", 0644, chip->dent,
			(void *)CHG_IS_BATT_FULL_CNT, &debugfs_fops);
	debugfs_create_file("SOC_EST", 0644, chip->dent,
			(void *)SOC_EST, &debugfs_fops);
	debugfs_create_file("PC_EST", 0644, chip->dent,
			(void *)PC_EST, &debugfs_fops);
	debugfs_create_file("OCV_EST", 0644, chip->dent,
			(void *)OCV_EST, &debugfs_fops);
	debugfs_create_file("MAX_VOLTAGE", 0644, chip->dent,
			(void *)MAX_VOLTAGE, &debugfs_fops);
	debugfs_create_file("RSENSE", 0644, chip->dent,
			(void *)RSENSE, &debugfs_fops);
	
	debugfs_create_file("ocv_renew_cnt", 0644, chip->dent,
			(void *)OCV_RENEW_CNT, &debugfs_fops);
	debugfs_create_file("is_battery_full_cnt", 0644, chip->dent,
			(void *)BATT_FULL_CNT, &debugfs_fops);
	debugfs_create_file("charging_end_cnt", 0644, chip->dent,
			(void *)CHARGING_END_CNT, &debugfs_fops);
	debugfs_create_file("vsense_avg", 0644, chip->dent,
			(void *)VSENSE_AVG, &reading_fops);
	debugfs_create_file("end_cc", 0644, chip->dent,
			(void *)BMS_END_CC, &debugfs_fops);
	debugfs_create_file("end_ocv", 0644, chip->dent,
			(void *)BMS_END_OCV, &debugfs_fops);
	debugfs_create_file("end_soc", 0644, chip->dent,
			(void *)BMS_END_SOC, &debugfs_fops);
	debugfs_create_file("start_cc", 0644, chip->dent,
			(void *)BMS_START_CC, &debugfs_fops);
	debugfs_create_file("start_ocv", 0644, chip->dent,
			(void *)BMS_START_OCV, &debugfs_fops);
	debugfs_create_file("start_soc", 0644, chip->dent,
			(void *)BMS_START_SOC, &debugfs_fops);
	debugfs_create_file("charge_increase", 0644, chip->dent,
			(void *)CHARGE_INCREASE, &debugfs_fops);
	debugfs_create_file("chargecycles", 0644, chip->dent,
			(void *)CHARGECYCLES, &debugfs_fops);
	debugfs_create_file("vsense", 0644, chip->dent,
			(void *)VSENSE, &debugfs_fops);
	debugfs_create_file("last_good_ocv", 0644, chip->dent,
			(void *)LAST_GOOD_OCV, &debugfs_fops);
	debugfs_create_file("vbat", 0644, chip->dent,
			(void *)VBAT, &debugfs_fops);
	debugfs_create_file("ibat", 0644, chip->dent,
			(void *)IBAT, &debugfs_fops);
	debugfs_create_file("batt_temp", 0644, chip->dent,
			(void *)BATT_TEMP, &debugfs_fops);
	debugfs_create_file("iavg", 0644, chip->dent,
			(void *)IAVG, &debugfs_fops);
	debugfs_create_file("rbatt", 0644, chip->dent,
			(void *)RBATT, &debugfs_fops);
	debugfs_create_file("PC_UUC", 0644, chip->dent,
			(void *)PC_UUC, &debugfs_fops);
	debugfs_create_file("PC_CC", 0644, chip->dent,
			(void *)PC_CC, &debugfs_fops);
	debugfs_create_file("PC_RC", 0644, chip->dent,
			(void *)PC_RC, &debugfs_fops);
	debugfs_create_file("UUC", 0644, chip->dent,
			(void *)UUC, &debugfs_fops);
	debugfs_create_file("CC", 0644, chip->dent,
			(void *)CC, &debugfs_fops);
	debugfs_create_file("RC", 0644, chip->dent,
			(void *)RC, &debugfs_fops);
	debugfs_create_file("RUC", 0644, chip->dent,
			(void *)RUC, &debugfs_fops);
	debugfs_create_file("FCC", 0644, chip->dent,
			(void *)FCC, &debugfs_fops);
	debugfs_create_file("SOC", 0644, chip->dent,
			(void *)SOC, &debugfs_fops);
	debugfs_create_file("SOC_5", 0644, chip->dent,
			(void *)SOC_5, &debugfs_fops);
	debugfs_create_file("SOC_4", 0644, chip->dent,
			(void *)SOC_4, &debugfs_fops);
	debugfs_create_file("SOC_3", 0644, chip->dent,
			(void *)SOC_3, &debugfs_fops);
	debugfs_create_file("SOC_2", 0644, chip->dent,
			(void *)SOC_2, &debugfs_fops);
	debugfs_create_file("SOC_1", 0644, chip->dent,
			(void *)SOC_1, &debugfs_fops);
	debugfs_create_file("SOC_0", 0644, chip->dent,
			(void *)SOC_0, &debugfs_fops);

}


#define REG_SBI_CONFIG		0x04F
#define PAGE3_ENABLE_MASK	0x6
#define PROGRAM_REV_MASK	0x0F
#define PROGRAM_REV		0x9
static int read_ocv_trim(struct pm8921_bms_chip *chip)
{
	int rc;
	u8 reg, sbi_config;

	rc = pm8xxx_readb(chip->dev->parent, REG_SBI_CONFIG, &sbi_config);
	if (rc) {
		pr_err("error = %d reading sbi config reg\n", rc);
		return rc;
	}

	reg = sbi_config | PAGE3_ENABLE_MASK;
	rc = pm8xxx_writeb(chip->dev->parent, REG_SBI_CONFIG, reg);
	if (rc) {
		pr_err("error = %d writing sbi config reg\n", rc);
		return rc;
	}

	rc = pm8xxx_readb(chip->dev->parent, TEST_PROGRAM_REV, &reg);
	if (rc)
		pr_err("Error %d reading %d addr %d\n",
			rc, reg, TEST_PROGRAM_REV);
	pr_err("program rev reg is 0x%x\n", reg);
	reg &= PROGRAM_REV_MASK;

	/* If the revision is equal or higher do not adjust trim delta */
	if (reg >= PROGRAM_REV) {
		chip->amux_2_trim_delta = 0;
		goto restore_sbi_config;
	}

	rc = pm8xxx_readb(chip->dev->parent, AMUX_TRIM_2, &reg);
	if (rc) {
		pr_err("error = %d reading trim reg\n", rc);
		return rc;
	}

	pr_err("trim reg is 0x%x\n", reg);
	chip->amux_2_trim_delta = abs(0x49 - reg);
	pr_err("trim delta is %d\n", chip->amux_2_trim_delta);

restore_sbi_config:
	rc = pm8xxx_writeb(chip->dev->parent, REG_SBI_CONFIG, sbi_config);
	if (rc) {
		pr_err("error = %d writing sbi config reg\n", rc);
		return rc;
	}

	return 0;
}

static int __devinit pm8921_bms_probe(struct platform_device *pdev)
{
	int rc = 0;
	int vbatt;
	struct pm8921_bms_chip *chip;
	const struct pm8921_bms_platform_data *pdata
				= pdev->dev.platform_data;

	if (!pdata) {
		pr_err("missing platform data\n");
		return -EINVAL;
	}

	chip = kzalloc(sizeof(struct pm8921_bms_chip), GFP_KERNEL);
	if (!chip) {
		pr_err("Cannot allocate pm_bms_chip\n");
		return -ENOMEM;
	}

	mutex_init(&chip->bms_output_lock);
	mutex_init(&chip->last_ocv_uv_mutex);
	chip->dev = &pdev->dev;
	chip->r_sense = pdata->r_sense;
	chip->v_cutoff = pdata->v_cutoff;
	chip->max_voltage_uv = pdata->max_voltage_uv;
	chip->chg_term_ua = pdata->chg_term_ua;
	chip->batt_type = pdata->battery_type;
	chip->rconn_mohm = pdata->rconn_mohm;
	chip->start_percent = -EINVAL;
	chip->end_percent = -EINVAL;
	chip->ocv_reading_at_100 = OCV_RAW_UNINITIALIZED;
	chip->prev_last_good_ocv_raw = OCV_RAW_UNINITIALIZED;
	chip->shutdown_soc_valid_limit = pdata->shutdown_soc_valid_limit;
	chip->adjust_soc_low_threshold = pdata->adjust_soc_low_threshold;

	chip->batt_id_min = pdata->batt_id_min;
	chip->batt_id_max = pdata->batt_id_max;
	chip->batt_id_min_ext = pdata->batt_id_min_ext;
	chip->batt_id_max_ext = pdata->batt_id_max_ext;

	if (chip->adjust_soc_low_threshold >= 45)
		chip->adjust_soc_low_threshold = 45;

	chip->prev_pc_unusable = -EINVAL;
	chip->soc_at_cv = -EINVAL;

	chip->ignore_shutdown_soc = pdata->ignore_shutdown_soc;

	
	pr_debug("r_sense = %d\n", chip->r_sense);
	pr_debug("v_cutoff = %d\n", chip->v_cutoff);
	pr_debug("max_voltage_uv = %d\n", chip->max_voltage_uv);
	pr_debug("chg_term_ua = %d\n", chip->chg_term_ua);
	pr_debug("batt_type = %d\n", chip->batt_type);
	pr_debug("rconn_mohm = %d\n", chip->rconn_mohm);
	pr_debug("start_percent = %d\n", chip->start_percent);
	pr_debug("end_percent = %d\n", chip->end_percent);
	pr_debug("shutdown_soc_valid_limit = %d\n",
			chip->shutdown_soc_valid_limit);
	pr_debug("adjust_soc_low_threshold = %d\n",
			chip->adjust_soc_low_threshold);
	pr_debug("prev_pc_unusable = %d\n", chip->prev_pc_unusable);
	pr_debug("soc_at_cv = %d\n", chip->soc_at_cv);
	pr_debug("ignore_shutdown_soc = %d\n", chip->ignore_shutdown_soc);
	

	rc = set_battery_data(chip);
	if (rc) {
		pr_err("%s bad battery data %d\n", __func__, rc);
		goto free_chip;
	}

	if (chip->pc_temp_ocv_lut == NULL) {
		pr_err("temp ocv lut table is NULL\n");
		rc = -EINVAL;
		goto free_chip;
	}

	/* set defaults in the battery data */
	if (chip->default_rbatt_mohm <= 0)
		chip->default_rbatt_mohm = DEFAULT_RBATT_MOHMS;

	chip->batt_temp_channel = pdata->bms_cdata.batt_temp_channel;
	chip->vbat_channel = pdata->bms_cdata.vbat_channel;
	chip->ref625mv_channel = pdata->bms_cdata.ref625mv_channel;
	chip->ref1p25v_channel = pdata->bms_cdata.ref1p25v_channel;
	chip->batt_id_channel = pdata->bms_cdata.batt_id_channel;
	chip->revision = pm8xxx_get_revision(chip->dev->parent);
	chip->enable_fcc_learning = pdata->enable_fcc_learning;

	
	pr_debug("enable_fcc_learning = %d\n", chip->enable_fcc_learning);
	

	mutex_init(&chip->calib_mutex);
	INIT_WORK(&chip->calib_hkadc_work, calibrate_hkadc_work);
	INIT_DELAYED_WORK(&chip->calib_hkadc_delayed_work,
				calibrate_hkadc_delayed_work);

	INIT_DELAYED_WORK(&chip->calculate_soc_delayed_work,
			calculate_soc_work);

	rc = request_irqs(chip, pdev);
	if (rc) {
		pr_err("couldn't register interrupts rc = %d\n", rc);
		goto free_chip;
	}

	rc = pm8921_bms_hw_init(chip);
	if (rc) {
		pr_err("couldn't init hardware rc = %d\n", rc);
		goto free_irqs;
	}

	read_shutdown_soc_and_iavg(chip);

	rc = set_battery_data(chip);
	if (rc) {
		pr_err("%s bad battery data %d\n", __func__, rc);
		goto free_chip;
	}

	if (chip->pc_temp_ocv_lut == NULL) {
		pr_err("temp ocv lut table is NULL\n");
		rc = -EINVAL;
		goto free_chip;
	}

	
	if (chip->default_rbatt_mohm <= 0)
		chip->default_rbatt_mohm = DEFAULT_RBATT_MOHMS;



	platform_set_drvdata(pdev, chip);
	the_chip = chip;
	create_debugfs_entries(chip);
	
	create_eval_debugfs_entries(chip);
	

	rc = read_ocv_trim(chip);
	if (rc) {
		pr_err("couldn't adjust ocv_trim rc= %d\n", rc);
		goto free_irqs;
	}
	check_initial_ocv(chip);

	/* start periodic hkadc calibration */
	schedule_delayed_work(&chip->calib_hkadc_delayed_work, 0);

	/* enable the vbatt reading interrupts for scheduling hkadc calib */
	pm8921_bms_enable_irq(chip, PM8921_BMS_GOOD_OCV);
	pm8921_bms_enable_irq(chip, PM8921_BMS_OCV_FOR_R);

	calculate_soc_work(&(chip->calculate_soc_delayed_work.work));

	get_battery_uvolts(chip, &vbatt);
	pr_info("OK battery_capacity_at_boot=%d volt = %d ocv = %d\n",
				pm8921_bms_get_percent_charge(),
				vbatt, chip->last_ocv_uv);

	pm8xxx_smpl_set_delay(2);

	return 0;

free_irqs:
	free_irqs(chip);
free_chip:
	kfree(chip);
	return rc;
}

static int __devexit pm8921_bms_remove(struct platform_device *pdev)
{
	struct pm8921_bms_chip *chip = platform_get_drvdata(pdev);

	free_irqs(chip);
	kfree(chip->adjusted_fcc_temp_lut);
	platform_set_drvdata(pdev, NULL);
	the_chip = NULL;
	kfree(chip);
	return 0;
}

static struct platform_driver pm8921_bms_driver = {
	.probe	= pm8921_bms_probe,
	.remove	= __devexit_p(pm8921_bms_remove),
	.driver	= {
		.name	= PM8921_BMS_DEV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init pm8921_bms_init(void)
{
	return platform_driver_register(&pm8921_bms_driver);
}

static void __exit pm8921_bms_exit(void)
{
	platform_driver_unregister(&pm8921_bms_driver);
}

late_initcall(pm8921_bms_init);
module_exit(pm8921_bms_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMIC8921 bms driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:" PM8921_BMS_DEV_NAME);
