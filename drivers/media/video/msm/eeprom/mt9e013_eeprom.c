/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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

#include <linux/module.h>
#include "msm_camera_eeprom.h"
#include "msm_camera_i2c.h"

DEFINE_MUTEX(mt9e013_eeprom_mutex);
static struct msm_eeprom_ctrl_t mt9e013_eeprom_t;

static const struct i2c_device_id mt9e013_eeprom_i2c_id[] = {
	{"mt9e013_eeprom", (kernel_ulong_t)&mt9e013_eeprom_t},
	{ }
};

static struct i2c_driver mt9e013_eeprom_i2c_driver = {
	.id_table = mt9e013_eeprom_i2c_id,
	.probe  = msm_eeprom_i2c_probe,
	.remove = __exit_p(mt9e013_eeprom_i2c_remove),
	.driver = {
		.name = "mt9e013_eeprom",
	},
};

static int __init mt9e013_eeprom_i2c_add_driver(void)
{
	int rc = 0;
	rc = i2c_add_driver(mt9e013_eeprom_t.i2c_driver);
	return rc;
}

static struct v4l2_subdev_core_ops mt9e013_eeprom_subdev_core_ops = {
	.ioctl = msm_eeprom_subdev_ioctl,
};

static struct v4l2_subdev_ops mt9e013_eeprom_subdev_ops = {
	.core = &mt9e013_eeprom_subdev_core_ops,
};

uint8_t mt9e013_wbcalib_data[6];
uint8_t mt9e013_afcalib_data[8];






struct msm_calib_wb mt9e013_wb_data;
struct msm_calib_af mt9e013_af_data;


static struct sensor_lenscalib_data mt9e013_lcalib_data;





static struct msm_camera_eeprom_info_t mt9e013_calib_supp_info = {
	{TRUE, sizeof(struct msm_calib_af), 0, 1},
	{TRUE, sizeof(struct msm_calib_wb), 1, 1000},


	{TRUE, sizeof(struct sensor_lenscalib_data), 2, 1},




	{FALSE, 0, 0, 1},
};

static struct msm_camera_eeprom_read_t mt9e013_eeprom_read_tbl[] = {
	{0x313800, &mt9e013_wbcalib_data[0], 6, 0},
	{0x343800, &mt9e013_afcalib_data[0], 8, 0},


	{0x353800, &mt9e013_lcalib_data.lsc_data[0], 375, 0},
	{0x363800, &mt9e013_lcalib_data.lsc_data[375], 375, 0},
	{0x373800, &mt9e013_lcalib_data.lsc_data[750], 355, 0},






};


static struct msm_camera_eeprom_data_t mt9e013_eeprom_data_tbl[] = {
	{&mt9e013_af_data, sizeof(struct msm_calib_af)},
	{&mt9e013_wb_data, sizeof(struct msm_calib_wb)},


	{&mt9e013_lcalib_data, sizeof(struct sensor_lenscalib_data)},




};

































































































































static void mt9e013_format_wbdata(void)
{
	mt9e013_wb_data.r_over_g = (uint16_t)(mt9e013_wbcalib_data[0] << 8) |
		mt9e013_wbcalib_data[1];
	mt9e013_wb_data.b_over_g = (uint16_t)(mt9e013_wbcalib_data[2] << 8) |
		mt9e013_wbcalib_data[3];
	mt9e013_wb_data.gr_over_gb = (uint16_t)(mt9e013_wbcalib_data[4] << 8) |
		mt9e013_wbcalib_data[5];
}

static void mt9e013_format_afdata(void)
{
	mt9e013_af_data.typinf_dac = (uint16_t)(mt9e013_afcalib_data[0] << 8) |
		mt9e013_afcalib_data[1];
	mt9e013_af_data.typmacro_dac = (uint16_t)
		(mt9e013_afcalib_data[2] << 8) | mt9e013_afcalib_data[3];
	mt9e013_af_data.inf_dac = (uint16_t)(mt9e013_afcalib_data[4] << 8) |
		mt9e013_afcalib_data[5];
	mt9e013_af_data.macro_dac = (uint16_t)(mt9e013_afcalib_data[6] << 8) |
		mt9e013_afcalib_data[7];
}

static void mt9e013_set_eeprom_pageaddr
	(struct msm_eeprom_ctrl_t *ectrl, uint32_t *paddr)
{
	int rc = 0;
	int16_t temp_read;
	uint16_t page = (*paddr & 0xFF0000) >> 8;

	rc = msm_camera_i2c_write(&ectrl->i2c_client, 0x3134, 0xCD95,
		MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0)
		CDBG("%s: eeprom write failed\n", __func__);
	rc = msm_camera_i2c_write(&ectrl->i2c_client, 0x304C, page,
		MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0)
		CDBG("%s: eeprom write failed\n", __func__);
	rc = msm_camera_i2c_write(&ectrl->i2c_client, 0x304A, 0x0010,
		MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0)
		CDBG("%s: eeprom write failed\n", __func__);
	rc = msm_camera_i2c_poll(&ectrl->i2c_client, 0x304A, 0x0020,
		MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0)
		CDBG("%s: eeprom poll failed\n", __func__);
	rc = msm_camera_i2c_read(&ectrl->i2c_client, 0x304A, &temp_read,
		MSM_CAMERA_I2C_WORD_DATA);
	CDBG("%s: eeprom read data =%d\n", __func__, temp_read);
	if (rc < 0)
		CDBG("%s: eeprom read failed\n", __func__);

	if ((temp_read & 0x0040) == 0) {
		CDBG("%s: eeprom read failed\n", __func__);
		return;
	}
	return;

}

void mt9e013_format_calibrationdata(void)
{
	mt9e013_format_wbdata();
	mt9e013_format_afdata();





}

static struct msm_eeprom_ctrl_t mt9e013_eeprom_t = {
	.i2c_driver = &mt9e013_eeprom_i2c_driver,
	.i2c_addr = 0x6C,
	.eeprom_v4l2_subdev_ops = &mt9e013_eeprom_subdev_ops,

	.i2c_client = {
		.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
	},

	.eeprom_mutex = &mt9e013_eeprom_mutex,

	.func_tbl = {
		.eeprom_init = NULL,
		.eeprom_release = NULL,
		.eeprom_get_info = msm_camera_eeprom_get_info,
		.eeprom_get_data = msm_camera_eeprom_get_data,
		.eeprom_set_dev_addr = mt9e013_set_eeprom_pageaddr,
		.eeprom_format_data = mt9e013_format_calibrationdata,
	},
	.info = &mt9e013_calib_supp_info,
	.info_size = sizeof(struct msm_camera_eeprom_info_t),
	.read_tbl = mt9e013_eeprom_read_tbl,
	.read_tbl_size = ARRAY_SIZE(mt9e013_eeprom_read_tbl),
	.data_tbl = mt9e013_eeprom_data_tbl,
	.data_tbl_size = ARRAY_SIZE(mt9e013_eeprom_data_tbl),
};

subsys_initcall(mt9e013_eeprom_i2c_add_driver);
MODULE_DESCRIPTION("mt9e013 EEPROM");
MODULE_LICENSE("GPL v2");
